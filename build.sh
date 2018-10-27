#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

BG_BLUE='\e[44m'

BG_NC='\e[49m'
# ---------------------------------------------------------------------------------------------------------------------
# arguments
# ---------------------------------------------------------------------------------------------------------------------

# who will be the generator to invoke for compilation
# options: MSVC, Ninja, and Make.
declare GENERATOR="MSVC"
# who will be compiling the project
# options: VC++, CLang, and LLVM (unsupported, planned).
declare COMPILER="VC++"
# platform specific architecture (note know your platform so you don't make impossible combinations).
# options: x64, and ARM.
declare ARCHITECTURE="x64"	
# should we clean the project files directory? This will wipe the generated project files directory.
declare CLEAN_CMAKE=false
# should we clean the build output directory? This will wipe the build output directory.
declare CLEAN_BUILD=false
# the root directory where the initial CMakeLists.txt file resides. This is where we will invoke CMake.
declare ROOT_DIRECTORY=$PWD
# output directory for builds.
declare BUILD_FOLDER=$PWD/builds
# where to generate the project files.
declare CMAKE_FOLDER=$PWD/project_files
# what vulkan version should we target?
# warning: this should correspond to the internally used "volk" library in case you dynamically link.
declare VULKAN_VERSION="1.1.82.1"
# where can we find the vulkan SDK path?
# note: it will automatically try to find it depending on some platform defaults.
declare VULKAN_ROOT=""
# when set to true it will run cmake again to update the project files found in the target directory.
# this is useful for when you want to tweak parameters, and not generate the full project again.
declare CMAKE_UPDATE=false
# should we statically link to the vulkan library? By default we dynamically link.
declare VULKAN_STATIC=false
# will be set to true in case we pass an argument that assumes we're building cmake.
# with this we can verify, and warn the user when the project won't be built (and ignore the arguments).
declare CUSTOM_ARGUMENTS=false
declare VERBOSE_LOGGING=false


# we will parse all arguments
# flags start with a '-' and other arguments are of the syntax ARGUMENT_NAME=ARGUMENT_VALUE
for (( i=1; i<=$#; i++))
do	
	if [ "${!i}" == "-v" ]; then
		VERBOSE_LOGGING=true
	fi
	
	if [ "${!i}" == "--help" ]; then
		echo "I'm useful help information!"
		exit 0
	fi
done

for (( i=1; i<=$#; i++))
do	
	ARGUMENT="${!i}"	
	if [ "${!i}" == "-v" ]; then
		continue
	fi
	
	if [[ ${ARGUMENT:0:1} == "\"" ]]; then
		ARGUMENT=${ARGUMENT:1:$((${#ARGUMENT}-1))}
		for (( ni=$i+1; ni<=$#; ni++)); do
			NARGUMENT="${!ni}"
			if [[ ${NARGUMENT:$((${#NARGUMENT}-1)):1} == "\"" ]]; then
				ARGUMENT=$ARGUMENT${NARGUMENT:0:$((${#NARGUMENT}-1))}
				let "i = $ni"
				break
			else
				ARGUMENT=$ARGUMENT$NARGUMENT
			fi
		done
	fi
	
	if [[ ${ARGUMENT:0:1} == "-" ]]; then
		KEY=${ARGUMENT:1:$((${#ARGUMENT}-1))}
		VALUE=true
		
		declare -i next_i=$i+1
		if [ $next_i -le "$#" ]; then
			declare NEXTARG="${!next_i}"
			if [[ $NEXTARG != *"="* ]] && [[ ${NEXTARG:0:1} != "-" ]]; then
				let "i += 1"
				VALUE=$NEXTARG
			fi
		fi
	else
		KEY=$(echo $ARGUMENT | cut -f1 -d=)
		VALUE=$(echo $ARGUMENT | cut -f2 -d=)	
	fi
	
	if [ $VERBOSE_LOGGING == "true" ]; then
		echo -e parsing argument [$GREEN$KEY$NC] with value [$YELLOW$VALUE$NC]
	fi

    case "$KEY" in
			a) ;&
            ARCHITECTURE)  	ARCHITECTURE=${VALUE} ;;
            CONFIG)  		CONFIG=${VALUE} ;;
			c) ;&
            CLEAN)  		CLEAN_CMAKE=${VALUE} 
							CLEAN_BUILD=${VALUE} ;;
            CLEAN_CMAKE)  	CLEAN_CMAKE=${VALUE} ;;
            CLEAN_BUILD)  	CLEAN_BUILD=${VALUE} ;;
			b) ;&
            BUILD_FOLDER) 	CUSTOM_ARGUMENTS=true
							BUILD_FOLDER=${VALUE} ;;
			o) ;&
            CMAKE_FOLDER) 	CUSTOM_ARGUMENTS=true
							CMAKE_FOLDER=${VALUE} ;;
            VULKAN_VERSION) CUSTOM_ARGUMENTS=true
							VULKAN_VERSION=${VALUE} ;;
            VULKAN_ROOT) 	CUSTOM_ARGUMENTS=true
							VULKAN_ROOT=${VALUE} ;;
			G) ;&
			GENERATOR)		GENERATOR=${VALUE} ;;
			C) ;&
			COMPILER)		COMPILER=${VALUE} ;;
			u) ;&
			update)	CMAKE_UPDATE=${VALUE} ;;
			VULKAN_STATIC) 	CUSTOM_ARGUMENTS=true
							VULKAN_STATIC=${value};;
            *)   echo -e ${PURPLE}WARNING${NC}: ignoring unrecognized argument: $ARGUMENT
    esac
done


# ---------------------------------------------------------------------------------------------------------------------
# validate the arguments
# ---------------------------------------------------------------------------------------------------------------------

case "$GENERATOR" in
	MSVC) if [ "$COMPILER" != "VC++" ] && [ "$COMPILER" != "CLang" ] && [ "$COMPILER" != "LLVM" ]; then 
			echo -e ${RED}ERROR${NC}: the [${GREEN}COMPILER${NC}] [${YELLOW}${COMPILER}${NC}] is incorrect. The [${GREEN}GENERATOR${NC}] [${YELLOW}${GENERATOR}${NC}] can only have one of the following compilers [${YELLOW}VC++${NC}], [${YELLOW}LLVM${NC}], or [${YELLOW}CLang${NC}]
			exit 1
		  fi;;
	Ninja) if [ "$COMPILER" != "CLang" ]; then 
			echo -e ${PURPLE}WARNING${NC}: the [${GREEN}COMPILER${NC}] [${YELLOW}${COMPILER}${NC}] is incorrect. Only [${YELLOW}CLang${NC}] is supported for the [${GREEN}GENERATOR${NC}] [${YELLOW}${GENERATOR}${NC}]. Setting the [${GREEN}COMPILER${NC}] to [${YELLOW}CLang${NC}] now.
			COMPILER="CLang"
		  fi;;
	Make)  if [ "$COMPILER" != "CLang" ]; then 
			echo -e ${PURPLE}WARNING${NC}: the [${GREEN}COMPILER${NC}] [${YELLOW}${COMPILER}${NC}] is incorrect. Only [${YELLOW}CLang${NC}] is supported for the [${GREEN}GENERATOR${NC}] [${YELLOW}${GENERATOR}${NC}]. Setting the [${GREEN}COMPILER${NC}] to [${YELLOW}CLang${NC}] now.
			COMPILER="CLang"
		  fi;;
	*)  echo -e ${RED}ERROR${NC}: did not recognize the [${GREEN}GENERATOR${NC}] [${YELLOW}${GENERATOR}${NC}], [${GREEN}GENERATOR${NC}] should be of the value [${YELLOW}MSVC${NC}],[${YELLOW}Ninja${NC}], or [${YELLOW}Make${NC}]
		exit 1
esac


if [ "$GENERATOR" == "MSVC" ] && [ "$ARCHITECTURE" != "ARM" ] && [ "$ARCHITECTURE" != "x64" ]; then
	echo -e incorrect [${GREEN}ARCHITECTURE${NC}] set, please either set it to [${YELLOW}ARM${NC}] or [${YELLOW}x64${NC}]
	exit 1
fi

if  [ "$GENERATOR" == "Unix" ] && [ "$ARCHITECTURE" != "x64" ]; then
	echo -e incorrect [${GREEN}ARCHITECTURE${NC}] set, only [${YELLOW}x64${NC}] supported
	exit 1
fi

if [ ! -z $CONFIG ]; then
	if [ "$CONFIG" != "Release" ] && [ "$CONFIG" != "Debug" ] && [ "$CONFIG" != "RelWithDebInfo" ] && [ "$CONFIG" != "MinSizeRel" ]; then
		echo -e ${RED}ERROR${NC}: incorrect [${GREEN}CONFIG${NC}] [${YELLOW}${CONFIG}${NC}] selected.
		echo -e "\texpected one of the following: [${YELLOW}Release${NC}], [${YELLOW}Debug${NC}], [${YELLOW}RelWithDebInfo${NC}], [${YELLOW}MinSizeRel${NC}]"
		exit 1
	fi
fi

declare cmake_output="${GENERATOR,,}/${ARCHITECTURE,,}"
declare build_output="${GENERATOR,,}/${ARCHITECTURE,,}"

declare CMAKE_PLATFORM=""
case "$GENERATOR" in
	MSVC) 
		  CMAKE_CXX=""
		  if [ "$COMPILER" == "CLang" ]; then
			CMAKE_PLATFORM=" -T v141_clang_c2 "
			cmake_output="${cmake_output}-CLang"
			build_output="${build_output}-CLang"
		  elif [ "$COMPILER" == "LLVM" ]; then
			CMAKE_PLATFORM=" -T LLVM-vs2017 "
			cmake_output="${cmake_output}-LLVM"
			build_output="${build_output}-LLVM"
		  else
		    CMAKE_PLATFORM=" -T v141 "
			cmake_output="${cmake_output}-VC++"
			build_output="${build_output}-VC++"
		  fi
		  
		  if [ $ARCHITECTURE == "x64" ]; then
			declare target_platform="Win64"
		  else
			declare target_platform="ARM"		  
		  fi
		  CMAKE_GENERATOR="Visual Studio 15 2017 $target_platform";;		  
	Ninja) 
		CLANG_PATH="${LLVM_PATH//\\//}"
		CMAKE_CXX="C:/Program Files/LLVM/bin/clang-cl.exe"		# DCMAKE_CXX_COMPILER
		CMAKE_C="C:/Program Files/LLVM/bin/clang-cl.exe"		# DCMAKE_C_COMPILER
		CMAKE_LINKER="C:/Program Files/LLVM/bin/lld-link.exe"	# DCMAKE_LINKER

        if [[ "$OSTYPE" == "linux-gnu" ]]; then
			export CC=/usr/bin/clang
            export CXX=/usr/bin/clang++
		elif [[ "$OSTYPE" == "darwin"* ]]; then
			export CC=/usr/bin/clang
            export CXX=/usr/bin/clang++
		elif [[ "$OSTYPE" == "cygwin" ]]; then
			export CC=C:/Program Files/LLVM/bin/clang-cl.exe
            export CXX=C:/Program Files/LLVM/bin/clang-cl.exe
		elif [[ "$OSTYPE" == "msys" ]]; then
			export CC=C:/Program Files/LLVM/bin/clang-cl.exe
            export CXX=C:/Program Files/LLVM/bin/clang-cl.exe
		elif [[ "$OSTYPE" == "win32" ]]; then
			export CC=C:/Program Files/LLVM/bin/clang-cl.exe
            export CXX=C:/Program Files/LLVM/bin/clang-cl.exe
		else
			echo -e ${RED}ERROR${NC}: unsupported platform detected.
			exit 1
		fi

		CMAKE_GENERATOR="Ninja";;
	Make) CMAKE_CXX=""
		CMAKE_GENERATOR="Unix Makefiles";;
esac

if [ $CMAKE_UPDATE == "true" ] && [ $CLEAN_CMAKE == "true" ]; then
	echo -e ${RED}ERROR${NC}: conflicting arguments: both cmake_clean and update are enabled, but only one can be enabled at a time.
	exit 1
fi

if [ $CMAKE_UPDATE == "true" ] && [ ! -f "$CMAKE_FOLDER/$cmake_output/CMakeCache.txt" ]; then
	echo -e "${RED}ERROR${NC}: tried to update an existing cmake project, but couldn't find a cache file present."
	exit 1
fi

if [ $CMAKE_UPDATE == "false" ] && [ -z $CONFIG ]; then
	if [ -z $VULKAN_ROOT ]; then
		if [[ "$OSTYPE" == "linux-gnu" ]]; then
			VULKAN_ROOT=/VulkanSDK/
		elif [[ "$OSTYPE" == "darwin"* ]]; then
			VULKAN_ROOT=/VulkanSDK/
		elif [[ "$OSTYPE" == "cygwin" ]]; then
			VULKAN_ROOT=C:/VulkanSDK/
		elif [[ "$OSTYPE" == "msys" ]]; then
			VULKAN_ROOT=C:/VulkanSDK/
		elif [[ "$OSTYPE" == "win32" ]]; then
			VULKAN_ROOT=C:/VulkanSDK/
		else
			echo -e ${RED}ERROR${NC}: unsupported platform detected.
			exit 1
		fi
		echo the vulkan SDK root has been set to $VULKAN_ROOT
		if [ ! -d "$VULKAN_ROOT" ]; then
			echo -e ${RED}ERROR${NC}: the automatic search for the vulkan SDK yielded no results, please pass in the value using the [${GREEN}VULKAN_ROOT${NC}] argument.
			exit 1
		fi
	fi

	echo checking if the correct vulkan sdk version is installed..
	if [ ! -d "$VULKAN_ROOT/$VULKAN_VERSION" ]; then
		echo -e ${RED}ERROR${NC}: could not find the vulkan SDK version [${GREEN}VULKAN_VERSION${NC}] [${YELLOW}${VULKAN_VERSION}${NC}], please install it to continue or choose a different version.
		exit 1
	fi
fi

# ---------------------------------------------------------------------------------------------------------------------
# generate the cmake project
# ---------------------------------------------------------------------------------------------------------------------


mkdir -p $CMAKE_FOLDER/$cmake_output
mkdir -p $BUILD_FOLDER/$build_output
if [ $CLEAN_BUILD == "true" ]; then
	echo cleaning $BUILD_FOLDER/$build_output
	rm -rf $BUILD_FOLDER/$build_output/*
fi

if [ $CLEAN_CMAKE == "true" ]; then
	echo cleaning $CMAKE_FOLDER/$cmake_output
	rm -rf $CMAKE_FOLDER/$cmake_output/*
fi

cd $CMAKE_FOLDER/$cmake_output

if [ $CMAKE_UPDATE == "true" ]; then
	echo reconfiguring project files for "$CMAKE_GENERATOR"
	cmake .
elif [[ ! -d "CMakeFiles" ]]; then
	echo generating project files for "$CMAKE_GENERATOR"
	echo setting build directory to "$BUILD_FOLDER/$build_output"
	cmake -G "$CMAKE_GENERATOR" "$ROOT_DIRECTORY" -DBUILD_DIRECTORY="$BUILD_FOLDER/$build_output" -DVULKAN_VERSION=$VULKAN_VERSION -DVULKAN_ROOT=$VULKAN_ROOT $CMAKE_PLATFORM -DVULKAN_STATIC=$VULKAN_STATIC
fi

if [ $CUSTOM_ARGUMENTS == "true" ] && [ $CMAKE_UPDATE == "true" ]; then
	echo -e "${PURPLE}WARNING${NC}: custom arguments were sent, but there was already a cmake project in the location, ignoring the passed arguments."
	echo -e "$if this was unintended then you should pass the CLEAN/CLEAN_CMAKE command to regenerate the project with the new arguments"
fi


# ---------------------------------------------------------------------------------------------------------------------
# build the project
# ---------------------------------------------------------------------------------------------------------------------

if [ ! -z $CONFIG ]; then
	echo building project with configuration $CONFIG
	cmake --build . --config $CONFIG
fi

echo -e "\n${GREEN}completed building the project${NC}"
