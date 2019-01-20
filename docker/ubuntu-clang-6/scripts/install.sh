#!/bin/bash

git clone /paradigm_local_git paradigm && 
cd paradigm && 
chmod +x build.sh &&
./build.sh -VK_VERSION ${VK_VERSION} -COMPILER CLang -G Make -VK_ROOT $VULKAN_ROOT_DIR -cmake_params "${EXTRA_CMAKE_PARAMS} -DCMAKE_BUILD_TYPE=$BUILD_TYPE" &&
cd project_files/make/x64/ &&
make -j${CORE_COUNT} &&
ctest .