from argparse import ArgumentParser
import io
import os
import shutil
import subprocess

CURRENT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.join(CURRENT_DIR, '..')

def run_command(command=[], directory=None, print_stdout=False, catch_stdout=False):
    process = subprocess.Popen(command, stdout=subprocess.PIPE, cwd=directory)
    output = []
    if print_stdout:
        for line in io.TextIOWrapper(process.stdout, newline=''):
            if not line.endswith('\r') and catch_stdout:
                output.append(line[:len(line)-1] if line[-1] == os.linesep else line)
    elif catch_stdout:
        for line in io.TextIOWrapper(process.stdout, newline=os.linesep):
            line = line.rsplit('\r', maxsplit=1)[-1]
            output.append(line[:len(line)-1])
    process.stdout.close()
    process.wait()
    if process.returncode != 0:
        raise Exception(f"Raised exitcode '{process.returncode}' while trying to run the command '{' '.join(comm for comm in command)}'")
    return output

def generate(directory, project="main"):
    os.makedirs(directory, exist_ok=True)
    if not os.path.exists(os.path.join(directory, 'settings.gradle')):
        run_command(command=[
                'gradle', 'init',
                '--type', 'basic',
                '--dsl', 'groovy',
                '--project-name', project,
                '--incubating'
            ], directory=directory, print_stdout=True)

    android_templates = os.path.join(CURRENT_DIR, 'templates', 'android')

    templates = [os.path.join(dp, f) for dp, dn, filenames in os.walk(android_templates) for f in filenames if os.path.splitext(f)[1] == '.template']

    for template in templates:
        os.makedirs(os.path.dirname(os.path.join(directory, os.path.relpath(template, android_templates))), exist_ok=True)
        with open(template, 'r') as template_file, open(os.path.join(directory, os.path.relpath(template, android_templates).rsplit('.', maxsplit=1)[0]), 'w') as file:
            template = template_file.read()
            template = template.replace('{{PROJECT}}', project)
            template = template.replace('{{COMPANY}}', "paradigmengine")
            file.write(template)

    resources = [os.path.join(dp, f) for dp, dn, filenames in os.walk(android_templates) for f in filenames if os.path.splitext(f)[1] != '.template']
    for resource in resources:
        os.makedirs(os.path.dirname(os.path.join(directory, os.path.relpath(resource, android_templates))), exist_ok=True)
        with open(resource, 'rb') as resource_file, open(os.path.join(directory, os.path.relpath(resource, android_templates)), 'wb') as file:
            file.write(resource_file.read())

    # create the symlinks to the source code
    cpp_dir = os.path.join(directory, 'app', 'src', 'main')
    [ os.symlink(os.path.join(ROOT_DIR, dir), os.path.join(cpp_dir, dir)) for dir in ['psl', 'core', 'extern', 'cmake', 'CMakeLists.txt', 'tools'] if not os.path.exists(os.path.join(cpp_dir, dir))]

def parse_installed_packages(channel=0):
    sdkman_output = run_command(["sdkmanager", "--list", f"--channel={str(channel)}"], catch_stdout=True)
    
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

def install_required_packages():
    packages = parse_installed_packages(channel=3)

    def install_required(package, channel=0):
        if package in packages["installed"]:
            return
        print(f"Missing '{package}', trying to install now..")
        if package in packages["available"]:
            print(f"installing '{package}'")
            run_command(command=["sdkmanager", package, f"--channel={str(channel)}"], print_stdout=True, catch_stdout=False)
        else:
            raise Exception(f"Could not find the required sdk package '{package}'")

    install_required("platforms;android-31")
    install_required("cmake;3.22.1", channel=3)
    install_required("ndk;25.0.8151533", channel=1)

def build(directory, type='debug'):
    commands =[]
    if type == 'debug':
        commands = [':app:bundleDebug']
    elif type == 'release':
        commands = [':app:bundleRelease']

    run_command(['./gradlew'] + commands, directory=directory, print_stdout=True)

def install(directory, type='debug'):
    run_command(['adb', 'install', '-r', os.path.join(directory, 'app', 'build', 'outputs', 'apk', type, f'app-{type}.apk')], print_stdout=True)

def run():
    run_command(['adb', 'shell', 'am', 'start', '-n', 'com.paradigmengine.main/com.paradigmengine.main.MainActivity'], print_stdout=True)
    
def main():
    parser = ArgumentParser()
    parser.add_argument("--output", default=os.path.join(ROOT_DIR, "builds", "android"))
    parser.add_argument("--sdk", default=None, help="set to override the used android sdk, falls back to path available one")
    parser.add_argument("-f", "--force", action="store_true", help="Forcibly generate the build directory even if it already exists")
    parser.add_argument("-r", "--run", action="store_true", help="run the intent after installation")
    parser.add_argument("-i", "--install", action="store_true", help="install the apk")
    parser.add_argument("-b", "--build", action="store_true", help="build the apk")
    parser.add_argument("-t", "--type", default="debug", help="type to generate")
    parser.add_argument("--purge", action="store_true", help="Delete previous install, and generate from scratch")
    args = parser.parse_args()

    install_required_packages()
    if args.purge:
        args.force = True
        shutil.rmtree(args.output, ignore_errors=True)


    if not os.path.exists(args.output) or args.force:
        generate(args.output)

    if args.build:
        build(args.output, args.type)
    if args.install:
        install(args.output, args.type)
    if args.run:
        run()

if __name__ == "__main__":
    main()

