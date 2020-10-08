#!/bin/bash -ex
# This script generates the appdata.xml and org.yuzu.$REPO_NAME.json files
# needed to define application metadata and build yuzu

# Converts "yuzu-emu/yuzu-release" to "yuzu-release"
REPO_NAME=$(echo $AZURE_REPO_SLUG | cut -d'/' -f 2)
# Converts "yuzu-release" to "yuzu Release"
REPO_NAME_FRIENDLY=$(echo $REPO_NAME | sed -e 's/-/ /g' -e 's/\b\(.\)/\u\1/g')

# Generate the correct appdata.xml for the version of yuzu we're building
cat > /tmp/appdata.xml <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<application>
  <id type="desktop">org.yuzu.$REPO_NAME.desktop</id>
  <name>$REPO_NAME_FRIENDLY</name>
  <summary>Nintendo Switch emulator</summary>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>GPL-2.0</project_license>
  <description>
    <p>yuzu is an experimental open-source emulator for the Nintendo Switch from the creators of Citra.</p>
    <p>It is written in C++ with portability in mind, with builds actively maintained for Windows and Linux. The emulator is currently only useful for homebrew development and research purposes.</p>
    <p>yuzu only emulates a subset of Switch hardware and therefore is generally only useful for running/debugging homebrew applications. At this time, yuzu cannot play a majority of commercial games without major problems. yuzu can boot some commercial Switch games to varying degrees of success, but your experience may vary between games and for different combinations of host hardware.</p>
    <p>yuzu is licensed under the GPLv2 (or any later version). Refer to the license.txt file included.</p>
  </description>
  <url type="homepage">https://yuzu-emu.org/</url>
  <url type="donation">https://yuzu-emu.org/donate/</url>
  <url type="bugtracker">https://github.com/yuzu-emu/yuzu/issues</url>
  <url type="faq">https://yuzu-emu.org/wiki/faq/</url>
  <url type="help">https://yuzu-emu.org/wiki/home/</url>
  <screenshot>https://yuzu-emu.org/images/screenshots/001-Super%20Mario%20Odyssey.png</screenshot>
  <screenshot>https://yuzu-emu.org/images/screenshots/004-Super%20Mario%20Odyssey.png</screenshot>
  <screenshot>https://yuzu-emu.org/images/screenshots/019-Pokken%20Tournament.png</screenshot>
  <screenshot>https://yuzu-emu.org/images/screenshots/052-Pokemon%20Let%27s%20Go.png</screenshot>
  <categories>
    <category>Games</category>
    <category>Emulator</category>
  </categories>
</application>
EOF

cat > /tmp/yuzu-wrapper <<EOF
#!/bin/bash

# Discord only accepts activity updates from pids >= 10
for i in 1 2 3 .. 20
do
    # Spawn a new shell
    # This guarantees that a new process is created (unlike with potential bash internals like echo etc.)
    bash -c "true"
    sleep 0
done

# Symlink com.discordapp.Discord ipc pipes if they do not exist yet
for i in {0..9}; do
    test -S \$XDG_RUNTIME_DIR/app/com.discordapp.Discord/discord-ipc-\$i && ln -sf {\$XDG_RUNTIME_DIR/app/com.discordapp.Discord,\$XDG_RUNTIME_DIR}/discord-ipc-\$i;
done

yuzu \$@
EOF

# Generate the yuzu flatpak manifest
cat > /tmp/org.yuzu.$REPO_NAME.json <<EOF
{
    "app-id": "org.yuzu.$REPO_NAME",
    "runtime": "org.kde.Platform",
    "runtime-version": "5.13",
    "sdk": "org.kde.Sdk",
    "command": "yuzu-wrapper",
    "rename-desktop-file": "yuzu.desktop",
    "rename-icon": "yuzu",
    "rename-appdata-file": "org.yuzu.$REPO_NAME.appdata.xml",
    "build-options": {
        "build-args": [
            "--share=network"
        ],
        "env": {
            "AZURE_BRANCH": "$AZURE_BRANCH",
            "AZURE_BUILD_ID": "$AZURE_BUILD_ID",
            "AZURE_BUILD_NUMBER": "$AZURE_BUILD_NUMBER",
            "AZURE_COMMIT": "$AZURE_COMMIT",
            "AZURE_JOB_ID": "$AZURE_JOB_ID",
            "AZURE_REPO_SLUG": "$AZURE_REPO_SLUG",
            "AZURE_TAG": "$AZURE_TAG",

            "CONAN_USER_HOME": "/run/build/yuzu"
        }
    },
    "finish-args": [
        "--device=all",
        "--socket=x11",
        "--socket=pulseaudio",
        "--share=network",
        "--share=ipc",
        "--filesystem=xdg-config/yuzu:create",
        "--filesystem=xdg-data/yuzu:create",
        "--filesystem=host:ro",
        "--filesystem=xdg-run/app/com.discordapp.Discord:create",
        "--filesystem=xdg-run/discord-ipc-0:rw",
        "--filesystem=xdg-run/discord-ipc-1:rw",
        "--filesystem=xdg-run/discord-ipc-2:rw",
        "--filesystem=xdg-run/discord-ipc-3:rw",
        "--filesystem=xdg-run/discord-ipc-4:rw",
        "--filesystem=xdg-run/discord-ipc-5:rw",
        "--filesystem=xdg-run/discord-ipc-6:rw",
        "--filesystem=xdg-run/discord-ipc-7:rw",
        "--filesystem=xdg-run/discord-ipc-8:rw",
        "--filesystem=xdg-run/discord-ipc-9:rw"
    ],
    "modules": [
        {
            "name": "python3-conan",
            "buildsystem": "simple",
            "build-commands": [
                "pip3 install --exists-action=i --no-index --find-links=\"file://\${PWD}\" --prefix=\${FLATPAK_DEST} \"conan\""
            ],
            "cleanup": ["*"],
            "sources": [
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/8a/bb/488841f56197b13700afd5658fc279a2025a39e22449b7cf29864669b15d/pyparsing-2.4.7-py2.py3-none-any.whl",
                    "sha256": "ef9d7589ef3c200abe66653d3f1ab1033c3c419ae9b9bdb1240a85b024efc88b"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/b1/a7/588bfa063e7763247ab6f7e1d994e331b85e0e7d09f853c59a6eb9696974/packaging-20.8-py2.py3-none-any.whl",
                    "sha256": "24e0da08660a87484d1602c30bb4902d74816b6985b93de36926f5bc95741858"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/56/aa/4ef5aa67a9a62505db124a5cb5262332d1d4153462eb8fd89c9fa41e5d92/urllib3-1.25.11-py2.py3-none-any.whl",
                    "sha256": "f5321fbe4bf3fefa0efd0bfe7fb14e90909eb62a48ccda331726b4319897dd5e"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/80/02/8f8880a4fd6625461833abcf679d4c12a44c76f9925f92bf212bb6cefaad/tqdm-4.56.0-py2.py3-none-any.whl",
                    "sha256": "4621f6823bab46a9cc33d48105753ccbea671b68bab2c50a9f0be23d4065cb5a"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/ee/ff/48bde5c0f013094d729fe4b0316ba2a24774b3ff1c52d924a8a4cb04078a/six-1.15.0-py2.py3-none-any.whl",
                    "sha256": "8b74bedcbbbaca38ff6d7491d76f2b06b3592611af620f8426e82dddb04a5ced"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/a2/38/928ddce2273eaa564f6f50de919327bf3a00f091b5baba8dfa9460f3a8a8/idna-2.10-py2.py3-none-any.whl",
                    "sha256": "b97d804b1e9b523befed77c48dacec60e6dcb0b5391d57af6a65a312a90648c0"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/19/c7/fa589626997dd07bd87d9269342ccb74b1720384a4d739a1872bd84fbe68/chardet-4.0.0-py2.py3-none-any.whl",
                    "sha256": "f864054d66fd9118f2e67044ac8981a54775ec5b67aed0441892edb553d21da5"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/5e/a0/5f06e1e1d463903cf0c0eebeb751791119ed7a4b3737fdc9a77f1cdfb51f/certifi-2020.12.5-py2.py3-none-any.whl",
                    "sha256": "719a74fb9e33b9bd44cc7f3a8d94bc35e4049deebe19ba7d8e108280cfd59830"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/29/c1/24814557f1d22c56d50280771a17307e6bf87b70727d975fd6b2ce6b014a/requests-2.25.1-py2.py3-none-any.whl",
                    "sha256": "c210084e36a42ae6b9219e00e48287def368a26d03a048ddad7bfee44f75871e"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/64/c2/b80047c7ac2478f9501676c988a5411ed5572f35d1beff9cae07d321512c/PyYAML-5.3.1.tar.gz",
                    "sha256": "b8eac752c5e14d3eca0e6dd9199cd627518cb5ec06add0de9d32baeee6fe645d"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/d4/70/d60450c3dd48ef87586924207ae8907090de0b306af2bce5d134d78615cb/python_dateutil-2.8.1-py2.py3-none-any.whl",
                    "sha256": "75bb3f31ea686f1197762692a9ee6a7550b59fc6ca3a1f4b5d7e32fb98e2da2a"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/87/8b/6a9f14b5f781697e51259d81657e6048fd31a113229cf346880bb7545565/PyJWT-1.7.1-py2.py3-none-any.whl",
                    "sha256": "5c6eca3c2940464d106b99ba83b00c6add741c9becaec087fb7ccdefea71350e"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/17/e3/c7ea888bd6e9849c60b1f378637850265177ed01297137f544a7ecf7d7ba/Pygments-2.7.4-py3-none-any.whl",
                    "sha256": "bc9591213a8f0e0ca1a5e68a479b4887fdc3e75d0774e5c71c31920c427de435"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/3d/3c/fe974b4f835f83cc46966e04051f8708b7535bac28fbc0dcca1ee0c237b8/pluginbase-1.0.0.tar.gz",
                    "sha256": "497894df38d0db71e1a4fbbfaceb10c3ef49a3f95a0582e11b75f8adaa030005"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/c1/b2/ad3cd464101435fdf642d20e0e5e782b4edaef1affdf2adfc5c75660225b/patch-ng-1.17.4.tar.gz",
                    "sha256": "627abc5bd723c8b481e96849b9734b10065426224d4d22cd44137004ac0d4ace"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/b9/2e/64db92e53b86efccfaea71321f597fa2e1b2bd3853d8ce658568f7a13094/MarkupSafe-1.1.1.tar.gz",
                    "sha256": "29872e92839765e546828bb7754a68c418d927cd064fd4708fab9fe9c8bb116b"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/30/9e/f663a2aa66a09d838042ae1a2c5659828bb9b41ea3a6efa20a20fd92b121/Jinja2-2.11.2-py2.py3-none-any.whl",
                    "sha256": "f0a4641d3cf955324a89c04f3d94663aa4d638abe8f733ecd3582848e1c37035"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/45/0b/38b06fd9b92dc2b68d58b75f900e97884c45bedd2ff83203d933cf5851c9/future-0.18.2.tar.gz",
                    "sha256": "b1bead90b70cf6ec3f0710ae53a525360fa360d306a86583adc6bf83a4db537d"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/78/20/c862d765287e9e8b29f826749ebae8775bdca50b2cb2ca079346d5fbfd76/fasteners-0.16-py2.py3-none-any.whl",
                    "sha256": "74b6847e0a6bb3b56c8511af8e24c40e4cf7a774dfff5b251c260ed338096a4b"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/25/b7/b3c4270a11414cb22c6352ebc7a83aaa3712043be29daa05018fd5a5c956/distro-1.5.0-py2.py3-none-any.whl",
                    "sha256": "df74eed763e18d10d0da624258524ae80486432cd17392d9c3d96f5e83cd2799"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/b9/2a/d5084a8781398cea745c01237b95d9762c382697c63760a95cc6a814ad3a/deprecation-2.0.7-py2.py3-none-any.whl",
                    "sha256": "dc9b4f252b7aca8165ce2764a71da92a653b5ffbf7a389461d7a640f6536ecb2"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/44/98/5b86278fbbf250d239ae0ecb724f8572af1c91f4a11edf4d36a206189440/colorama-0.4.4-py2.py3-none-any.whl",
                    "sha256": "9f47eda37229f68eee03b24b9748937c7dc3868f906e8ba69fbcbdd3bc5dc3e2"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/bf/44/aeafdd6ca05a8e1c3f91eeeb272a202d5cb1b3b23730a5ca686a81c48d24/bottle-0.12.19-py3-none-any.whl",
                    "sha256": "f6b8a34fe9aa406f9813c02990db72ca69ce6a158b5b156d2c41f345016a723d"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/08/51/6cf3a2b18ca35cbe4ad3c7538a7c3dc0cb24e71629fb16e729c137d06432/node_semver-0.6.1-py3-none-any.whl",
                    "sha256": "d4bf83873894591a0cbb6591910d96917fbadc9731e8e39e782d3a2fbc2b841e"
                },
                {
                    "type": "file",
                    "url": "https://files.pythonhosted.org/packages/cf/3b/7fc6030e64609ef6ddf9a3f88c297794d59d89fd2ab13989a9aee47cad02/conan-1.33.0.tar.gz",
                    "sha256": "3debc02daf1be7198ed190322ff6d7deaeab0a2ef3e3f4b23033100cfa9bd8ab"
                }
            ]
        },
        {
            "name": "yuzu",
            "buildsystem": "cmake-ninja",
            "builddir": true,
            "config-opts": [
                "-DDISPLAY_VERSION=$1",
                "-DYUZU_USE_QT_WEB_ENGINE=OFF",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DYUZU_ENABLE_COMPATIBILITY_REPORTING=ON",
                "-DENABLE_COMPATIBILITY_LIST_DOWNLOAD=ON",
                "-DUSE_DISCORD_PRESENCE=ON"
            ],
            "cleanup": [
              "/bin/yuzu-cmd",
              "/share/man",
              "/share/pixmaps"
            ],
            "post-install": [
                "install -Dm644 ../appdata.xml /app/share/appdata/org.yuzu.$REPO_NAME.appdata.xml",
                "desktop-file-install --dir=/app/share/applications ../dist/yuzu.desktop",
                "install -Dm644 ../dist/yuzu.svg /app/share/icons/hicolor/scalable/apps/yuzu.svg",
                "sed -i 's/Name=yuzu/Name=$REPO_NAME_FRIENDLY/g' /app/share/applications/yuzu.desktop",
                "mv /app/share/mime/packages/yuzu.xml /app/share/mime/packages/org.yuzu.$REPO_NAME.xml",
                "sed 's/yuzu/org.yuzu.$REPO_NAME/g' -i /app/share/mime/packages/org.yuzu.$REPO_NAME.xml",
                'install -D ../yuzu-wrapper /app/bin/yuzu-wrapper',
                "desktop-file-edit --set-key=Exec --set-value='/app/bin/yuzu-wrapper %f' /app/share/applications/yuzu.desktop"
            ],
            "sources": [
                {
                    "type": "git",
                    "url": "https://github.com/yuzu-emu/$REPO_NAME.git",
                    "branch": "master",
                    "disable-shallow-clone": true
                },
                {
                    "type": "file",
                    "path": "/tmp/appdata.xml"
                },
                {
                    "type": "file",
                    "path": "/tmp/yuzu-wrapper"
                }
            ]
        }
    ]
}
EOF

