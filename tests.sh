#!/bin/bash
set -e

command -v docker >/dev/null 2>&1 || { echo >&2 "I require docker but it's not installed.  Aborting."; exit 1;}


cd docker
docker build windows-vcpp -t windows-vcpp
cd ..

echo "booting up windows-vcpp image..."
docker run --rm --env EXTRA_CMAKE_PARAMS="-DPE_MODE=LIB" --platform=windows --mount type=bind,source="$PWD",destination=c:\\paradigm_local_git,readonly --name=temp windows-vcpp

exit 0

#if [[ "$(docker images -q ubuntu-clang-6 2> /dev/null)" == "" ]]; then
	cd docker
	docker build ubuntu-clang-6 -t ubuntu-clang-6
	cd ..
#fi

echo "booting up ubuntu-clang-6 image..."
docker run --rm --env EXTRA_CMAKE_PARAMS="-DPE_MODE=LIB" --platform=linux --mount type=bind,source="$PWD",destination=/paradigm_local_git/,readonly --name=temp ubuntu-clang-6

exit 0
