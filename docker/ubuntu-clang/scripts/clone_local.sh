#!/bin/bash

rm -rf /paradigm/ &&
mkdir paradigm &&
git clone --depth 1 file:////paradigm_local_git paradigm &&
cd paradigm && 
chmod +x build.py &&
cd ..