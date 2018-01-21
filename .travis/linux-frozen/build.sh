#!/bin/bash -ex

docker pull ubuntu:18.04
docker run -v $(pwd):/yuzu ubuntu:18.04 /bin/bash -ex /yuzu/.travis/linux-frozen/docker.sh
