#!/bin/bash -ex

apt-get update
apt-get install -y clang-format-6.0

# Run clang-format
cd /yuzu
./.travis/clang-format/script.sh
