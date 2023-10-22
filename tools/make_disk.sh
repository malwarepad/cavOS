#!/bin/bash
set -x # show cmds
set -e # fail globally

dd if=/dev/zero of=disk.img bs=512 count=131072
parted disk.img mklabel msdos mkpart primary ext4 2048s 100% set 1 boot on

sudo losetup /dev/loop101 disk.img
sudo losetup /dev/loop102 disk.img -o 1048576
sudo mkdosfs -F32 -f 2 /dev/loop102
sudo fatlabel /dev/loop102 CAVOS
sudo mkdir -p /cavosmnt
sudo mount /dev/loop102 /cavosmnt

sudo mkdir -p /cavosmnt/boot/grub
if command -v grub2-install &>/dev/null; then # fedora case
	sudo grub2-install --target=i386-pc --root-directory=/cavosmnt --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/loop101
	sudo cp -r tmp/boot/grub/* /cavosmnt/boot/grub2/
else
	sudo grub-install --target=i386-pc --root-directory=/cavosmnt --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/loop101
fi

sudo cp -r tmp/* /cavosmnt/

sudo umount /cavosmnt
sudo losetup -d /dev/loop101
sudo losetup -d /dev/loop102
