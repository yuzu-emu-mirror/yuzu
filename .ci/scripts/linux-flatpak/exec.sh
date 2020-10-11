#!/bin/bash -ex
mkdir -p "ccache"
mkdir -p "$HOME/.ssh"

chmod a+x ./.ci/scripts/linux-flatpak/docker.sh

# the UID for the container yuzu user is 1027
sudo chown -R 1027 "ccache"
sudo chown -R 1027 $(pwd)
sudo chown -R 1027 "$HOME/.ssh"
docker run --env-file .ci/scripts/linux-flatpak/azure-ci.env --env-file .ci/scripts/linux-flatpak/azure-ci-flatpak.env -v $(pwd):/yuzu -v "$(pwd)/ccache":/home/yuzu/ccache -v "$HOME/.ssh":/home/yuzu/.ssh -v "$SSH_KEY":/tmp/ssh.key -v "$GPG_KEY":/tmp/gpg.key --privileged meirod/build-environments:linux-flatpak /bin/bash -ex /yuzu/.ci/scripts/linux-flatpak/docker.sh $1
sudo chown -R $UID "$HOME/.ssh"
sudo chown -R $UID "ccache"
sudo chown -R $UID $(pwd)
