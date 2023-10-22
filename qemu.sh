#!/bin/bash

RED='\033[0;31m'
NC='\033[0m'

if ! command -v qemu-system-x86_64 &>/dev/null; then
	echo -e "${RED}The 'qemu-system-x86_64' emulator is not installed! It is required to test the compiled ISO.${NC}"
	exit 1
fi

if [ ! -f cavOS.iso ]; then
	qemu-system-x86_64 -d guest_errors -serial stdio -drive file=disk.img,format=raw -m 1g -netdev user,id=mynet0 -net nic,model=ne2k_pci,netdev=mynet0
else
	qemu-system-x86_64 -d guest_errors -serial stdio -drive file=cavOS.iso,format=raw -m 1g -netdev user,id=mynet0 -net nic,model=ne2k_pci,netdev=mynet0
fi
