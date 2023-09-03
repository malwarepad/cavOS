#!/bin/bash

RED='\033[0;31m'
NC='\033[0m'

if ! command -v qemu-system-x86_64 &>/dev/null; then
	echo -e "${RED}The 'qemu-system-x86_64' emulator is not installed! It is required to test the compiled ISO.${NC}"
	exit 1
fi

if [ ! -f cavOS.iso ]; then
	qemu-system-x86_64 disk.img &>/dev/null
else
	qemu-system-x86_64 -hda cavOS.iso &>/dev/null
fi
