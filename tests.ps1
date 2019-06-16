cd docker
docker build ubuntu-clang-6 -t ubuntu-clang-6
cd ..

echo "booting up ubuntu-clang-6 image..."
docker run --rm --env EXTRA_CMAKE_PARAMS="-DPE_MODE=LIB" --platform=linux --mount type=bind,source="$(get-location)",destination=/paradigm_local_git/,readonly --name=temp ubuntu-clang-6 /bin/bash -c "./copy.sh;./install.sh"

exit 0

cd docker
docker build windows-vcpp -t windows-vcpp
cd ..

echo "booting up windows-vcpp image..."
docker run --rm --env EXTRA_CMAKE_PARAMS="-DPE_MODE=LIB" --platform=windows --mount type=bind,source="$(get-location)",destination=/paradigm_local_git/,readonly --name=temp windows-vcpp-6 /bin/bash -c "./copy.sh;./install.sh"

exit 0