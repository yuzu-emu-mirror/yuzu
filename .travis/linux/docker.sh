#!/bin/bash -ex

apt-get update
apt-get install -y build-essential git libcurl4-openssl-dev libqt5opengl5-dev libsdl2-dev libssl-dev python qtbase5-dev wget

# Get a recent version of CMake
wget https://cmake.org/files/v3.10/cmake-3.10.1-Linux-x86_64.sh
sh cmake-3.10.1-Linux-x86_64.sh --exclude-subdir --prefix=/ --skip-license

mkdir /unicorn
cd /unicorn
git clone https://github.com/yuzu-emu/unicorn .
UNICORN_ARCHS=aarch64 ./make.sh
./make.sh install

cd /yuzu

mkdir build && cd build
cmake .. -DUSE_SYSTEM_CURL=ON -DCMAKE_BUILD_TYPE=Release
make -j4

ctest -VV -C Release
