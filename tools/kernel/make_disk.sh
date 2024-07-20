#!/usr/bin/env bash
set -x # show cmds
set -e # fail globally

# $1 -> target system, $2 -> mounted volume, $3 -> image file
if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]; then
	echo "Please supply the correct arguments!"
	exit 1
fi
# NOTE: $4 -> quick kernel copy

if test -f /dev/loop101 || test -f /dev/loop102 || test -f /dev/loop103; then
	echo "Loopback devices designated for cavOS are already assigned to something! (/dev/loop101, /dev/loop102, /dev/loop103)"
	exit 1
fi

SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

LIMINE_DIR="${SCRIPTPATH}/../../src/bootloader/limine/"
LIMINE_EXEC="${LIMINE_DIR}/limine"

if [ ! -f "${LIMINE_EXEC}" ]; then
	echo "Please compile the bootloader (limine) first!"
	exit 1
fi

SIZE_IN_BYTES=$(du -sb "$1" | cut -f1)
SIZE_IN_BLOCKS=$((($SIZE_IN_BYTES / 512) * 2))

if [ -z "$4" ]; then
	dd if=/dev/zero of="${3}" bs=512 count=$SIZE_IN_BLOCKS
	parted "${3}" mklabel msdos
	parted "${3}" mkpart primary ext4 2048s 68157440B
	parted "${3}" set 1 boot on
	parted "${3}" mkpart primary ext4 136314880B 100%
	"$LIMINE_EXEC" bios-install "${3}"
fi

sudo losetup /dev/loop101 "${3}"
sudo losetup /dev/loop102 "${3}" -o 1048576   #  1  MB
sudo losetup /dev/loop103 "${3}" -o 136314880 # 128 MB

if [ -z "$4" ]; then
	sudo mkdosfs -F32 -f 2 /dev/loop102
	sudo mkdosfs -F32 -f 2 /dev/loop103
	# sudo mkfs.ext2 /dev/loop102
	sudo fatlabel /dev/loop102 LIMINE
	sudo fatlabel /dev/loop103 CAVOS
fi

sudo mkdir -p "${2}"
sudo mount /dev/loop103 "${2}"
sudo mkdir -p "${2}/boot/"
sudo mount /dev/loop102 "${2}/boot/"

sudo mkdir -p "${2}/boot/limine/" "${2}/boot/EFI/BOOT"
sudo cp "$LIMINE_DIR/limine-bios.sys" "${2}/boot/limine/"
sudo cp "$LIMINE_DIR/BOOTX64.EFI" "${2}/boot/EFI/BOOT"
sudo cp "$LIMINE_DIR/BOOTIA32.EFI" "${2}/boot/EFI/BOOT"

if [ -z "$4" ]; then
	sudo cp -r ${1}/* "${2}/"
else
	sudo cp -r ${1}/boot/* "${2}/boot/"
fi

CURRENT_DIRECTORY=$(dirname "$0")
chmod +x "${CURRENT_DIRECTORY}/cleanup.sh"
"${CURRENT_DIRECTORY}/cleanup.sh" "${2}"
