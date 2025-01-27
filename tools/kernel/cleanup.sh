#!/usr/bin/env bash
set -x # show cmds

is_mounted() {
	mountpoint -q "$1"
}

unmount_with_retry() {
	local target="$1"
	while is_mounted "$target"; do
		if sudo umount "$target" 2>/dev/null; then
			echo "Successfully unmounted: $target"
			return 0
		else
			echo "Failed to unmount $target (possibly busy). Retrying in 2 seconds..."
			sleep 2
		fi
	done
	echo "$target is not mounted."
}

# $1 -> mounted volume
if [ -z "$1" ]; then
	echo "Please supply the correct arguments!"
	exit 1
fi

unmount_with_retry "${1}/boot"
unmount_with_retry "${1}"
sudo losetup -d /dev/loop101
