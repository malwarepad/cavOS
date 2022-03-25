#!/bin/bash

RED='\033[0;31m'
NC='\033[0m'

if ! command -v qemu-system-x86_64 &> /dev/null; then
	echo -e "${RED}The 'qemu-system-x86_64' emulator is not installed! It is required to test the compiled ISO.${NC}"
	exit 1
fi

if [ ! -f cavOS.iso ]; then
	make clear
	if ! make build ; then
		echo -e "${RED}cavOS did not build correctly! Please check the make output${NC}"
		exit 1
	fi
fi

qemu-system-x86_64 -cdrom cavOS.iso -m 512M &> /dev/null