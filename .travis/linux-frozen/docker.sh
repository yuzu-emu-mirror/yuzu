#!/bin/bash -ex

cd /yuzu

apt-get update
apt-get install -y build-essential wget git python-launchpadlib libssl-dev

# Install specific versions of packages with their dependencies
# The apt repositories remove older versions regularly, so we can't use
# apt-get and have to pull the packages directly from the archives.
/yuzu/.travis/linux-frozen/install_package.py    \
    libsdl2-dev 2.0.4+dfsg1-2ubuntu2 xenial    \
    qtbase5-dev 5.9.3+dfsg-0ubuntu1 bionic    \
    libharfbuzz0b 1.7.2-1 bionic    \
    libqt5opengl5-dev 5.9.3+dfsg-0ubuntu1 bionic    \
    libfontconfig1 2.12.6-0ubuntu1 bionic    \
    libfreetype6 2.8-0.2ubuntu2 bionic    \
    libicu-le-hb0 1.0.3+git161113-4 bionic    \
    libdouble-conversion1 2.0.1-4ubuntu1 bionic    \
    qtchooser 64-ga1b6736-5 bionic    \
    libicu60 60.2-1ubuntu1 bionic

# Get a recent version of CMake
wget https://cmake.org/files/v3.9/cmake-3.9.0-Linux-x86_64.sh
echo y | sh cmake-3.9.0-Linux-x86_64.sh --prefix=cmake
export PATH=/yuzu/cmake/cmake-3.9.0-Linux-x86_64/bin:$PATH

mkdir build && cd build
cmake .. -DYUZU_BUILD_UNICORN=ON -DCMAKE_BUILD_TYPE=Release
make -j4

ctest -VV -C Release
