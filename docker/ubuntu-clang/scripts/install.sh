#!/bin/bash

cd paradigm && 
chmod +x build.py &&
python build.py "$@" &&
cd project_files/*/x64/ &&
make -j${CORE_COUNT} &&
ctest .