

![](https://img.shields.io/badge/language-c%2B%2B17-blue.svg?longCache=true&style=for-the-badge) ![](https://img.shields.io/badge/vulkan-1.1.82.1-red.svg?longCache=true&style=for-the-badge) ![](https://img.shields.io/badge/GLES-3.2-green.svg?longCache=true&style=for-the-badge)  [![last git tag](https://img.shields.io/github/tag/JessyDL/paradigm.svg?style=for-the-badge&colorB=6e42ce)](https://github.com/JessyDL/paradigm/tree/0.1.1)

# Paradigm Engine
Paradigm Engine is a Vulkan first modern graphics rendering engine written in C++17 with support for GLES (3.2). It concerns itself mostly with the heavy lifting of the rendering part of the engine, and supporting a toolchain that is flexible to your project needs and structure. It stays away from dictating your code design, forcing you to use inheritence or the like for your logic, stays away from macro usage (unless in very rare instances), avoids globals, and provides simple bindings for you to implement your language of choice. C# bindings will be provided; as well as an example projects showcasing simple gameplay.

Some of the key reasons why this engine exists:
- Showcasing an approach to serialization in C++ that does not rely on external compilers or macro magic.
- Building a renderer that is fully integrated with a multithreaded ECS.
- A focus on small binary size, for ease of distribution in a team.
- Trying out new rendering engine designs that are more suited, and enabled by, modern graphics API's.
- Flexible testbed for those who want to quickly prototype/validate something.
- A trivial binding point so you can insert the language of your choice for your logic/extensions.
- Minimize globals and service locator patterns that are present in many engines out there and instead use dependency injection.
-  <sub><sup>And ofcourse brush up my skills in writing systems and architectures I'm not familiar with, and give myself new challenges in other problem domains. :D </sub></sup>

For more detailed description about the engine itself, go to the readme of the `core` project, which is in the folder of the same name. Similarly all sub-projects that make up the engine all have their own readme's describing their intent, structure, status and dependencies. This readme is just to give a larger oversight over the entire project.
## Status
| Architecture| Status        | Unit Tests | Backend|
| :-------------|:-------------| -----:|---:|
| ![](https://img.shields.io/badge/x64-Windows-blue.svg?style=for-the-badge)| [![Travis (.com) branch](https://img.shields.io/travis/com/JessyDL/paradigm.svg?logo=travis&style=for-the-badge)](https://travis-ci.com/JessyDL/paradigm) | [![Travis (.com) branch](https://img.shields.io/travis/com/JessyDL/paradigm.svg?label=tests&style=for-the-badge)](https://travis-ci.com/JessyDL/paradigm) | Vulkan/GLES |
| ![](https://img.shields.io/badge/x64-Unix-blue.svg?style=for-the-badge)|  [![Travis (.com) branch](https://img.shields.io/travis/com/JessyDL/paradigm.svg?logo=travis&style=for-the-badge)](https://travis-ci.com/JessyDL/paradigm) | [![Travis (.com) branch](https://img.shields.io/travis/com/JessyDL/paradigm.svg?label=tests&style=for-the-badge)](https://travis-ci.com/JessyDL/paradigm)| Vulkan/GLES |
| ![](https://img.shields.io/badge/x64-OSx-blue.svg?style=for-the-badge)|  ![](https://img.shields.io/badge/-to_be_designed-lightgrey.svg?style=for-the-badge) |    - | MoltenVK/GLES|
| ![](https://img.shields.io/badge/x64-iOS-blue.svg?style=for-the-badge)| ![](https://img.shields.io/badge/-to_be_designed-lightgrey.svg?style=for-the-badge) |    - | MoltenVK/GLES|
| ![](https://img.shields.io/badge/ARMv7-Android-blue.svg?style=for-the-badge)| ![](https://img.shields.io/badge/status-in_development-purple.svg?style=for-the-badge)|   - | Vulkan/GLES|
| ![](https://img.shields.io/badge/ARM64-Android-blue.svg?style=for-the-badge)|  ![](https://img.shields.io/badge/deploy-success-green.svg?style=for-the-badge)|   - | Vulkan/GLES|
## Building
### Prerequisites
To use `assembler` on Windows, Bash on Ubuntu on Windows should be installed.
[CMake ]( http://cmake.org/) 3.11 or higher is required on all platforms.

### Creating the project files
You can build the libraries using the provided build.sh file that can be invoked, or by invoking cmake directly. The build.py just helps you to set up a workspace and sets some critical defines that will be used in the build process.

So far only MSVC (2019), and CLang (6.0.0) with LLVM 7 - 8 and libc++ are supported. The project will likely incorrectly generate for other compilers/setups.

If lost, the docker folder contains a setup environment for both linux (ubuntu), as well as windows. You can see all dependencies your platform needs right there.

#### build.py
The build script is a helper script to set everything up quick and easy. It will generate a solution in the `/project_file/{generator}/{architecture}/` folder by default, and when building it will output to `/builds/{generator}/{architecture}/`.
You can tweak various settings and values, you'll find them at the top of the `build.py` file.
#### cmake
The less easy way, but perhaps better and easier to integrate. The values that are required to be set can be seen in the cmake invocation in build.py, but will be repeated here:
-DBUILD_DIRECTORY="path/to/where/to/build/to" (not to be confused with where the project files will be)
-DVULKAN_ROOT: path to install location of the vulkan SDK (excluding the version part).
-DVULKAN_VERSION: string based value that is a 1-1 match with an installed vulkan SDK instance.

## Examples
Following are some examples that showcase various usages of this project, click the image to go to the repository.

**[basic-app](https://github.com/JessyDL/paradigm-example-app)**

<a href="https://github.com/JessyDL/paradigm-example-app"><img src="https://raw.githubusercontent.com/JessyDL/paradigm-example-app/master/example_app_01.png" height=150></a> 

Basic example app showcasing how to create a surface, loading a meta::library, create memory::region's and a resource::cache, and finally rendering a basic textured box.

**[assembler](https://github.com/JessyDL/assembler)**

<a href="https://github.com/JessyDL/assembler"><img src="https://raw.githubusercontent.com/JessyDL/assembler/master/output.png" height=150></a> 

The assembler project is a CLI toolchain that allows you to generate data files that can be used by the engine, as well as generating resource libraries. Its setup is a bit different due to the fact that it consumes the library mostly as a source. 

## Project Structure
### Sub-Projects
All the sub-project's code resides in the root in a folder that shares the same name. Each one of them contains both the src and inc folder as well as a readme that describes that specific project's intended goals.

### Tools
For tools that can aid in generating shaders, importing models, and generating resource libraries, check out the [assembler repository](https://github.com/JessyDL/assembler).

### Versioning
Version information can be found in the `project_info.h` in every `inc` folder. This project uses versioning of the format MAJOR.MINOR.PATCH-{Release Type}{Commit SHA}. Release type can be one of the following:
- i - internal
- r - release

When the release type is missing, it is considered a debug/test build.
There never can be a release type on the same version that is generated from a differing source. This means we also do not allow incremental releases on the same version (e.g. r1, r2, r3, ...). Debug/test builds are exempt from this rule.

You can find internal and (public) release builds in the tags on this repo.

##  External Libraries
The following external libraries may be used in one, or many of the sub projects. The specific project readme's will describe their dependencies in more detail so you can always verify which project uses what.
- `GLI` image loading and saving
- `GLM` mathematics library used only by `GLI`
- `volk` dynamic bindings for vulkan
- `Vulkan-hpp` generated C++ like headers for Vulkan
- `Catch2` when compiling the tests, Catch2 will be pulled in.
- `spdlog` used for logging
- `fmt` modern formatting library for C++

# Documentation
A reference documentation is available at [https://jessydl.github.io/paradigm/](https://jessydl.github.io/paradigm/).
API examples, tutorials, and best practices guide will be written at a later time. For the time being, you can look at the example projects provided in the examples section.

You can also find further documentation in the `docs` directory, such as information on how to use the [Entity Component System](https://github.com/JessyDL/paradigm/blob/develop/docs/ecs.md).
# Tests
### building
Tests are on by default when compiling the project as a library, you can disable this by toggline `PE_MODE` in `cmake` from `LIB` to `LIB_NO_TESTS`. Tests will not be included when building the project in `EXE` mode.

When using the `build.py` script, you can set this value like this `--cmake_params "-DPE_MODE=LIB"`. `LIB` is the default value for this project.

Tests use Catch2 v2.4.0, these will be fetched automatically when the CMake script detects testing to be true.

# Future
### Language
This project will keep up with the latest C++ language improvements till atleast C++2a, as we require some of the newer features to create safer and easier to reason about interfaces for, amongst other things, serialization. After C++2a we will freeze the language to that version and keep it stable.
### Graphics Backends
After we have both the editor project and the rendering engine going, we will look towards supporting all available Vulkan platforms first, and then after implement GLeS 3.0+. Don't expect this to happen anytime soon though, a long way from there still.

# Extras
### Statically bind Vulkan
If you wish to statically bind vulkan, you can use the flag `--vk_static` in the `build.py` script, or alternatively invoke `cmake` directly with the `-DVK_STATIC=true` flag. 
Note that depending on the platform, static binding is impossible (like Android).
### Build as executable
If you wish to build the engine, not as a library, but instead as an executable, you can enable this behaviour by passing `-DPE_MODE=EXE` as a `--cmake_param` in the `build.py` script, or directly in your cmake invocation. This will set the `CORE_EXECUTABLE` define in the compiler, and will trigger the `core` project to be built as an executable instead of being a library.

### Benchmarks
Rudimentary benchmarks (heavily WIP) have been added to the project, you can enable this by setting the cmake value `PE_MODE` to *anything but* `EXE`, and toggling on `PE_BENCHMARKS`.

# License
This project is dual-licensed under commercial and open source licenses. Licensed under GNU AGPLv3 for free usage, and licensed under a commercial license you can purchase or request for commercial usage.

-  GNU AGPLv3: You may use Paradigm Engine as a Free Open Source Software as outlined in the terms found here: [https://www.gnu.org/licenses/agpl-3.0.en.html](https://www.gnu.org/licenses/agpl-3.0.en.html)
    
-  Commercial license: If you do not want to disclose the source of your application you have the option to purchase a commercial license. The commercial license gives you the full rights to create and distribute software on your own terms without any open source license obligations. For more information, please contact me directly.

This license (and its setup) only applies to the current major version of this software. So if you are getting the license on version 1.0.0, you will be able to use all upgrades that carry the same major version number (in this case 1.x.x). Once the switch is made to a newer version such as 2.x.x of the project, you will need to upgrade your license to use that major version's updated license (in case there is a new license), or the updated license setup. 

Of course you may continue to use any version of this project you currently have licensed/have a license for, without needing to change your license as long as you do not pull new updates from major versions you do not have a license for.

This license, and its setup applies to all sub-projects in this repository unless explicitly stated otherwise.
