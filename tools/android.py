from argparse import ArgumentParser
from email.mime import application
import io
import os
import shutil
import subprocess

CURRENT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.join(CURRENT_DIR, '..')

def run_command(command=[], directory=None, print_stdout=False, catch_stdout=False):
    process = subprocess.Popen(command, stdout=os.sys.stdout if print_stdout and not catch_stdout else subprocess.PIPE, stderr=os.sys.stderr, cwd=directory)
    output = []
    if catch_stdout:
        if print_stdout:
            for line in io.TextIOWrapper(process.stdout, newline=''):
                if not line.endswith('\r'):
                    output.append(line[:len(line)-1] if line[-1] == os.linesep else line)
        else:
            for line in io.TextIOWrapper(process.stdout, newline=os.linesep):
                line = line.rsplit('\r', maxsplit=1)[-1]
                output.append(line[:len(line)-1])
        process.stdout.close()
    process.wait()
    if process.returncode != 0:
        raise Exception(f"Raised exitcode '{process.returncode}' while trying to run the command '{' '.join(comm for comm in command)}'")
    return output

class Android:
    def __init__(self, directory, sdk=None, gradle=None, bundletool=None, skip_setup=False) -> None:
        self._sdk = sdk
        if self._sdk != None and self._sdk != "":
            os.environ["ANDROID_SDK_ROOT"] = self._sdk
        elif "ANDROID_SDK_ROOT" in os.environ:
            self._sdk = os.environ["ANDROID_SDK_ROOT"]
        self._gradle = gradle or ""
        self._bundletool = bundletool or ""
        self._directory = directory
        self._generated = os.path.exists(directory) and os.path.exists(os.path.join(directory, 'build.gradle'))
        self._regenerate_apks = set()
        if not skip_setup:
            packages = Android._parse_installed_packages(channel=3, sdk=self._sdk)
            for dependency, channel in Android.dependencies():
                Android._install_required(packages, dependency, channel, sdk=self._sdk)

    def _install_required(packages, package, channel=0, sdk=None):
        if package in packages["installed"]:
            print(f"sdk package '{package}' was found")
            return
        print(f"Missing '{package}', trying to install now..")
        if package in packages["available"]:
            print(f"installing '{package}'")
            application = os.path.join(sdk, 'cmdline-tools', 'latest', 'bin', 'sdkmanager') if sdk != None else 'sdkmanager'
            run_command(command=[application, package, f"--channel={str(channel)}"], print_stdout=True, catch_stdout=False)
        else:
            raise Exception(f"Could not find the required sdk package '{package}'")
    def _parse_installed_packages(channel=0, sdk=None):
        application = os.path.join(sdk, 'cmdline-tools', 'latest', 'bin', 'sdkmanager') if sdk != None else 'sdkmanager'
        sdkman_output = run_command([application, "--list", f"--channel={str(channel)}"], catch_stdout=True)
        
        packages = {'installed': {}, 'available': {}}
        for line in [package for package in sdkman_output if package.count('|') == 3]:
            name, version, _, location = (l.strip() for l in line.split('|'))
            if name == "Path" or name.startswith('-'):
                continue
            packages['installed'][name] = {'version': version, 'location': location}

        for line in [package for package in sdkman_output if package.count('|') == 2]:
            name, version, _ = (l.strip() for l in line.split('|'))
            if name == "Path" or name.startswith('-'):
                continue
            packages['available'][name] = {'version': version }

        return packages

    def dependencies():
        return [("platforms;android-31", 0), ("cmake;3.22.1", 3), ("ndk;25.0.8151533", 1)]
    
    def is_generated(self) -> bool:
        return self._generated

    def generate(self, overwrite=False) -> None:
        if self._generated:
            if not overwrite: return

            shutil.rmtree(self._directory, ignore_errors=True)
            self._generated = False

        os.makedirs(self._directory, exist_ok=True)
        if not os.path.exists(os.path.join(self._directory, 'settings.gradle')):
            application = os.path.join(self._gradle, 'gradle')
            run_command(command=[
                    application, 'init',
                    '--type', 'basic',
                    '--dsl', 'groovy',
                    '--project-name', "main",
                    '--incubating'
                ], directory=self._directory, print_stdout=True)

        android_build_root = os.path.join(CURRENT_DIR, '..', 'core', 'main', 'android')
        shutil.copytree(android_build_root, self._directory, dirs_exist_ok=True)
        
        # create the symlinks to the source code
        cpp_dir = os.path.join(self._directory, 'main', 'src', 'main')
        [ os.symlink(os.path.join(ROOT_DIR, dir), os.path.join(cpp_dir, dir)) for dir in ['psl', 'core', 'extern', 'cmake', 'CMakeLists.txt', 'tools'] if not os.path.exists(os.path.join(cpp_dir, dir))]
        self._generated = True

    def build(self, type="debug", arguments=None) -> None:
        if not self._generated:
            raise Exception("Project has not been generated yet")
        print(f"building main-{type}.aab")
        commands =[]
        if type == 'debug':
            commands = [':main:bundleDebug']
        elif type == 'release':
            commands = [':main:bundleRelease']
        if arguments is not None:
            commands.extend([f'-Pparadigm.{key}={value}' for key, value in zip(*[iter(arguments)]*2)])
        
        commands.append('--stacktrace')
        run_command(['./gradlew'] + commands, directory=self._directory, print_stdout=True)
        self._regenerate_apks.add(type)

    def build_apks(self, type="debug") -> None:
        path = os.path.join(self._directory, 'main', 'build', 'outputs', 'bundle', type, f'main-{type}.aab')
        output = os.path.join(self._directory, 'main', 'build', 'outputs', 'bundle', type, f'main-{type}.apks')
        if not os.path.exists(path):
            raise Exception(f"No AAB generated yet, please run 'build' first. Could not find aab at '{path}'")

        print(f"building main-{type}.apks")
        application = os.path.join(self._bundletool, 'bundletool')
        run_command([
            application,
            'build-apks',
            f"--bundle={path}",
            f"--output={output}",
            "--overwrite",
            '--local-testing'], directory=self._directory, print_stdout=True)
        if type in self._regenerate_apks:
            self._regenerate_apks.remove(type)

    def install(self, type='debug'):
        path = os.path.join(self._directory, 'main', 'build', 'outputs', 'bundle', type, f'main-{type}.apks')
        if not os.path.exists(path):
            self.build_apks(type)

        if type in self._regenerate_apks:
            print("main-{type}.apks out of date, trying to rebuild...")
            self.build_apks(type)

        print(f"installing main-{type}.apks")
        application = os.path.join(self._bundletool, 'bundletool')
        run_command([application, 'install-apks', f"--apks={path}"], directory=self._directory, print_stdout=True)

    def run(self, type=None):
        if type is not None and type in self._regenerate_apks:
            print("main-{type}.apks out of date, trying to rebuild...")
            self.build_apks(type)
            self.install(type)
        output = run_command(['adb', 'shell', 'pm', 'list', 'packages', 'com.paradigmengine.main'], catch_stdout=True)
        if(len(output) != 1 or 'com.paradigmengine.main' not in output[0]):
            if type is not None:
                print(f"app was not installed, will try to install the '{type}' variant now")
                self.install(type)
                return self.run()
            raise Exception("app wasn't installed, please install first")
        
        application = os.path.join(self._sdk, 'platform-tools', 'adb') if self._sdk != None else 'adb'
        run_command([application, 'shell', 'am', 'start', '-n', 'com.paradigmengine.main/android.app.NativeActivity'], print_stdout=True)


def main():
    parser = ArgumentParser()
    parser.add_argument("--output", default=os.path.join(ROOT_DIR, "builds", "android"))
    parser.add_argument("--sdk", default=None, help="set to override the used android sdk, falls back to path available one")
    parser.add_argument("--gradle", default=None, help="set to override the used gradle, falls back to path available one")
    parser.add_argument("--bundletool", default=None, help="set to override the used bundletool, falls back to path available one")
    parser.add_argument("-f", "--force", action="store_true", help="Forcibly generate the build directory even if it already exists")
    parser.add_argument("-r", "--run", action="store_true", help="run the intent after installation")
    parser.add_argument("-i", "--install", action="store_true", help="install the apk")
    parser.add_argument("-b", "--build", default=False, nargs='*', help="build the .aab")
    parser.add_argument("-a", "--apks", action="store_true", help="build the .apks from the .aab")
    parser.add_argument("-t", "--type", default="debug", help="type to generate")
    parser.add_argument("--purge", action="store_true", help="Delete previous install, and generate from scratch")
    parser.add_argument("--skip-setup", action="store_true", help="Skip the setup phase where the SDK tools are installed")
    args = parser.parse_args()

    android = Android(args.output, sdk=args.sdk, gradle=args.gradle, bundletool=args.bundletool, skip_setup=args.skip_setup)
    android.generate(overwrite=args.purge)

    if args.type is not None:
        args.type = args.type.lower()

    if isinstance(args.build, list):
        android.build(args.type, args.build)
    if args.apks:
        android.build_apks(args.type)
    if args.install:
        android.install(args.type)
    if args.run:
        android.run(args.type)

if __name__ == "__main__":
    main()

