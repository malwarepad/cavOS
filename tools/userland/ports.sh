#!/bin/bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

# Useful
USR_PATHNAME="${SCRIPTPATH}/../../target/usr/"
PATCH_PATHNAME="${SCRIPTPATH}/../../patches"

# build_package_autotools($uri, $install_dir, $config_sub_path, $extra_parameters, $optional_patchname, $extra_install_parameters, $autoconf_extra)
source "${SCRIPTPATH}/../shared/cross_compile.sh"

# Ensure /bin is not a directory anymore
if [ $(readlink "$USR_PATHNAME/../bin" || echo "-") != "-" ]; then
	echo -e "target/bin/ already exists and is not set up correctly! It should not be a symlink and just be a normal folder!\nIt is suggested you start over as the userspace has changed since you last tested cavOS. Delete the repo, re-clone and try again!"
	exit 1
fi

# Ensure we have verified nameservers on our resolv.conf so chroots work (wsl2 thing mostly)
echo -e "# This file was automatically generated by the cavOS bootstrapping script ports.sh. In order to stop that, you will have to modify it yourself.\nnameserver 1.1.1.1\nnameserver 1.0.0.1" >"$USR_PATHNAME/../etc/resolv.conf"

# Make sure we are using the correct timezone
HOST_TIMEZONE=$(readlink -f /etc/localtime)
TARGET_TIMEZONE=$(readlink -f "$USR_PATHNAME/../etc/localtime" || echo definitely_not_lol)
if [ "${HOST_TIMEZONE}" != "${TARGET_TIMEZONE}" ]; then
	rm -f "$TARGET_TIMEZONE"
	ln -s "$HOST_TIMEZONE" "$TARGET_TIMEZONE"
fi

# APK package manager (host)
APK_PATH="$HOME/opt/apk-static-cavos"
APK_URI="https://gitlab.alpinelinux.org/api/v4/projects/5/packages/generic/v2.14.6/x86_64/apk.static"
APK_SHA512="782b29d10256ad07fbdfa9bf1b2ac4df9a9ae7162c836ee0ecffc991a4f75113512840f7b3959f5deb81f1d6042c15eeb407139896a8a02c57060de986489e7a"
chmod +x "tools/shared/pass_acq.sh"
tools/shared/pass_acq.sh "$APK_URI" "$APK_SHA512" "$APK_PATH"
chmod +x "$APK_PATH"

# Alpine version. todo: when it gets out of date, implement some sort of update check mechanism
ALPINE_BOOSTRAP_VERSION="3.21"

if ! cat "target/etc/apk/world" 2>/dev/null | grep -i "alpine-base"; then
	# bootstrap alpine userspace
	sudo "$APK_PATH" --arch x86_64 -X http://dl-cdn.alpinelinux.org/alpine/v$ALPINE_BOOSTRAP_VERSION/main/ -U --allow-untrusted --root target/ --initdb add alpine-base bash
	echo -e "http://dl-cdn.alpinelinux.org/alpine/v$ALPINE_BOOSTRAP_VERSION/main\nhttp://dl-cdn.alpinelinux.org/alpine/v$ALPINE_BOOSTRAP_VERSION/community" | sudo tee target/etc/apk/repositories

	# get into it
	source "${SCRIPTPATH}/../shared/chroot.sh"
	chroot_establish "$USR_PATHNAME/../"
	sudo chroot "target/" /bin/bash -c "apk update"
	chroot_drop "$USR_PATHNAME/../"
fi

# Basic software
source "${SCRIPTPATH}/../shared/chroot.sh"
chroot_establish "$USR_PATHNAME/../"
sudo chroot "target/" /bin/bash -c "apk add coreutils procps vim findutils diffutils patch grep sed gawk gzip xz make file tar nasm python3 gcc musl-dev pciutils fastfetch figlet xorg-server xinit xf86-input-evdev xf86-video-fbdev twm xsetroot xeyes"
chroot_drop "$USR_PATHNAME/../"

# Empty for the time being
echo "" | sudo tee target/usr/lib/os-release

# For the love of god use bash instead of busybox sh
sudo rm target/bin/sh
sudo ln -s "/bin/bash" target/bin/sh
