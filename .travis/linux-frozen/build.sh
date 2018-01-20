#!/bin/bash -ex

docker pull ubuntu:16.04
docker run -v $(pwd):/yuzu ubuntu:16.04 /bin/bash -ex /yuzu/.travis/linux-frozen/docker.sh
