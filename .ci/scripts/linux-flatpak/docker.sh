#!/bin/bash -ex

# Converts "yuzu-emu/yuzu-release" to "yuzu-release"
REPO_NAME=$(echo $AZURE_REPO_SLUG | cut -d'/' -f 2)
YUZU_SRC_DIR="/yuzu"
BUILD_DIR="$YUZU_SRC_DIR/build"
REPO_DIR="$YUZU_SRC_DIR/repo"
STATE_DIR="$YUZU_SRC_DIR/.flatpak-builder"
SSH_DIR="/upload"
SSH_KEY="/tmp/ssh.key"
GPG_KEY="/tmp/gpg.key"

# Generate flatpak Manifest and AppData files (/tmp/appdata.xml and /tmp/org.yuzu.$REPO_NAME.json)
/bin/bash -ex $YUZU_SRC_DIR/.ci/scripts/linux-flatpak/generate-data.sh $1

# Configure SSH keys
eval "$(ssh-agent -s)"
chmod 700 "$HOME/.ssh"
ssh-add "$SSH_KEY"
echo "[$FLATPAK_SSH_HOSTNAME]:$FLATPAK_SSH_PORT,[$(dig +short $FLATPAK_SSH_HOSTNAME)]:$FLATPAK_SSH_PORT $FLATPAK_SSH_PUBLIC_KEY" > $HOME/.ssh/known_hosts

# Configure GPG keys
gpg2 --import "$GPG_KEY"

# Set permissions
chown -R yuzu "$YUZU_SRC_DIR"
chmod -R 700 "$YUZU_SRC_DIR"

# Mount our flatpak repository
# -o reconnect and -o ServerAliveInterval ensure that
# the share stays active during long flatpak builds
mkdir -p "$REPO_DIR"
#sshfs "$FLATPAK_SSH_USER@$FLATPAK_SSH_HOSTNAME:$SSH_DIR" "$REPO_DIR" -C -p "$FLATPAK_SSH_PORT" -o IdentityFile="$SSH_KEY" -o ServerAliveInterval=60 -o "reconnect" -o auto_cache -o no_readahead

# setup ccache location
chown -R yuzu "$HOME/ccache"
chmod -R 700 "$HOME/ccache"
mkdir -p "$STATE_DIR"
chown -R yuzu "$STATE_DIR"
chmod -R 700 "$STATE_DIR"
ln -sv --force $HOME/ccache "$STATE_DIR/ccache"

# Set ccache permissions
chmod -R 700 "$STATE_DIR/ccache"

# Build the yuzu flatpak
flatpak-builder -v --jobs=4 --ccache --force-clean --state-dir="$STATE_DIR" --gpg-sign="$FLATPAK_GPG_PUBLIC_KEY" --repo="$REPO_DIR" "$BUILD_DIR" "/tmp/org.yuzu.$REPO_NAME.json"
flatpak build-update-repo "$REPO_DIR" -v --generate-static-deltas --gpg-sign="$FLATPAK_GPG_PUBLIC_KEY"
