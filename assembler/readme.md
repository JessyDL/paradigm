# Assembler
The `assembler` project is a Command Line Interface tool to manipulate and generate various data structures that the engine can consume. Using `assembler` you can compile `shaders` that generates their companion [`core::meta::shaders`](https://jessydl.github.io/paradigm/classcore_1_1meta_1_1shader.html), import models using  [assimp v4.1.0](https://github.com/assimp/assimp/releases/tag/v4.1.0/), setup a [meta::library](https://jessydl.github.io/paradigm/classmeta_1_1library.html), generate [meta::file](https://jessydl.github.io/paradigm/classmeta_1_1file.html)s, and more. 

## Prerequisites
[CMake ]( http://cmake.org/) 3.21 or higher is required on all platforms.

Transitive requirements from dependencies (particularly `paradigm`'s dependencies), but in addition the following are required:
- Assimp
- GLSLang
- SPIRV-Cross

These dependencies will be automatically downloaded and built when building `assembler` through CMake.

## Running
After building the project, it should be automatically set up to run. When executing you should be greeted with a screen similar to this (after executing the `--help` command).
<img src="https://raw.githubusercontent.com/JessyDL/paradigm/master/assembler/output.png " width=600>
## Commands
`assembler` can be used in 2 modes, either directly by starting up the binary, or by invoking it from another command line tool and sending it arguments. When unsure what to do, write `--help` or `-h`, and information will be printed on the current actions that can be done. To quit, write `--quit` or `-q`.

When invoking from a script, or through another command line tool or terminal, and you want to send multiple parameters, use the ` | ` symbol to pipe together commands.

Be sure not to forget to send the `--quit` command when executing from a script or terminal. The default mode is `interactive` and so it will check for `std::cin` while it has not received a clear quit command.
## Dependencies
There are also various external dependencies used, the following is the list of all direct external dependencies `assembler` has. It is assumed the dependency version is the latest available unless explicitly stated.
Many of these dependencies also pull in more dependencies. Verify on the project pages directly what these are.
- [assimp v6.0.2](https://github.com/assimp/assimp)
- [glslang](https://github.com/KhronosGroup/glslang) (version dependent on the vulkan SDK version used)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) (version dependent on the vulkan SDK version used)

# License
The `assembler` project, and all code below this file's tree is licensed under GNU AGPLv3. As long as attribution is added of what the original source is, and a link to this repository (https://github.com/JessyDL/assembler).
