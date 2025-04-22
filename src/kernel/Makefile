COMPILER = ~/opt/cross/bin/x86_64-cavos-gcc
LINKER = ~/opt/cross/bin/x86_64-cavos-ld
ASSEMBLER = nasm
# -w(no warnings) -g(dev)
CFLAGS = -m64 -g -c -ffreestanding -Wall -Werror -fcommon -Iinclude/ -Inetworking/lwip/include/ -fPIE -mno-80387 \
    -mno-mmx \
    -mno-sse \
		-nostartfiles \
		-nostdlib \
    -mno-sse2 \
    -mno-red-zone -fno-stack-protector \
    -fno-stack-check \
    -fno-lto
ASFLAGS = -f elf64
LDFLAGS = -m elf_x86_64 \
		-nostdlib \
    -static \
    -pie \
    --no-dynamic-linker \
    -z text \
    -z max-page-size=0x1000 -T link.ld

TARGET = ../../target
MOUNTPOINT = /cavosmnt
TOOLS = ../../tools

TARGET_IMG = ../../disk.img
TARGET_VMWARE = ../../disk.vmdk
TARGET_ISO = ../../cavOS.iso

OUTPUT = $(TARGET)/boot/kernel.bin

C_SOURCES := $(shell find . -name '*.c' ! -name "malloc.c" ! -name "printf.c" -printf "%P\n")
ASM_SOURCES := $(shell find . -name '*.asm' -printf "%P\n")

C_OBJS = $(patsubst %.c,%.o,$(C_SOURCES))
C_EXTRA_OBJS = memory/malloc.o drivers/printf.o

ASM_OBJS = $(patsubst %.asm,%.asm.o,$(ASM_SOURCES))

all: $(C_OBJS) $(ASM_OBJS) $(C_EXTRA_OBJS)
	mkdir $(TARGET)/ -p
	mkdir $(TARGET)/boot/ -p
	$(LINKER) $(LDFLAGS) -o $(OUTPUT) $(C_OBJS) $(C_EXTRA_OBJS) $(ASM_OBJS)
	
memory/malloc.o:memory/malloc.c
	$(COMPILER) $(CFLAGS) memory/malloc.c -o memory/malloc.o -DHAVE_MMAP=0 -DLACKS_TIME_H=1 -DLACKS_SYS_PARAM_H=1 -LACKS_STRING_H=0 -Dmalloc_getpagesize=4096 -DNO_MALLOC_STATS=1 -DMORECORE_CONTIGUOUS=0 -DUSE_LOCKS=2

drivers/printf.o:drivers/printf.c
	$(COMPILER) $(CFLAGS) drivers/printf.c -o drivers/printf.o -DPRINTF_INCLUDE_CONFIG_H=1

%.o: %.c
	$(COMPILER) $(CFLAGS) $(subst .o,.c,$@) -o $@

%.asm.o: %.asm
	$(ASSEMBLER) $(ASFLAGS) -o $@ $(subst .asm.o,.asm,$@)

disk:all
	chmod +x $(TOOLS)/kernel/make_disk.sh
	$(TOOLS)/kernel/make_disk.sh $(TARGET) $(MOUNTPOINT) $(TARGET_IMG) || (chmod +x $(TOOLS)/kernel/cleanup.sh && $(TOOLS)/kernel/cleanup.sh $(MOUNTPOINT))

disk_dirty:all
	chmod +x $(TOOLS)/kernel/make_disk.sh
	$(TOOLS)/kernel/make_disk.sh $(TARGET) $(MOUNTPOINT) $(TARGET_IMG) yes || (chmod +x $(TOOLS)/kernel/cleanup.sh && $(TOOLS)/kernel/cleanup.sh $(MOUNTPOINT))

vmware:
	qemu-img convert $(TARGET_IMG) -O vmdk $(TARGET_VMWARE)

tools:
	chmod +x $(TOOLS)/toolchain/get_tools.sh
	$(TOOLS)/toolchain/get_tools.sh
	
clean:
# rm -f $(TARGET)/obj/*.o
	find . -name '*.o' -delete
	rm -r -f $(TARGET)/kernel.bin
#	rm -f $(TARGET_IMG) $(TARGET_VMWARE) $(TARGET_ISO)

qemu:
	qemu-system-x86_64 -d guest_errors -serial stdio -drive file=$(TARGET_IMG),format=raw,id=disk,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -m 4g -netdev user,id=mynet0 -net nic,model=rtl8139,netdev=mynet0

qemu_dbg:
	qemu-system-x86_64 -d guest_errors -no-shutdown -no-reboot -serial stdio -drive file=$(TARGET_IMG),format=raw,id=disk,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -m 8g -netdev user,id=mynet0,hostfwd=udp::5555-:69,hostfwd=tcp::5555-:69 -net nic,model=rtl8139,netdev=mynet0 -object filter-dump,id=id,netdev=mynet0,file=../../netdmp.pcapng -s -smp 4

qemu_iso:
	qemu-system-x86_64 -d guest_errors -serial stdio -drive file=$(TARGET_ISO),format=raw -m 1g -netdev user,id=mynet0 -net nic,model=rtl8139,netdev=mynet0
