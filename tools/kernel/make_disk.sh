#!/usr/bin/env bash
set -x # show cmds
set -e # fail globally

# $1 -> target system, $2 -> mounted volume, $3 -> image file
if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]; then
	echo "Please supply the correct arguments!"
	exit 1
fi
# NOTE: $4 -> quick kernel copy

SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

if [ ! -d "$1/bin" ]; then
	cd "$1"
	ln -s usr/bin bin
	cd "$SCRIPTPATH"
fi

if test -f /dev/loop101; then
	echo "Loopback device designated for cavOS is already assigned to something! (/dev/loop101)"
	exit 1
fi

LIMINE_DIR="${SCRIPTPATH}/../../src/bootloader/limine/"
LIMINE_EXEC="${LIMINE_DIR}/limine"

if [ ! -f "${LIMINE_EXEC}" ]; then
	echo "Please compile the bootloader (limine) first!"
	exit 1
fi

SIZE_IN_BYTES=$(du -sb "$1" | cut -f1)
SIZE_IN_BLOCKS=$((($SIZE_IN_BYTES / 512) * 2 + 500000))

if [ -z "$4" ]; then
	dd if=/dev/zero of="${3}" bs=512 count=$SIZE_IN_BLOCKS
	sudo parted "${3}" mklabel msdos
	sudo parted "${3}" mkpart primary ext4 2048s 68157440B
	sudo parted "${3}" set 1 boot on
	sudo parted "${3}" mkpart primary ext4 136314880B 100%
	"$LIMINE_EXEC" bios-install "${3}"
fi

sudo losetup -P /dev/loop101 "${3}"

if [ -z "$4" ]; then
	sudo mkdosfs -F32 -f 2 /dev/loop101p1 || sudo mkfs.fat -F32 -f 2 /dev/loop101p1
	sudo mke2fs -L "cavOS" -O ^dir_index /dev/loop101p2 "$(((($SIZE_IN_BLOCKS - 350000) * 512) / 1024))"
	sudo fatlabel /dev/loop101p1 LIMINE
fi

sudo mkdir -p "${2}"
sudo mount /dev/loop101p2 "${2}"
sudo mkdir -p "${2}/boot/"
sudo mount /dev/loop101p1 "${2}/boot/"

sudo mkdir -p "${2}/boot/limine/" "${2}/boot/EFI/BOOT"
sudo cp "$LIMINE_DIR/limine-bios.sys" "${2}/boot/limine/"
sudo cp "$LIMINE_DIR/BOOTX64.EFI" "${2}/boot/EFI/BOOT"
sudo cp "$LIMINE_DIR/BOOTIA32.EFI" "${2}/boot/EFI/BOOT"

if [ -z "$4" ]; then
	sudo cp -r ${1}/* "${2}/"
else
	sudo cp -r ${1}/boot/* "${2}/boot/"
fi

sudo rm -f "${2}/.gitignore"

CURRENT_DIRECTORY=$(dirname "$0")
chmod +x "${CURRENT_DIRECTORY}/cleanup.sh"
"${CURRENT_DIRECTORY}/cleanup.sh" "${2}"
