#!/usr/bin/env bash
set -x # show cmds
set -e # fail globally

# $1 -> target system, $2 -> mounted volume, $3 -> image file
if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]; then
	echo "Please supply the correct arguments!"
	exit 1
fi

SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

LIMINE_DIR="${SCRIPTPATH}/../src/bootloader/limine/"
LIMINE_EXEC="${LIMINE_DIR}/limine"

if [ ! -f "${LIMINE_EXEC}" ]; then
	echo "Please compile the bootloader (limine) first!"
	exit 1
fi

dd if=/dev/zero of="${3}" bs=512 count=131072
parted "${3}" mklabel msdos mkpart primary ext4 2048s 100% set 1 boot on
"$LIMINE_EXEC" bios-install "${3}"

sudo losetup /dev/loop101 "${3}"
sudo losetup /dev/loop102 "${3}" -o 1048576
sudo mkdosfs -F32 -f 2 /dev/loop102
sudo fatlabel /dev/loop102 CAVOS
sudo mkdir -p "${2}"
sudo mount /dev/loop102 "${2}"

sudo mkdir -p "${2}/boot/limine/" "${2}/EFI/BOOT"
sudo cp "$LIMINE_DIR/limine-bios.sys" "${2}/boot/limine/"
sudo cp "$LIMINE_DIR/BOOTX64.EFI" "${2}/EFI/BOOT"
sudo cp "$LIMINE_DIR/BOOTIA32.EFI" "${2}/EFI/BOOT"

sudo cp -r ${1}/* "${2}/"

CURRENT_DIRECTORY=$(dirname "$0")
chmod +x "${CURRENT_DIRECTORY}/cleanup.sh"
"${CURRENT_DIRECTORY}/cleanup.sh" "${2}"

# sudo umount "${2}"
# sudo losetup -d /dev/loop101
# sudo losetup -d /dev/loop102
