#!/bin/bash -ex

apt-get update
apt-get install -y build-essential git libqt5opengl5-dev libsdl2-dev libssl-dev python qtbase5-dev wget ninja-build

# Get a recent version of CMake
wget https://cmake.org/files/v3.10/cmake-3.10.1-Linux-x86_64.sh
sh cmake-3.10.1-Linux-x86_64.sh --exclude-subdir --prefix=/ --skip-license

cd /yuzu

mkdir build && cd build
cmake .. -DYUZU_BUILD_UNICORN=ON -DCMAKE_BUILD_TYPE=Release -G Ninja
ninja

ctest -VV -C Release
