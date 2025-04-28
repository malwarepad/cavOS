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

# Limine
relimine: cleanlimine limine
	
cleanlimine:
	rm -rf src/bootloader/limine
	rm -f src/bootloader/.version

limine:
	chmod +x src/bootloader/assert.sh
	src/bootloader/assert.sh

uacpi:
	chmod +x tools/kernel/uacpi.sh
	tools/kernel/uacpi.sh

# Primary (disk creation)
disk_prepare: verifytools limine uacpi musl ports
# @$(MAKE) -C src/libs/system
	@$(MAKE) -C src/software/test -j1
	@$(MAKE) -C src/software/badtest -j1
	@$(MAKE) -C src/software/drawimg -j1
disk: disk_prepare clean
	@$(MAKE) -C src/kernel disk
disk_dirty: disk_prepare
	@$(MAKE) -C src/kernel disk_dirty

# Verify our toolchain is.. there!
TOOLCHAIN_GCC_VERSION := $(shell ~/opt/cross/bin/x86_64-cavos-gcc --version 2>/dev/null)
GCC_CHECK_DATE = 1727727259
GCC_ACTUAL_DATE := $(shell stat -c %Y ~/opt/cross/bin/x86_64-cavos-gcc)
verifytools:
ifdef TOOLCHAIN_GCC_VERSION
	@echo "$(TOOLCHAIN_GCC_VERSION)"
else
	@echo -e '\033[0;31mx86_64-cavos-gcc was not found! Please use "make tools" to compile the toolchain!\033[0m'
	@exit 1
endif
	@if [ $(GCC_ACTUAL_DATE) -lt $(GCC_CHECK_DATE) ]; then \
		echo -e '\033[0;31mThe cavOS toolchain is outdated! Please remove the ~/opt/cross/ directory and re-build it via "make tools"!\033[0m'; \
		exit 1; \
	fi

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
ringhome:
	cmd.exe /c C:/Users/Panagiotis/ring.bat
dev: clean disk_dirty qemu_dbg

# Code formatting tools
FIND_CMD = find src/kernel/ -type f \
	\( -name *.c -o -name *.h \) \
	-not -path "src/kernel/acpi/uacpi/*" \
	-not -path "src/kernel/include/uacpi/*" \
	-not -path "src/kernel/networking/lwip/*" \
	-not -name printf.c \
	-not -name printf.h \
	-not -name malloc.c \
	-not -name malloc.h \
	-not -name limine.h
CLANG_FORMAT_STYLE:=-style='{ BasedOnStyle: LLVM, AlignConsecutiveDeclarations: true, IndentWidth: 2 }'

# Embedded clang-format (I came across big issues when versions do not match perfectly)
CLANG_FORMAT_TARGET="$(HOME)/opt/clang-format-cavos"
format_prerequisites:
	@chmod +x tools/toolchain/get_formatter.sh
	@tools/toolchain/get_formatter.sh

format: format_prerequisites
	@$(FIND_CMD) | xargs $(CLANG_FORMAT_TARGET) $(CLANG_FORMAT_STYLE) -i

format_check: format_prerequisites
	@$(FIND_CMD) | xargs $(CLANG_FORMAT_TARGET) $(CLANG_FORMAT_STYLE) --dry-run --Werror

# this makefile is designed to work in order..
# however the -j flag is passed to ones that benefit from multiple jobs (such as the kernel)
.NOTPARALLEL: