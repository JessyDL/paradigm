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

dir_path = os.path.dirname(os.path.realpath(__file__))

parser = ArgumentParser(description='Generate build files for the current project.')
parser.add_argument("-g", "--generator", const="auto", default="auto", nargs='?',
                    help="Set the generator for the project (MSVC, Make, or Ninja)", dest="generator")
parser.add_argument("-c", "--compiler", const="auto", default="auto", nargs='?',
                    help="Set the compiler for the project (MSVC or CLang)", dest="compiler")
parser.add_argument("--arch", const="x64", default="x64", nargs='?', 
                    help="Architecture type to generate for", dest="architecture")
parser.add_argument("--clean", const="all", default="", nargs='?', 
                    help="Allows you to clean the project files, build files, or all", dest="clean")
parser.add_argument("--root", const=dir_path, default=dir_path, nargs='?', 
                    help="Override for the root directory to work from", dest="root_dir")
parser.add_argument("--build_dir", const="builds", default="builds", nargs='?', 
                    help="Override for the build directory, this is relative to the root", dest="build_dir")
parser.add_argument("--project_dir", const="project_files", default="project_files", nargs='?', 
                    help="Override for the project files directory, this is relative to the root", dest="project_dir")
parser.add_argument("--vulkan", const="1.1.108", default="1.1.108", nargs='?', 
                    help="vulkan version to use", dest="vk_version")
parser.add_argument("--build_config", const="Release", default="Release", nargs='?',
                    help="build configuration to use when the build flag is true", dest="build_config")
parser.add_argument("--vk_static", action='store_true',dest="vk_static", help="when this flag is set, vulkan will statically bind")
parser.add_argument("-u", "--update", action='store_true', dest="cmake_update")
parser.add_argument("-v", "--verbose", action='store_true', dest="verbose")
parser.add_argument("-b", "--build", action='store_true', help="build the target as well", dest="build")
parser.add_argument("--cmake_params", action='store', type=str, nargs='*',dest="cmake_params")
parser.add_argument("--graphics", action='store', default="vulkan, gles", type=str, nargs='+',choices=['vulkan','gles','molten'],dest="graphics")
args = parser.parse_args()

if args.verbose:
    print(args)

# validating
args.generator = args.generator.lower()
if args.generator == 'auto':
    args.generator = 'msvc'

cmake_generator = "ninja"
if args.generator == "msvc":
    if args.architecture == 'x64':
        cmake_generator = "Visual Studio 16 2019"
    elif args.architecture == 'arm':
        cmake_generator = "Visual Studio 16 2019"
elif args.generator == 'make':
    cmake_generator = "Unix Makefiles"
		
args.compiler = args.compiler.lower

project_dir = os.path.join(args.root_dir, args.project_dir, args.generator, args.architecture)
build_dir = os.path.join(args.root_dir, args.build_dir, args.generator, args.architecture)

if not os.path.exists(project_dir):
    os.makedirs(project_dir)

if not os.path.exists(build_dir):
    os.makedirs(build_dir)

if args.clean == 'all' or args.clean == 'build':
    print("cleaning: {0}".format(build_dir))
    for the_file in os.listdir(build_dir):
        file_path = os.path.join(build_dir, the_file)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path): shutil.rmtree(file_path, onerror=onerror)
        except Exception as e:
            print(e)
						
if args.clean == 'all' or args.clean == 'project':
    print("cleaning: {0}".format(project_dir))
    for the_file in os.listdir(project_dir):
        file_path = os.path.join(project_dir, the_file)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path): shutil.rmtree(file_path, onerror=onerror)
        except Exception as e:
            print(e)
            raise

working_dir = os.getcwd()
os.chdir(project_dir)


cmakeExe="cmake"
if(sys.platform.startswith('win')):
    cmakeExe="cmake.exe"

retCode = 0
if not os.path.exists(os.path.join(project_dir, "CMakeFiles")):
    print("generating project files")
    cmakeCmd = [cmakeExe, "-G", cmake_generator, \
		 "-DPE_BUILD_DIR="+build_dir, \
		 "-DVK_VERSION="+args.vk_version, \
		 "-DVK_STATIC="+("ON" if args.vk_static else "OFF"), \
		 "-DCMAKE_BUILD_TYPE="+args.build_config, \
		 "-DPE_VULKAN="+("ON" if "vulkan" in args.graphics else "OFF"), \
		 "-DPE_GLES="+("ON" if "gles" in args.graphics else "OFF"), \
		 "-DPE_MOLTEN="+("ON" if "molten" in args.graphics else "OFF")]
    if args.cmake_params:
        cmakeCmd = cmakeCmd + args.cmake_params
    print("invoking cmake")
    if args.verbose:
        print("using arguments: ", cmakeCmd)
    cmakeCmd = cmakeCmd + [ "-H"+ args.root_dir, "-B"+project_dir]
    retCode = subprocess.check_call(cmakeCmd, shell=False)
    print("returncode ", retCode)
elif args.cmake_update:
    print("updating project files")
    cmakeCmd = [cmakeExe, r".", \
		 "-DVK_VERSION="+args.vk_version, \
		 "-DCMAKE_BUILD_TYPE="+args.build_config, \
		 "-DPE_VULKAN="+("ON" if "vulkan" in args.graphics else "OFF"), \
		 "-DPE_GLES="+("ON" if "gles" in args.graphics else "OFF"), \
		 "-DPE_MOLTEN="+("ON" if "molten" in args.graphics else "OFF")]
    if args.cmake_params:
        cmakeCmd = cmakeCmd + args.cmake_params
    retCode = subprocess.check_call(cmakeCmd, shell=False)
    print("returncode ", retCode)
	

if args.build and retCode == 0:
    print("building now...")
    cmakeCmd = [cmakeExe, "--build", r".", "--config", args.build_config]
    retCode = subprocess.check_call(cmakeCmd, shell=False)
    print("returncode ", retCode)
	
os.chdir(working_dir)

if retCode == 0:
    print(bcolors.GREEN + "completed building the project" + bcolors.NC)