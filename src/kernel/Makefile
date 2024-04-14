COMPILER = ~/opt/cross/bin/x86_64-cavos-gcc
LINKER = ~/opt/cross/bin/x86_64-cavos-ld
ASSEMBLER = nasm
# -w(no warnings) -g(dev)
CFLAGS = -m64 -c -ffreestanding -Wall -fcommon -Iinclude/ -fPIE -mno-80387 \
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

OBJS = $(TARGET)/obj/kc.o $(TARGET)/obj/idt.o $(TARGET)/obj/printf.o $(TARGET)/obj/isr.o $(TARGET)/obj/kb.o $(TARGET)/obj/string.o $(TARGET)/obj/system.o $(TARGET)/obj/util.o $(TARGET)/obj/shell.o $(TARGET)/obj/disk.o $(TARGET)/obj/rtc.o $(TARGET)/obj/testing.o $(TARGET)/obj/pmm.o $(TARGET)/obj/gdt.o $(TARGET)/obj/timer.o $(TARGET)/obj/schedule.o $(TARGET)/obj/pci.o $(TARGET)/obj/pci_id.o $(TARGET)/obj/israsm.o $(TARGET)/obj/task.o $(TARGET)/obj/stack.o $(TARGET)/obj/vga.o $(TARGET)/obj/console.o $(TARGET)/obj/serial.o $(TARGET)/obj/elf.o $(TARGET)/obj/paging.o $(TARGET)/obj/vmm.o $(TARGET)/obj/syscalls.o $(TARGET)/obj/ne2k.o $(TARGET)/obj/nic_controller.o $(TARGET)/obj/arp.o $(TARGET)/obj/rtl8139.o $(TARGET)/obj/bitmap.o $(TARGET)/obj/ipv4.o $(TARGET)/obj/udp.o $(TARGET)/obj/checksum.o $(TARGET)/obj/icmp.o $(TARGET)/obj/dhcp.o $(TARGET)/obj/tcp.o $(TARGET)/obj/socket.o $(TARGET)/obj/fs_controller.o $(TARGET)/obj/ahci.o $(TARGET)/obj/rtl8169.o $(TARGET)/obj/linked_list.o $(TARGET)/obj/bootloaderParser.o $(TARGET)/obj/md5.o $(TARGET)/obj/psf.o $(TARGET)/obj/fb.o $(TARGET)/obj/malloc.o $(TARGET)/obj/malloc_glue.o $(TARGET)/obj/sseasm.o $(TARGET)/obj/diskio.o $(TARGET)/obj/ff.o $(TARGET)/obj/ffsystem.o $(TARGET)/obj/ffunicode.o
OUTPUT = $(TARGET)/boot/kernel.bin

$(TARGET)/obj:
	mkdir -p $(TARGET)/obj

all:$(TARGET)/obj $(OBJS)
	mkdir $(TARGET)/ -p
	mkdir $(TARGET)/boot/ -p
	$(LINKER) $(LDFLAGS) -o $(OUTPUT) $(OBJS)

$(TARGET)/obj/israsm.o:cpu/isr.asm
	mkdir $(TARGET)/obj/ -p
	$(ASSEMBLER) $(ASFLAGS) -o $(TARGET)/obj/israsm.o cpu/isr.asm

$(TARGET)/obj/sseasm.o:cpu/sse.asm
	mkdir $(TARGET)/obj/ -p
	$(ASSEMBLER) $(ASFLAGS) -o $(TARGET)/obj/sseasm.o cpu/sse.asm
	
$(TARGET)/obj/syscalls.o:cpu/syscalls.c
	$(COMPILER) $(CFLAGS) cpu/syscalls.c -o $(TARGET)/obj/syscalls.o 
	
$(TARGET)/obj/vmm.o:memory/vmm.c
	$(COMPILER) $(CFLAGS) memory/vmm.c -o $(TARGET)/obj/vmm.o 
	
$(TARGET)/obj/paging.o:memory/paging.c
	$(COMPILER) $(CFLAGS) memory/paging.c -o $(TARGET)/obj/paging.o 
	
$(TARGET)/obj/task.o:multitasking/task.c
	$(COMPILER) $(CFLAGS) multitasking/task.c -o $(TARGET)/obj/task.o 
	
$(TARGET)/obj/stack.o:multitasking/stack.c
	$(COMPILER) $(CFLAGS) multitasking/stack.c -o $(TARGET)/obj/stack.o 
	
$(TARGET)/obj/elf.o:utilities/elf.c
	$(COMPILER) $(CFLAGS) utilities/elf.c -o $(TARGET)/obj/elf.o 
	
$(TARGET)/obj/psf.o:utilities/psf.c
	$(COMPILER) $(CFLAGS) utilities/psf.c -o $(TARGET)/obj/psf.o 
	
$(TARGET)/obj/ne2k.o:drivers/nics/ne2k.c
	$(COMPILER) $(CFLAGS) drivers/nics/ne2k.c -o $(TARGET)/obj/ne2k.o 
	
$(TARGET)/obj/rtl8139.o:drivers/nics/rtl8139.c
	$(COMPILER) $(CFLAGS) drivers/nics/rtl8139.c -o $(TARGET)/obj/rtl8139.o 
	
$(TARGET)/obj/rtl8169.o:drivers/nics/rtl8169.c
	$(COMPILER) $(CFLAGS) drivers/nics/rtl8169.c -o $(TARGET)/obj/rtl8169.o 
	
$(TARGET)/obj/nic_controller.o:drivers/nics/nic_controller.c
	$(COMPILER) $(CFLAGS) drivers/nics/nic_controller.c -o $(TARGET)/obj/nic_controller.o 
	
$(TARGET)/obj/serial.o:drivers/serial.c
	$(COMPILER) $(CFLAGS) drivers/serial.c -o $(TARGET)/obj/serial.o 
	
$(TARGET)/obj/schedule.o:multitasking/schedule.c
	$(COMPILER) $(CFLAGS) multitasking/schedule.c -o $(TARGET)/obj/schedule.o 
	
$(TARGET)/obj/console.o:graphical/console.c
	$(COMPILER) $(CFLAGS) graphical/console.c -o $(TARGET)/obj/console.o 
	
$(TARGET)/obj/fb.o:graphical/fb.c
	$(COMPILER) $(CFLAGS) graphical/fb.c -o $(TARGET)/obj/fb.o 
	
$(TARGET)/obj/malloc.o:memory/malloc.c
	$(COMPILER) $(CFLAGS) memory/malloc.c -o $(TARGET)/obj/malloc.o -DHAVE_MMAP=0 -DLACKS_TIME_H=1 -DLACKS_SYS_PARAM_H=1 -LACKS_STRING_H=0 -Dmalloc_getpagesize=4096 -DNO_MALLOC_STATS=1 -DMORECORE_CONTIGUOUS=0
	
$(TARGET)/obj/malloc_glue.o:memory/malloc_glue.c
	$(COMPILER) $(CFLAGS) memory/malloc_glue.c -o $(TARGET)/obj/malloc_glue.o 
	
$(TARGET)/obj/timer.o:cpu/timer.c
	$(COMPILER) $(CFLAGS) cpu/timer.c -o $(TARGET)/obj/timer.o 
	
$(TARGET)/obj/pci.o:drivers/pci.c
	$(COMPILER) $(CFLAGS) drivers/pci.c -o $(TARGET)/obj/pci.o 
	
$(TARGET)/obj/pci_id.o:drivers/pci_id.c
	$(COMPILER) $(CFLAGS) drivers/pci_id.c -o $(TARGET)/obj/pci_id.o 
	
$(TARGET)/obj/gdt.o:cpu/gdt.c
	$(COMPILER) $(CFLAGS) cpu/gdt.c -o $(TARGET)/obj/gdt.o 

$(TARGET)/obj/arp.o:networking/arp.c
	$(COMPILER) $(CFLAGS) networking/arp.c -o $(TARGET)/obj/arp.o 

$(TARGET)/obj/ipv4.o:networking/ipv4.c
	$(COMPILER) $(CFLAGS) networking/ipv4.c -o $(TARGET)/obj/ipv4.o 

$(TARGET)/obj/udp.o:networking/udp.c
	$(COMPILER) $(CFLAGS) networking/udp.c -o $(TARGET)/obj/udp.o 

$(TARGET)/obj/tcp.o:networking/tcp.c
	$(COMPILER) $(CFLAGS) networking/tcp.c -o $(TARGET)/obj/tcp.o 

$(TARGET)/obj/socket.o:networking/socket.c
	$(COMPILER) $(CFLAGS) networking/socket.c -o $(TARGET)/obj/socket.o 

$(TARGET)/obj/checksum.o:networking/checksum.c
	$(COMPILER) $(CFLAGS) networking/checksum.c -o $(TARGET)/obj/checksum.o 

$(TARGET)/obj/icmp.o:networking/icmp.c
	$(COMPILER) $(CFLAGS) networking/icmp.c -o $(TARGET)/obj/icmp.o 

$(TARGET)/obj/dhcp.o:networking/dhcp.c
	$(COMPILER) $(CFLAGS) networking/dhcp.c -o $(TARGET)/obj/dhcp.o 
		
$(TARGET)/obj/kc.o:entry/kernel.c
	$(COMPILER) $(CFLAGS) entry/kernel.c -o $(TARGET)/obj/kc.o 
		
$(TARGET)/obj/bootloaderParser.o:entry/bootloader.c
	$(COMPILER) $(CFLAGS) entry/bootloader.c -o $(TARGET)/obj/bootloaderParser.o
		
$(TARGET)/obj/idt.o:cpu/idt.c
	$(COMPILER) $(CFLAGS) cpu/idt.c -o $(TARGET)/obj/idt.o 

$(TARGET)/obj/kb.o:drivers/kb.c
	$(COMPILER) $(CFLAGS) drivers/kb.c -o $(TARGET)/obj/kb.o

$(TARGET)/obj/pmm.o:memory/pmm.c
	$(COMPILER) $(CFLAGS) memory/pmm.c -o $(TARGET)/obj/pmm.o

$(TARGET)/obj/isr.o:cpu/isr.c
	$(COMPILER) $(CFLAGS) cpu/isr.c -o $(TARGET)/obj/isr.o

$(TARGET)/obj/vga.o:drivers/vga.c
	$(COMPILER) $(CFLAGS) drivers/vga.c -o $(TARGET)/obj/vga.o

$(TARGET)/obj/printf.o:drivers/printf.c
	$(COMPILER) $(CFLAGS) drivers/printf.c -o $(TARGET)/obj/printf.o -DPRINTF_INCLUDE_CONFIG_H=1

$(TARGET)/obj/ahci.o:drivers/ahci.c
	$(COMPILER) $(CFLAGS) drivers/ahci.c -o $(TARGET)/obj/ahci.o

$(TARGET)/obj/disk.o:drivers/disk.c
	$(COMPILER) $(CFLAGS) drivers/disk.c -o $(TARGET)/obj/disk.o
	
$(TARGET)/obj/fs_controller.o:filesystems/fs_controller.c
	$(COMPILER) $(CFLAGS) filesystems/fs_controller.c -o $(TARGET)/obj/fs_controller.o
	
$(TARGET)/obj/diskio.o:filesystems/fatfs/diskio.c
	$(COMPILER) $(CFLAGS) filesystems/fatfs/diskio.c -o $(TARGET)/obj/diskio.o
	
$(TARGET)/obj/ff.o:filesystems/fatfs/ff.c
	$(COMPILER) $(CFLAGS) filesystems/fatfs/ff.c -o $(TARGET)/obj/ff.o
	
$(TARGET)/obj/ffsystem.o:filesystems/fatfs/ffsystem.c
	$(COMPILER) $(CFLAGS) filesystems/fatfs/ffsystem.c -o $(TARGET)/obj/ffsystem.o
	
$(TARGET)/obj/ffunicode.o:filesystems/fatfs/ffunicode.c
	$(COMPILER) $(CFLAGS) filesystems/fatfs/ffunicode.c -o $(TARGET)/obj/ffunicode.o
		
$(TARGET)/obj/rtc.o:cpu/rtc.c
	$(COMPILER) $(CFLAGS) cpu/rtc.c -o $(TARGET)/obj/rtc.o

$(TARGET)/obj/bitmap.o:utilities/data_structures/bitmap.c
	$(COMPILER) $(CFLAGS) utilities/data_structures/bitmap.c -o $(TARGET)/obj/bitmap.o

$(TARGET)/obj/linked_list.o:utilities/data_structures/linked_list.c
	$(COMPILER) $(CFLAGS) utilities/data_structures/linked_list.c -o $(TARGET)/obj/linked_list.o

$(TARGET)/obj/string.o:utilities/shell/string.c
	$(COMPILER) $(CFLAGS) utilities/shell/string.c -o $(TARGET)/obj/string.o

$(TARGET)/obj/system.o:cpu/system.c
	$(COMPILER) $(CFLAGS) cpu/system.c -o $(TARGET)/obj/system.o

$(TARGET)/obj/util.o:utilities/util.c
	$(COMPILER) $(CFLAGS) utilities/util.c -o $(TARGET)/obj/util.o

$(TARGET)/obj/md5.o:utilities/md5.c
	$(COMPILER) $(CFLAGS) utilities/md5.c -o $(TARGET)/obj/md5.o

$(TARGET)/obj/shell.o:utilities/shell/shell.c
	$(COMPILER) $(CFLAGS) utilities/shell/shell.c -o $(TARGET)/obj/shell.o

$(TARGET)/obj/testing.o:entry/testing.c
	$(COMPILER) $(CFLAGS) entry/testing.c -o $(TARGET)/obj/testing.o

disk:all
	chmod +x $(TOOLS)/make_disk.sh
	$(TOOLS)/make_disk.sh $(TARGET) $(MOUNTPOINT) $(TARGET_IMG) || (chmod +x $(TOOLS)/cleanup.sh && $(TOOLS)/cleanup.sh $(MOUNTPOINT))

vmware:disk
	qemu-img convert $(TARGET_IMG) -O vmdk $(TARGET_VMWARE)

tools:
	chmod +x $(TOOLS)/get_tools.sh
	$(TOOLS)/get_tools.sh
	
clean:
	rm -f $(TARGET)/obj/*.o
	rm -r -f $(TARGET)/kernel.bin
	rm -f $(TARGET_IMG) $(TARGET_VMWARE) $(TARGET_ISO)

qemu:
	qemu-system-x86_64 -d guest_errors -serial stdio -drive file=$(TARGET_IMG),format=raw,id=disk,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -m 1g -netdev user,id=mynet0 -net nic,model=rtl8139,netdev=mynet0

qemu_dbg:
	qemu-system-x86_64 -d guest_errors -no-shutdown -no-reboot -serial stdio -drive file=$(TARGET_IMG),format=raw,id=disk,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -m 8g -netdev user,id=mynet0,hostfwd=udp::5555-:69,hostfwd=tcp::5555-:69 -net nic,model=rtl8139,netdev=mynet0 -object filter-dump,id=id,netdev=mynet0,file=../../netdmp.pcapng -s

qemu_iso:
	qemu-system-x86_64 -d guest_errors -serial stdio -drive file=$(TARGET_ISO),format=raw -m 1g -netdev user,id=mynet0 -net nic,model=rtl8139,netdev=mynet0

dev:clean disk qemu_dbg
