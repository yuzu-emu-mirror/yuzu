#!/bin/bash -ex

. .travis/common/pre-upload.sh

REV_NAME="yuzu-linux-${GITDATE}-${GITREV}"
ARCHIVE_NAME="${REV_NAME}.tar.xz"
COMPRESSION_FLAGS="-cJvf"

mkdir "$REV_NAME"

cp build/bin/Release/yuzu-cmd "$REV_NAME"
cp build/bin/Release/yuzu "$REV_NAME"

. .travis/common/post-upload.sh
