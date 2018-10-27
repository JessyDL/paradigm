
![](https://img.shields.io/badge/language-c%2B%2B17-blue.svg?longCache=true&style=for-the-badge) ![](https://img.shields.io/badge/using-vulkan%201.1-red.svg?longCache=true&style=for-the-badge) ![](https://img.shields.io/badge/status-passed%20all%20tests-green.svg?longCache=true&style=for-the-badge) ![](https://img.shields.io/badge/version-0.1.0-lightgrey.svg?longCache=true&style=for-the-badge) 
# Paradigm Engine
Paradigm Engine is a Vulkan first modern graphics rendering engine written in C++17. It concerns itself mostly with the heavy lifting of the rendering part of the engine, and supporting a toolchain that is flexible to your project needs and structure. It stays away from dictating your code design, forcing you to use inheritence or the like for your logic, stays away from macro usage (unless in very rare instances), avoids globals, and provides simple bindings for you to implement your language of choice. C# bindings will be provided as well as example projects showcasing simple gameplay.

Some of the key reasons why this engine exists:
- Showcasing an approach to serialization in C++ that does not rely on external compilers or macro magic.
- Stepping away from the heavy handedness that most engines demand in the structure of your project, and the structure of your code. 
- Small binary so that the engine can be part of the source history, making it easier to keep large teams on the correct version.
- Stepping away from common old rendering engine designs that are still widely used but that are no longer required for modern graphics API's and bring needless constraints on the end project.
- Flexible testbed for those who want to quickly prototype/validate something.
- A trivial binding point so you can insert the language of your choice for your logic/extensions.
- Minimize globals and service locator patterns that plague many engines out there and instead use dependency injection.
-  <sub><sup>And ofcourse brush up my skills in writing systems and architectures I'm not familiar with, and give myself new challenges in other problem domains. :D </sub></sup>

For more detailed description about the engine itself, go to the readme of the `core` project, which is in the folder of the same name. Similarly all sub-projects that make up the engine all have their own readme's describing their intent, structure, status and dependencies. This readme is just to give a larger oversight over the entire project.
# Status
| Architecture| Status        | Size | Backend|
| :-------------|:-------------| -----:|---:|
| ![](https://img.shields.io/badge/x64-Windows-blue.svg?style=for-the-badge)| ![](https://img.shields.io/badge/deploy-success-green.svg?style=for-the-badge)| 1.2mb | Vulkan |
| ![](https://img.shields.io/badge/x64-Unix-blue.svg?style=for-the-badge)|  ![](https://img.shields.io/badge/deploy-success-green.svg?style=for-the-badge)| -| Vulkan |
| ![](https://img.shields.io/badge/x64-OSx-blue.svg?style=for-the-badge)|  ![](https://img.shields.io/badge/-to_be_designed-lightgrey.svg?style=for-the-badge) |    - | MoltenVK|
| ![](https://img.shields.io/badge/x64-iOS-blue.svg?style=for-the-badge)| ![](https://img.shields.io/badge/-to_be_designed-lightgrey.svg?style=for-the-badge) |    - | MoltenVK|
| ![](https://img.shields.io/badge/ARMv7-Android-blue.svg?style=for-the-badge)| ![](https://img.shields.io/badge/-to_be_designed-lightgrey.svg?style=for-the-badge)|   - | Vulkan|
| ![](https://img.shields.io/badge/ARM64-Android-blue.svg?style=for-the-badge)|  ![](https://img.shields.io/badge/deploy-success-green.svg?style=for-the-badge)|   - | Vulkan|
# Building
Currently the only way to get the project is by building from source, when the editor finally starts to be publically deployed, premade binaries will be released. 

### CMake
CMake 3.11 is required (FetchContent is used for modules). Look at build.sh for information.

# Project Structure
## Sub-Projects
All the sub-project's code resides in the root in a folder that shares the same name. Each one of them contains both the src and inc folder as well as a readme that describes that specific project.

There is also a tools folder, this contains both the code for the tools (like assembler project) as well as several handy tools that you will be relying on precompiled for you.

## Versioning
Version information can be found in the `project_info.h` in every ``inc`` folder. This project uses Semantic Versioning of the format MAJOR.MINOR.PATCH-{Release Type}{Commit SHA}. Release type can be one of the following
- i - internal
- r - release
There can not be a release type on the same version that is generated from a differing source. This means we also do not allow incremental releases on the same version (e.g. r1, r2, r3, ...).

## Miscelaneous
The data/library folder is there only as example files, you are free to remove them when not needed.

# External Libraries
The following external libraries may be used in one, or many of the sub projects. The specific project readme's will describe their dependencies in more detail so you can always verify which project uses what.
### GLM
GLM is a mathematics library. We use this extensively in the `core` project.
### GLI
GLI is a small library that is responsible for texture format loading. Similar to GLM, we use this extensively in the `core` project.
### Assimp
Assimp is an extensive library that can load various models as well as entire scenes. Due to its scope it is not part of  `core` and instead is part of the  `assembler` project where it can generate models streams that the  `core` project can consume, both in string as well as binary format.
### GLSLang
Specifically the GLSLangValidator provides the shader compiler for the engine. It is not included in the  `core` project, but instead used in  `assembler` to generate the SPIR-V that is loaded during runtime. Additionally  `assembler` generates the metadata required for the core project to understand your shader. Please see the readme of  `assembler` for more info. 
### Catch2
This is our unit testing library for our projects.
### Vulkan-hpp
We use the generated hpp files from the Khronos group to get somewhat sensible C++ like code.

# Future
### Language
This project will keep up with the latest C++ language improvements till atleast C++2a, as we require some of the newer features to create safer and easier to reason about interfaces for, amongst other things, serialization. After C++2a we will freeze the language to that version and keep it stable.
### Graphics Backends
After we have both the editor project and the rendering engine going, we will look towards supporting all available Vulkan platforms first, and then after implement GLeS 3.0. Don't expect this to happen anytime soon though, a long way from there still.
### Architectures
We are planning on supporting Unix and Android in the near future, followed by MacOS and iOS afterwards with the MoltenVK graphics backend.

# License

This project is dual-licensed under commercial and open source licenses. Licensed under GNU AGPLv3 for free usage, and licensed under a commercial license you can purchase or request for commercial usage.

-  GNU AGPLv3: You may use Paradigm Engine as a Free Open Source Software as outlined in the terms found here: [https://www.gnu.org/licenses/agpl-3.0.en.html](https://www.gnu.org/licenses/agpl-3.0.en.html)
    
-  Commercial license: If you do not want to disclose the source of your application you have the option to purchase a commercial license. The commercial license gives you the full rights to create and distribute software on your own terms without any open source license obligations. For more information, please contact me directly.

This license (and its setup) only applies to the current major version of this software. So if you are getting the license on version 1.0.0, you will be able to use all upgrades that carry the same major version number (in this case 1.x.x). Once the switch is made to a newer version such as 2.x.x of the project, you will need to upgrade your license to use that major version's updated license (in case there is a new license), or the updated license setup. 

Of course you may continue to use any version of this project you currently have licensed/have a license for, without needing to change your license as long as you do not pull new updates from major versions you do not have a license for.

This license, and its setup applies to all sub-projects in this repository unless explicitly stated otherwise.
