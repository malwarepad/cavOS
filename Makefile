all: disk

# https://stackoverflow.com/questions/3931741/why-does-make-think-the-target-is-up-to-date
.PHONY: disk tools clean qemu qemu_dbg vmware dev kernel newlib renewlib cleannewlib

renewlib: cleannewlib newlib

cleannewlib:
	rm -rf src/libs/newlib/cavos-build
	rm -rf src/libs/newlib/cavos-out/i686-cavos/

newlib:
	chmod +x src/libs/newlib/build.sh
	./src/libs/newlib/build.sh --noreplace

# Primary (disk creation)
disk: newlib
	@$(MAKE) -C src/libs/system
	@$(MAKE) -C src/software/test
	@$(MAKE) -C src/kernel disk

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
dev: clean disk qemu_dbg
	