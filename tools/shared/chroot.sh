#!/bin/bash

# REMEMBER!
# chroot /whatever/wherever/whenever /usr/bin/bash << "EOT"
# # anything
# EOT

chroot_drop() {
	sudo umount -l "$1/dev/shm" || true
	sudo umount -l "$1/run" || true
	sudo umount -l "$1/sys" || true
	sudo umount -l "$1/proc" || true
	sudo umount -l "$1/dev/pts" || true
	sudo umount -l "$1/dev" || true
}

chroot_establish() {
	chroot_drop "$1"

	mkdir -p "$1/"{dev,proc,sys,run,tmp}
	sudo mount --bind /dev "$1/dev"
	sudo mount -t devpts devpts -o gid=5,mode=0620 "$1/dev/pts"
	sudo mount -t proc proc "$1/proc"
	sudo mount -t sysfs sysfs "$1/sys"
	sudo mount -t tmpfs tmpfs "$1/run"
	if [ -h "$1/dev/shm" ]; then
		sudo install -v -d -m 1777 "$1/$(realpath /dev/shm)"
	else
		sudo mount -t tmpfs -o nosuid,nodev tmpfs "$1/dev/shm"
	fi
}

# chroot_establish "$1"
# sudo chroot "$1/" /usr/bin/env -i HISTFILE=/dev/null /bin/bash
# chroot_drop "$1"
