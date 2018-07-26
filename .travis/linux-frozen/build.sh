#!/bin/bash -ex
mkdir -p "$HOME/.ccache"
docker pull ubuntu:18.04
docker run -v $(pwd):/yuzu -v "$HOME/.ccache":/root/.ccache ubuntu:18.04 /bin/bash -ex /yuzu/.travis/linux-frozen/docker.sh

