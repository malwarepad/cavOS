#!/usr/bin/env bash
set -x # show cmds

# $1 -> mounted volume
if [ -z "$1" ]; then
	echo "Please supply the correct arguments!"
	exit 1
fi

sudo umount -l "${1}"
sudo umount -l "${1}/boot"
sudo losetup -d /dev/loop101
sudo losetup -d /dev/loop102
sudo losetup -d /dev/loop103
