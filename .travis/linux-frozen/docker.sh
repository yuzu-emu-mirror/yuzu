#!/bin/bash -ex

cd /yuzu

apt-get update
apt-get install -y build-essential wget git python-launchpadlib ccache

# Install specific versions of packages with their dependencies
# The apt repositories remove older versions regularly, so we can't use
# apt-get and have to pull the packages directly from the archives.
/yuzu/.travis/linux-frozen/install_package.py       \
    libsdl2-dev 2.0.7+dfsg1-3ubuntu1 bionic          \
    qtbase5-dev 5.9.3+dfsg-0ubuntu2 bionic           \
    libqt5opengl5-dev 5.9.3+dfsg-0ubuntu2 bionic     \
    libicu57 57.1-6ubuntu0.2 bionic                  \
    cmake 3.10.2-1ubuntu2 bionic

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/lib/ccache/gcc -DCMAKE_CXX_COMPILER=/usr/lib/ccache/g++
make -j4

ctest -VV -C Release
