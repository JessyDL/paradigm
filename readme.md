

![](https://img.shields.io/badge/language-c%2B%2B20-blue.svg?longCache=true&style=for-the-badge) ![](https://img.shields.io/badge/vulkan-1.2-red.svg?longCache=true&style=for-the-badge) ![](https://img.shields.io/badge/GLES-3.1--3.2-green.svg?longCache=true&style=for-the-badge)  [![last git tag](https://img.shields.io/github/tag/JessyDL/paradigm.svg?style=for-the-badge&colorB=6e42ce)](https://github.com/JessyDL/paradigm/tree/0.1.1)

# Paradigm Engine
Paradigm Engine is a Vulkan first modern graphics rendering library written in C++20 with support for GLES (3.2). It concerns itself mostly with the heavy lifting of the rendering part of the engine, and supporting a toolchain that is flexible to your project's needs and structure. It stays away from dictating your code design, forcing you to use inheritence or the like for your logic, stays away from macro usage (unless in very rare instances), avoids globals, and provides simple bindings for you to implement your language of choice.

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
| Architecture| Status        | Vulkan| GLeS| Metal|
:-------------|:-------------|:---:|:---:|:---:
| ![](https://img.shields.io/badge/x64-Windows-blue.svg?style=for-the-badge)| ![GitHub Workflow Status](https://img.shields.io/github/workflow/status/JessyDL/Paradigm/windows?label=%20&style=for-the-badge)|  1.2 | 3.1 - 3.2| - |
| ![](https://img.shields.io/badge/x64-GNU/Linux-blue.svg?style=for-the-badge)| ![GitHub Workflow Status](https://img.shields.io/github/workflow/status/JessyDL/Paradigm/ubuntu?label=%20&style=for-the-badge) |  1.2 | 3.2*| - |
| ![](https://img.shields.io/badge/x64-OSx-blue.svg?style=for-the-badge)|  ![](https://img.shields.io/badge/-to_be_designed-lightgrey.svg?style=for-the-badge) | -| -| - |
| ![](https://img.shields.io/badge/x64-iOS-blue.svg?style=for-the-badge)| ![](https://img.shields.io/badge/-to_be_designed-lightgrey.svg?style=for-the-badge) |  -| -| - |
| ![](https://img.shields.io/badge/ARMv7-Android-blue.svg?style=for-the-badge)| ![](https://img.shields.io/badge/-in_development-purple.svg?style=for-the-badge)| 1.2| 3.1 - 3.2| - |
| ![](https://img.shields.io/badge/ARM64-Android-blue.svg?style=for-the-badge)|![GitHub Workflow Status](https://img.shields.io/github/workflow/status/JessyDL/Paradigm/android?label=%20&style=for-the-badge)| 1.2| 3.1 - 3.2| - |

*GLES on GNU/Linux requires `libegl1-mesa-dev` and `libgles2-mesa-dev` to be installed.
## Building
### Prerequisites
#### All platforms
Python 3.9 or newer

[CMake ]( http://cmake.org/) 3.22 or higher is required on all platforms.

A C++23 compliant compiler. See the Github Actions for up-to-date examples of supported compilers on different platforms.

#### Android
Android SDK
Gradle (7.6 or later)

### Creating the project files
You can build this project by either using the provided `CMakePresets.json` file (recommended), or by running `paradigm.py` with python 3.9+. See the [`CMakePresets` section](#cmakepresets) for more information.

So far MSVC (2022), CLang (12.0.0) with libc++, and GCC (12.1.0) are supported. The project will *likely* incorrectly generate for other compilers/setups. You can always verify the compiler version that was used by checking the github CI runners.

If lost, check the github actions workflow folder to see ideal platform setups.

#### CMakePresets
Various CMakePresets exists, they are specified to working combinations of graphics API's that are supported for the given platform. They can be used as normal, or used as guides for custom setups.

The cmake presets are structured in the following way `{platform}-{type}-{graphics}`, see next section for more information. Following are the valid settings for each.

platform:
 - windows
 - gnu-linux
 - android

type:
 - debug
 - release

graphics:
 - vulkan
 - all (imples both vulkan and gles support).

#### paradigm.py
The paradigm script is a helper script that can invoke, amongst others, the builder script (tools/build.py). Invoke the builder script using `--run build`, this will set everything up quick and easy. It will generate a solution in the `/project_file/{generator}/{architecture}/` folder by default, and when building it will output to `/builds/{generator}/{architecture}/`.
You can tweak various settings and values, you'll find them at the top of the `tools/build.py` file.

As an example running `py paradigm.py --run build --graphics vulkan gles --generator Ninja` will set up the project as an executable, with all available graphics backends, using the Ninja build tool.

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
Version information can be found in the `paradigm.hpp` in the `core/inc` directory (note this file is generated by `tools/generate_header_info.py` which auto runs every build). This project uses versioning of the format MAJOR.MINOR.PATCH.{GIT SHA1}. Release type can be one of the following:
- i - internal
- r - release

release versions are determined if the SHA1 is pointing to a tagged commit, otherwise it's considered an internal build.

##  External Libraries
The following external libraries may be used in one, or many of the sub projects. The specific project readme's will describe their dependencies in more detail so you can always verify which project uses what.
- `GLI` image loading and saving
- `GLM` mathematics library used only by `GLI`
- `litmus` used for running tests
- `spdlog` used for logging
- `fmt` modern formatting library for C++
- `utfcpp` utf8 string parsing
- `strype` support for stringifying typenames and enums on supported compilers

note that dependencies might pull in further dependencies that are not listed here.

### Conditional dependencies
The following dependencies are conditionally included:
- `Vulkan-hpp` when enabling the Vulkan backend `Vulkan-hpp` is used to generated C++ like headers for Vulkan
- `GLAD` when enabling the GL backend `GLAD` is used to generate the GL headers
- `google/benchmark` when enabling `PE_BENCHMARKS`

# Documentation
A reference documentation is available at [https://paradigmengine.github.io/](https://paradigmengine.github.io/).
API examples, tutorials, and best practices guide will be written at a later time. For the time being, you can look at the example projects provided in the examples section.

You can also find further documentation in the `docs` directory, such as information on how to use the [Entity Component System](https://github.com/JessyDL/paradigm/blob/develop/docs/ecs.md).
# Tests
### building
Tests are on by default when compiling the project as a library, you can disable this by toggline `PE_MODE` in `cmake` from `LIB` to `LIB_NO_TESTS`. Tests will not be included when building the project in `EXE` mode.

When using CMake directly (or with CMakePresets), you'll have the project files output in `project_files`, from there out you can either boot it up in your editor of choice, or if your generator isn't also an IDE, then build it from there. Build outputs by default will appear in `/builds/`.

When using the `paradigm.py` or `tools/build.py` script, you can set this value like this `--cmake_params="-DPE_MODE=LIB"`. `LIB` is the default value for this project.

Tests use Google Benchmark, these will be fetched automatically when the CMake script detects testing to be true.

# Future
### Language
This project will keep up with the latest C++ language improvements till atleast C++2a, as we require some of the newer features to create safer and easier to reason about interfaces for, amongst other things, serialization. After C++2a we will freeze the language to that version and keep it stable.
### Graphics Backends
After fully completing support for GLES 3.1+ and Vulkan, and a graphical toolset for editing has been finished, focus will be shifted on a Metal backend.
A WebGPU backend might be looked into to support more platforms as well if it has reached MVP/1.0 status.
# Extras
### Build as executable
If you wish to build the engine, not as a library, but instead as an executable, you can enable this behaviour by passing `-DPE_MODE=EXE` as a `--cmake_param` in the `build.py` script, or directly in your cmake invocation. This will set the `CORE_EXECUTABLE` define in the compiler, and will trigger the `core` project to be built as an executable instead of being a library.

### Benchmarks
Rudimentary benchmarks (heavily WIP) have been added to the project, you can enable this by setting the cmake value `PE_MODE` to *anything but* `EXE`, and toggling on `PE_BENCHMARKS`.

# License
This project is dual-licensed under commercial and open source licenses. Licensed under GNU AGPLv3 for free usage, and licensed under a commercial license you can purchase or request for commercial usage.

-  GNU AGPLv3: You may use Paradigm Engine as a Free Open Source Software as outlined in the terms found here: [https://www.gnu.org/licenses/agpl-3.0.en.html](https://www.gnu.org/licenses/agpl-3.0.en.html)
    
-  Commercial license: If you do not want to disclose the source of your application you have the option to purchase a commercial license. The commercial license gives you the full rights to create and distribute software on your own terms without any open source license obligations. For more information, please contact [Jessy De Lannoit directly here](https://www.linkedin.com/in/jessydelannoit/).

This license (and its setup) only applies to the current major version of this software. So if you are getting the license on version 1.0.0, you will be able to use all upgrades that carry the same major version number (in this case 1.x.x). Once the switch is made to a newer version such as 2.x.x of the project, you will need to upgrade your license to use that major version's updated license (in case there is a new license), or the updated license setup. 

Of course you may continue to use any version of this project you currently have licensed/have a license for, without needing to change your license as long as you do not pull new updates from major versions you do not have a license for.

This license, and its setup applies to all sub-projects in this repository unless explicitly stated otherwise.
