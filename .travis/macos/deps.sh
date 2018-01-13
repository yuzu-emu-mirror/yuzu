#!/bin/sh -ex

brew update
brew install dylibbundler p7zip qt5 sdl2

mkdir externals/unicorn
pushd externals/unicorn
git clone https://github.com/yuzu-emu/unicorn .
UNICORN_ARCHS=aarch64 ./make.sh macos-universal-no
popd
