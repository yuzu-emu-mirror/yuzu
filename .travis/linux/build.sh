#!/bin/bash -ex

docker run -v $(pwd):/yuzu ubuntu:18.04 /bin/bash /yuzu/.travis/linux/docker.sh
