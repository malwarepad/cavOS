COMPILER = ~/opt/cross/bin/i686-elf-gcc
LINKER = ~/opt/cross/bin/i686-elf-ld
ASSEMBLER = nasm
CFLAGS = -m32 -c -ffreestanding -w -fcommon -Iinclude/ -g
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T boot/link.ld

TARGET = ../../target
MOUNTPOINT = /cavosmnt
TOOLS = ../../tools

TARGET_IMG = ../../disk.img
TARGET_VMWARE = ../../disk.vmdk
TARGET_ISO = ../../cavOS.iso

OBJS = $(TARGET)/obj/kasm.o $(TARGET)/obj/kc.o $(TARGET)/obj/idt.o $(TARGET)/obj/ata.o $(TARGET)/obj/printf.o $(TARGET)/obj/isr.o $(TARGET)/obj/kb.o $(TARGET)/obj/string.o $(TARGET)/obj/system.o $(TARGET)/obj/util.o $(TARGET)/obj/shell.o $(TARGET)/obj/disk.o $(TARGET)/obj/fat32.o $(TARGET)/obj/rtc.o $(TARGET)/obj/testing.o $(TARGET)/obj/pmm.o $(TARGET)/obj/gdtasm.o $(TARGET)/obj/gdt.o $(TARGET)/obj/timer.o $(TARGET)/obj/schedule.o $(TARGET)/obj/pci.o $(TARGET)/obj/israsm.o $(TARGET)/obj/taskasm.o $(TARGET)/obj/task.o $(TARGET)/obj/liballoc.o $(TARGET)/obj/vga.o $(TARGET)/obj/console.o $(TARGET)/obj/backupconsole.o $(TARGET)/obj/serial.o $(TARGET)/obj/elf.o $(TARGET)/obj/paging.o $(TARGET)/obj/vmm.o $(TARGET)/obj/syscalls.o $(TARGET)/obj/fat32_utils.o $(TARGET)/obj/fat32_fat.o $(TARGET)/obj/fat32_lfn.o $(TARGET)/obj/ne2k.o $(TARGET)/obj/nic_controller.o $(TARGET)/obj/arp.o $(TARGET)/obj/rtl8139.o $(TARGET)/obj/bitmap.o $(TARGET)/obj/ipv4.o $(TARGET)/obj/udp.o $(TARGET)/obj/checksum.o $(TARGET)/obj/icmp.o $(TARGET)/obj/dhcp.o $(TARGET)/obj/tcp.o
OUTPUT = $(TARGET)/boot/kernel.bin

all:$(OBJS)
	mkdir $(TARGET)/ -p
	mkdir $(TARGET)/boot/ -p
	$(LINKER) $(LDFLAGS) -o $(OUTPUT) $(OBJS)

$(TARGET)/obj/kasm.o:boot/kernel.asm
	mkdir $(TARGET)/obj/ -p
	$(ASSEMBLER) $(ASFLAGS) -o $(TARGET)/obj/kasm.o boot/kernel.asm

$(TARGET)/obj/gdtasm.o:cpu/gdt.asm
	mkdir $(TARGET)/obj/ -p
	$(ASSEMBLER) $(ASFLAGS) -o $(TARGET)/obj/gdtasm.o cpu/gdt.asm

$(TARGET)/obj/israsm.o:cpu/isr.asm
	mkdir $(TARGET)/obj/ -p
	$(ASSEMBLER) $(ASFLAGS) -o $(TARGET)/obj/israsm.o cpu/isr.asm

$(TARGET)/obj/taskasm.o:multitasking/task.asm
	mkdir $(TARGET)/obj/ -p
	$(ASSEMBLER) $(ASFLAGS) -o $(TARGET)/obj/taskasm.o multitasking/task.asm
	
$(TARGET)/obj/syscalls.o:cpu/syscalls.c
	$(COMPILER) $(CFLAGS) cpu/syscalls.c -o $(TARGET)/obj/syscalls.o 
	
$(TARGET)/obj/vmm.o:memory/vmm.c
	$(COMPILER) $(CFLAGS) memory/vmm.c -o $(TARGET)/obj/vmm.o 
	
$(TARGET)/obj/paging.o:memory/paging.c
	$(COMPILER) $(CFLAGS) memory/paging.c -o $(TARGET)/obj/paging.o 
	
$(TARGET)/obj/task.o:multitasking/task.c
	$(COMPILER) $(CFLAGS) multitasking/task.c -o $(TARGET)/obj/task.o 
	
$(TARGET)/obj/elf.o:utilities/elf.c
	$(COMPILER) $(CFLAGS) utilities/elf.c -o $(TARGET)/obj/elf.o 
	
$(TARGET)/obj/ne2k.o:drivers/nics/ne2k.c
	$(COMPILER) $(CFLAGS) drivers/nics/ne2k.c -o $(TARGET)/obj/ne2k.o 
	
$(TARGET)/obj/rtl8139.o:drivers/nics/rtl8139.c
	$(COMPILER) $(CFLAGS) drivers/nics/rtl8139.c -o $(TARGET)/obj/rtl8139.o 
	
$(TARGET)/obj/nic_controller.o:drivers/nics/nic_controller.c
	$(COMPILER) $(CFLAGS) drivers/nics/nic_controller.c -o $(TARGET)/obj/nic_controller.o 
	
$(TARGET)/obj/serial.o:drivers/serial.c
	$(COMPILER) $(CFLAGS) drivers/serial.c -o $(TARGET)/obj/serial.o 
	
$(TARGET)/obj/schedule.o:multitasking/schedule.c
	$(COMPILER) $(CFLAGS) multitasking/schedule.c -o $(TARGET)/obj/schedule.o 
	
$(TARGET)/obj/console.o:graphical/console.c
	$(COMPILER) $(CFLAGS) graphical/console.c -o $(TARGET)/obj/console.o 
	
$(TARGET)/obj/backupconsole.o:graphical/backupconsole.c
	$(COMPILER) $(CFLAGS) graphical/backupconsole.c -o $(TARGET)/obj/backupconsole.o 
	
$(TARGET)/obj/liballoc.o:memory/liballoc.c
	$(COMPILER) $(CFLAGS) memory/liballoc.c -o $(TARGET)/obj/liballoc.o 
	
$(TARGET)/obj/timer.o:cpu/timer.c
	$(COMPILER) $(CFLAGS) cpu/timer.c -o $(TARGET)/obj/timer.o 
	
$(TARGET)/obj/pci.o:drivers/pci.c
	$(COMPILER) $(CFLAGS) drivers/pci.c -o $(TARGET)/obj/pci.o 
	
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

$(TARGET)/obj/checksum.o:networking/checksum.c
	$(COMPILER) $(CFLAGS) networking/checksum.c -o $(TARGET)/obj/checksum.o 

$(TARGET)/obj/icmp.o:networking/icmp.c
	$(COMPILER) $(CFLAGS) networking/icmp.c -o $(TARGET)/obj/icmp.o 

$(TARGET)/obj/dhcp.o:networking/dhcp.c
	$(COMPILER) $(CFLAGS) networking/dhcp.c -o $(TARGET)/obj/dhcp.o 
		
$(TARGET)/obj/kc.o:entry/kernel.c
	$(COMPILER) $(CFLAGS) entry/kernel.c -o $(TARGET)/obj/kc.o 
		
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

$(TARGET)/obj/ata.o:drivers/ata.c
	$(COMPILER) $(CFLAGS) drivers/ata.c -o $(TARGET)/obj/ata.o

$(TARGET)/obj/disk.o:drivers/disk.c
	$(COMPILER) $(CFLAGS) drivers/disk.c -o $(TARGET)/obj/disk.o
	
$(TARGET)/obj/fat32.o:filesystems/fat32/fat32.c
	$(COMPILER) $(CFLAGS) filesystems/fat32/fat32.c -o $(TARGET)/obj/fat32.o
	
$(TARGET)/obj/fat32_utils.o:filesystems/fat32/fatutils.c
	$(COMPILER) $(CFLAGS) filesystems/fat32/fatutils.c -o $(TARGET)/obj/fat32_utils.o
	
$(TARGET)/obj/fat32_fat.o:filesystems/fat32/fat.c
	$(COMPILER) $(CFLAGS) filesystems/fat32/fat.c -o $(TARGET)/obj/fat32_fat.o
	
$(TARGET)/obj/fat32_lfn.o:filesystems/fat32/lfn.c
	$(COMPILER) $(CFLAGS) filesystems/fat32/lfn.c -o $(TARGET)/obj/fat32_lfn.o
		
$(TARGET)/obj/rtc.o:cpu/rtc.c
	$(COMPILER) $(CFLAGS) cpu/rtc.c -o $(TARGET)/obj/rtc.o

$(TARGET)/obj/bitmap.o:utilities/data_structures/bitmap.c
	$(COMPILER) $(CFLAGS) utilities/data_structures/bitmap.c -o $(TARGET)/obj/bitmap.o

$(TARGET)/obj/string.o:utilities/shell/string.c
	$(COMPILER) $(CFLAGS) utilities/shell/string.c -o $(TARGET)/obj/string.o

$(TARGET)/obj/system.o:cpu/system.c
	$(COMPILER) $(CFLAGS) cpu/system.c -o $(TARGET)/obj/system.o

$(TARGET)/obj/util.o:utilities/util.c
	$(COMPILER) $(CFLAGS) utilities/util.c -o $(TARGET)/obj/util.o

$(TARGET)/obj/shell.o:utilities/shell/shell.c
	$(COMPILER) $(CFLAGS) utilities/shell/shell.c -o $(TARGET)/obj/shell.o

$(TARGET)/obj/testing.o:entry/testing.c
	$(COMPILER) $(CFLAGS) entry/testing.c -o $(TARGET)/obj/testing.o

iso:all 
	grub-mkrescue -o $(TARGET_ISO) $(TARGET)/

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
	qemu-system-i386 -d guest_errors -serial stdio -drive file=$(TARGET_IMG),format=raw -m 1g -netdev user,id=mynet0 -net nic,model=rtl8139,netdev=mynet0

qemu_dbg:
	qemu-system-i386 -d guest_errors -no-shutdown -no-reboot -serial stdio -drive file=$(TARGET_IMG),format=raw -m 1g -netdev user,id=mynet0 -net nic,model=rtl8139,netdev=mynet0 -object filter-dump,id=id,netdev=mynet0,file=../../netdmp.pcapng

qemu_iso:
	qemu-system-i386 -d guest_errors -serial stdio -drive file=$(TARGET_ISO),format=raw -m 1g -netdev user,id=mynet0 -net nic,model=rtl8139,netdev=mynet0

dev:clean disk qemu_dbg
