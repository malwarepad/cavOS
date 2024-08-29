#!/bin/bash

# REMEMBER!
# chroot /whatever/wherever/whenever /usr/bin/bash << "EOT"
# # anything
# EOT

SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

TARGET_DIR="$SCRIPTPATH/../../target/"

chroot_drop() {
	sudo umount -l "$TARGET_DIR/dev/shm" || true
	sudo umount -l "$TARGET_DIR/run" || true
	sudo umount -l "$TARGET_DIR/sys" || true
	sudo umount -l "$TARGET_DIR/proc" || true
	sudo umount -l "$TARGET_DIR/dev/pts" || true
	sudo umount -l "$TARGET_DIR/dev" || true
}

chroot_establish() {
	chroot_drop

	mkdir -p "$TARGET_DIR/"{dev,proc,sys,run,tmp}
	sudo mount --bind /dev "$TARGET_DIR/dev"
	sudo mount -t devpts devpts -o gid=5,mode=0620 "$TARGET_DIR/dev/pts"
	sudo mount -t proc proc "$TARGET_DIR/proc"
	sudo mount -t sysfs sysfs "$TARGET_DIR/sys"
	sudo mount -t tmpfs tmpfs "$TARGET_DIR/run"
	if [ -h "$TARGET_DIR/dev/shm" ]; then
		sudo install -v -d -m 1777 "$TARGET_DIR/$(realpath /dev/shm)"
	else
		sudo mount -t tmpfs -o nosuid,nodev tmpfs "$TARGET_DIR/dev/shm"
	fi

	# sudo chroot "$TARGET_DIR/" /usr/bin/bash -c "ls files
	# cat files/ab.txt"
}

# chroot_establish
# sudo chroot "$TARGET_DIR/" /usr/bin/env -i HISTFILE=/dev/null /bin/bash
# chroot_drop
