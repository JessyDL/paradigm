from argparse import ArgumentParser
import os 
import sys
import platform
import shutil
import subprocess

import functools
print = functools.partial(print, flush=True)

def onerror(func, path, exc_info):
    """
    Error handler for ``shutil.rmtree``.

    If the error is due to an access error (read only file)
    it attempts to add write permission and then retries.

    If the error is for another reason it re-raises the error.
    
    Usage : ``shutil.rmtree(path, onerror=onerror)``
    """
    import stat
    if not os.access(path, os.W_OK):
        # Is the error an access error ?
        os.chmod(path, stat.S_IWUSR)
        func(path)
    else:
        print(path)
        raise

class bcolors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    NC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
        
class Paradigm(object):
    def initialize(self, directory = os.path.dirname(os.path.realpath(__file__))):
        build_arguments = ArgumentParser(description='Generate build files for the current project.')
        build_arguments.add_argument("-g", "--generator", const="auto", default="auto", nargs='?',
                            help="Set the generator for the project (MSVC, Make, or Ninja)", dest="generator")
        build_arguments.add_argument("-c", "--compiler", const="auto", default="auto", nargs='?',
                            help="Set the compiler for the project (MSVC or CLang)", dest="compiler")
        build_arguments.add_argument("--arch", const="x64", default="x64", nargs='?', 
                            help="Architecture type to generate for", dest="architecture")
        build_arguments.add_argument("--clean", const="all", default="", nargs='?', 
                            help="Allows you to clean the project files, build files, or all", dest="clean")
        build_arguments.add_argument("--root", const=directory, default=directory, nargs='?', 
                            help="Override for the root directory to work from", dest="root_dir")
        build_arguments.add_argument("--build_dir", const="builds", default="builds", nargs='?', 
                            help="Override for the build directory, this is relative to the root", dest="build_dir")
        build_arguments.add_argument("--project_dir", const="project_files", default="project_files", nargs='?', 
                            help="Override for the project files directory, this is relative to the root", dest="project_dir")
        build_arguments.add_argument("--vulkan", const="1.1.108", default="1.1.108", nargs='?', 
                            help="vulkan version to use", dest="vk_version")
        build_arguments.add_argument("--build_config", const="Release", default="Release", nargs='?',
                            help="build configuration to use when the build flag is true", dest="build_config")
        build_arguments.add_argument("--vk_static", action='store_true',dest="vk_static", help="when this flag is set, vulkan will statically bind")
        build_arguments.add_argument("-u", "--update", action='store_true', dest="cmake_update")
        build_arguments.add_argument("-v", "--verbose", action='store_true', dest="verbose")
        build_arguments.add_argument("-b", "--build", action='store_true', help="build the target as well", dest="build")
        build_arguments.add_argument("--cmake_params", action='store', type=str, nargs='*',dest="cmake_params")
        build_arguments.add_argument("--graphics", action='store', default="vulkan, gles", type=str, nargs='+',choices=['vulkan','gles','molten'],dest="graphics")
        return build_arguments

    def parse(self, parser):
        args, remaining_argv = parser.parse_known_args()
        args.generator = args.generator.lower()
        if args.generator == 'auto':
            args.generator = 'msvc'

        if args.generator == "msvc":
            if args.architecture == 'x64':
                args.generator = "Visual Studio 16 2019"
            elif args.architecture == 'arm':
                args.generator = "Visual Studio 16 2019"
        elif args.generator == 'make':
            args.generator = "Unix Makefiles"
                
        args.compiler = args.compiler.lower

        args.project_dir = os.path.join(args.root_dir, args.project_dir, args.generator, args.architecture)
        args.build_dir = os.path.join(args.root_dir, args.build_dir, args.generator, args.architecture)

        if not os.path.exists(args.project_dir):
            os.makedirs(args.project_dir)

        if not os.path.exists(args.build_dir):
            os.makedirs(args.build_dir)
            
        if args.clean == 'all' or args.clean == 'build':
            self.clean(args.build_dir)
                                
        if args.clean == 'all' or args.clean == 'project':
            self.clean(args.project_dir)

        if args.verbose:
            print(args)
        return args, remaining_argv

    def clean(self, directory):
        print("cleaning: {0}".format(directory))
        for the_file in os.listdir(directory):
            file_path = os.path.join(directory, the_file)
            try:
                if os.path.isfile(file_path):
                    os.unlink(file_path)
                elif os.path.isdir(file_path): shutil.rmtree(file_path, onerror=onerror)
            except Exception as e:
                print(e)
    def generate_command(self, args):
        working_dir = os.getcwd()
        os.chdir(args.project_dir)
        
        cmakeExe="cmake"
        if(sys.platform.startswith('win')):
            cmakeExe="cmake.exe"
            
        cmakeCmd = []
        
        if not os.path.exists(os.path.join(args.project_dir, "CMakeFiles")):
            cmakeCmd = [cmakeExe, "-G", args.generator, \
                 "-DPE_BUILD_DIR="+args.build_dir, \
                 "-DVK_VERSION="+args.vk_version, \
                 "-DVK_STATIC="+("ON" if args.vk_static else "OFF"), \
                 "-DCMAKE_BUILD_TYPE="+args.build_config, \
                 "-DPE_VULKAN="+("ON" if "vulkan" in args.graphics else "OFF"), \
                 "-DPE_GLES="+("ON" if "gles" in args.graphics else "OFF"), \
                 "-DPE_MOLTEN="+("ON" if "molten" in args.graphics else "OFF")]
            if args.cmake_params:
                cmakeCmd = cmakeCmd + args.cmake_params
            cmakeCmd = cmakeCmd + [ "-H"+ args.root_dir, "-B"+args.project_dir]
        elif args.cmake_update:
            cmakeCmd = [cmakeExe, r".", \
                 "-DVK_VERSION="+args.vk_version, \
                 "-DCMAKE_BUILD_TYPE="+args.build_config, \
                 "-DPE_VULKAN="+("ON" if "vulkan" in args.graphics else "OFF"), \
                 "-DPE_GLES="+("ON" if "gles" in args.graphics else "OFF"), \
                 "-DPE_MOLTEN="+("ON" if "molten" in args.graphics else "OFF")]
            if args.cmake_params:
                cmakeCmd = cmakeCmd + args.cmake_params
                
        os.chdir(working_dir)
        return cmakeCmd
         
    def build_command(self, args):
        cmakeExe="cmake"
        if(sys.platform.startswith('win')):
            cmakeExe="cmake.exe"
            
        cmakeCmd = [cmakeExe, "--build", r".", "--config", args.build_config]
        return cmakeCmd
        
    def build(self, directory, generate_cmd, build_cmd=""):
        working_dir = os.getcwd()
        os.chdir(directory)

        print("setting up project files")        
        retCode = subprocess.check_call(generate_cmd, shell=False)            

        if build_cmd and retCode == 0:
            print("building now...")
            retCode = subprocess.check_call(build_cmd, shell=False)
            
        os.chdir(working_dir)

        if retCode == 0:
            print(bcolors.GREEN + "completed building the project" + bcolors.NC)
        else:
            print(bcolors.FAIL + "failed building the project" + bcolors.NC)
    
    def __call__(self):
        build_arguments = self.initialize()
        args, remaining_argv = self.parse(build_arguments)
        generate_cmd = self.generate_command(args)
        build_cmd=""
        if(args.verbose):
            build_cmd = self.build_command(args)
        
        self.build(args.project_dir, generate_cmd, build_cmd)

def main():
    p = Paradigm()
    p()

if __name__ == "__main__":main()