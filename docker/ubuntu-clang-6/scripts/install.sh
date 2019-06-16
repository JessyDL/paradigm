#!/bin/bash

opt=(--vk_version ${VK_VERSION})
opt+=(--compiler CLang)
opt+=(--generator Make)
opt+=(--verbose)
opt+=(--vk_root $VULKAN_ROOT_DIR)
opt+=(--cmake_params="${EXTRA_CMAKE_PARAMS}")
opt+=(--build_config $BUILD_TYPE)

cd paradigm && 
chmod +x build.py &&
python3 build.py "${opt[@]}" &&
cd project_files/make/x64/ &&
make -j${CORE_COUNT} &&
ctest .