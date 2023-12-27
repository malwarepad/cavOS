all: disk

# https://stackoverflow.com/questions/3931741/why-does-make-think-the-target-is-up-to-date
.PHONY: disk tools clean qemu qemu_dbg vmware dev

# Primary (disk creation)
disk:
	@$(MAKE) -C src/libs/system
	@$(MAKE) -C src/software/test
	@$(MAKE) -C src/kernel disk

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
	