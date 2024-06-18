all: disk

# https://stackoverflow.com/questions/3931741/why-does-make-think-the-target-is-up-to-date
.PHONY: disk tools clean qemu qemu_dbg vmware dev kernel musl remusl cleanmusl limine ports verifytools

# Musl libc
remusl: cleanmusl musl

cleanmusl:
	rm -rf src/libs/musl/cavos-build
	rm -rf src/libs/musl/cavos-out/include
	rm -rf src/libs/musl/cavos-out/lib

musl:
	chmod +x src/libs/musl/build.sh
	./src/libs/musl/build.sh --noreplace

# Essential ports
ports:
	chmod +x tools/userland/ports.sh
	tools/userland/ports.sh

# PCI IDs
repci_id: cleanpci_id target/usr/share/hwdata/pci.ids

cleanpci_id:
	rm -f target/usr/share/hwdata/pci.ids || true

target/usr/share/hwdata/pci.ids:
	mkdir -p target/usr/share/hwdata
	wget --directory-prefix=target/usr/share/hwdata https://pci-ids.ucw.cz/v2.2/pci.ids

# Limine
relimine: cleanlimine limine
	
cleanlimine:
	rm -rf src/bootloader/limine

limine:
	@$(MAKE) -C src/bootloader all

# Primary (disk creation)
disk: verifytools limine musl ports target/usr/share/hwdata/pci.ids
# @$(MAKE) -C src/libs/system
	@$(MAKE) -C src/software/test
	@$(MAKE) -C src/software/badtest
	@$(MAKE) -C src/software/drawimg
	@$(MAKE) -C src/kernel disk

# Verify our toolchain is.. there!
TOOLCHAIN_GCC_VERSION := $(shell ~/opt/cross/bin/x86_64-cavos-gcc --version 2>/dev/null)
verifytools:
ifdef TOOLCHAIN_GCC_VERSION
	@echo "$(TOOLCHAIN_GCC_VERSION)"
else
	@echo -e '\033[0;31mx86_64-cavos-gcc was not found! Please use "make tools" to compile the toolchain!\033[0m'
	@exit 1
endif

# Raw kernel.bin
kernel:
	@$(MAKE) -C src/kernel all

# Tools
tools:
	@$(MAKE) -C src/kernel tools

clean:
	@$(MAKE) -C src/kernel clean

# Hypervisors
qemu:
	@$(MAKE) -C src/kernel qemu

qemu_dbg:
	@$(MAKE) -C src/kernel qemu_dbg

vmware:
	@$(MAKE) -C src/kernel vmware

# Development tools
dev: disk qemu_dbg
	