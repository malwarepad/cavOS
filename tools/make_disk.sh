#!/bin/bash
set -x # show cmds
set -e # fail globally

# $1 -> target system, $2 -> mounted volume, $3 -> image file
if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]; then
	echo "Please supply the correct arguments!"
	exit 1
fi

dd if=/dev/zero of="${3}" bs=512 count=131072
parted "${3}" mklabel msdos mkpart primary ext4 2048s 100% set 1 boot on

sudo losetup /dev/loop101 "${3}"
sudo losetup /dev/loop102 "${3}" -o 1048576
sudo mkdosfs -F32 -f 2 /dev/loop102
sudo fatlabel /dev/loop102 CAVOS
sudo mkdir -p "${2}"
sudo mount /dev/loop102 "${2}"

sudo mkdir -p "${2}/boot/grub"
if command -v grub2-install &>/dev/null; then # fedora case
	sudo grub2-install --target=i386-pc --root-directory="${2}" --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/loop101
	sudo cp -r ${1}/boot/grub/* "${2}/boot/grub2/"
else
	sudo grub-install --target=i386-pc --root-directory="${2}" --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/loop101
fi

sudo cp -r ${1}/* "${2}/"

CURRENT_DIRECTORY=$(dirname "$0")
chmod +x "${CURRENT_DIRECTORY}/cleanup.sh"
"${CURRENT_DIRECTORY}/cleanup.sh" "${2}"

# sudo umount "${2}"
# sudo losetup -d /dev/loop101
# sudo losetup -d /dev/loop102
