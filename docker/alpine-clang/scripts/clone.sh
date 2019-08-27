#!/bin/bash

rm -rf /paradigm/ &&
mkdir paradigm &&
git clone --depth 1 https://github.com/JessyDL/paradigm.git paradigm &&
cd paradigm && 
chmod +x build.py &&
cd ..