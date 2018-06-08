#!/bin/bash -ex

docker run -e CCACHE_DIR=/ccache -v $HOME/.ccache:/ccache -v $(pwd):/yuzu ubuntu:18.04 /bin/bash /yuzu/.travis/linux/docker.sh
