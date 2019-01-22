git clone --depth 1 file:////paradigm_local_git paradigm;
cd paradigm;
.\build.ps1 -VK_VERSION ${VK_VERSION} -cmake_params "${EXTRA_CMAKE_PARAMS} -DCMAKE_BUILD_TYPE=$BUILD_TYPE";
cd project_files/msvc/x64-VC++/;
cmake --build . --config $BUILD_TYPE -j ${CORE_COUNT};
ctest .;