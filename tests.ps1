cd docker
docker build windows-vcpp -t windows-vcpp
cd ..

echo "booting up windows-vcpp image..."
docker run --rm -it --env EXTRA_CMAKE_PARAMS="-DPE_MODE=LIB" --platform=windows --mount type=bind,source="$(get-location)",destination=C:/paradigm_local_git,readonly --name=temp windows-vcpp

exit 0

#if [[ "$(docker images -q ubuntu-clang-6 2> /dev/null)" == "" ]]; then
	cd docker
	docker build ubuntu-clang-6 -t ubuntu-clang-6
	cd ..
#fi

echo "booting up ubuntu-clang-6 image..."
docker run --rm --env EXTRA_CMAKE_PARAMS="-DPE_MODE=LIB" --platform=linux --mount type=bind,source="$(get-location)",destination=/paradigm_local_git/,readonly --name=temp ubuntu-clang-6

exit 0
