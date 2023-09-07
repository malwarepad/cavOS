COMPILER = ~/opt/cross/bin/i686-elf-gcc
LINKER = ~/opt/cross/bin/i686-elf-ld
ASSEMBLER = nasm
CFLAGS = -m32 -c -ffreestanding -w -fcommon
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T src/boot/link.ld

OBJS = tmp/obj/kasm.o tmp/obj/kc.o tmp/obj/idt.o tmp/obj/ata.o tmp/obj/printf.o tmp/obj/asm_ports.o tmp/obj/isr.o tmp/obj/kb.o tmp/obj/tty.o tmp/obj/vga.o tmp/obj/string.o tmp/obj/system.o tmp/obj/util.o tmp/obj/shell.o tmp/obj/disk.o tmp/obj/fat32.o tmp/obj/allocation.o tmp/obj/rtc.o
OUTPUT = tmp/boot/kernel.bin

all:$(OBJS)
	mkdir tmp/ -p
	mkdir tmp/boot/ -p
	$(LINKER) $(LDFLAGS) -o $(OUTPUT) $(OBJS)

tmp/obj/kasm.o:src/boot/kernel.asm
	mkdir tmp/obj/ -p
	$(ASSEMBLER) $(ASFLAGS) -o tmp/obj/kasm.o src/boot/kernel.asm
	
tmp/obj/kc.o:src/entry/kernel.c
	$(COMPILER) $(CFLAGS) src/entry/kernel.c -o tmp/obj/kc.o 
		
tmp/obj/idt.o:src/cpu/idt.c
	$(COMPILER) $(CFLAGS) src/cpu/idt.c -o tmp/obj/idt.o 

tmp/obj/kb.o:src/drivers/kb.c
	$(COMPILER) $(CFLAGS) src/drivers/kb.c -o tmp/obj/kb.o

tmp/obj/isr.o:src/cpu/isr.c
	$(COMPILER) $(CFLAGS) src/cpu/isr.c -o tmp/obj/isr.o

tmp/obj/tty.o:src/drivers/tty.c
	$(COMPILER) $(CFLAGS) src/drivers/tty.c -o tmp/obj/tty.o

tmp/obj/vga.o:src/drivers/vga.c
	$(COMPILER) $(CFLAGS) src/drivers/vga.c -o tmp/obj/vga.o

tmp/obj/printf.o:src/drivers/vga.c
	$(COMPILER) $(CFLAGS) src/drivers/printf.c -o tmp/obj/printf.o

tmp/obj/ata.o:src/drivers/vga.c
	$(COMPILER) $(CFLAGS) src/drivers/ata.c -o tmp/obj/ata.o

tmp/obj/disk.o:src/drivers/disk.c
	$(COMPILER) $(CFLAGS) src/drivers/disk.c -o tmp/obj/disk.o
	
tmp/obj/fat32.o:src/drivers/fat32.c
	$(COMPILER) $(CFLAGS) src/drivers/fat32.c -o tmp/obj/fat32.o
	
tmp/obj/allocation.o:src/memory/allocation.c
	$(COMPILER) $(CFLAGS) src/memory/allocation.c -o tmp/obj/allocation.o
	
tmp/obj/rtc.o:src/cpu/rtc.c
	$(COMPILER) $(CFLAGS) src/cpu/rtc.c -o tmp/obj/rtc.o

tmp/obj/string.o:src/utilities/shell/string.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/string.c -o tmp/obj/string.o

tmp/obj/system.o:src/cpu/system.c
	$(COMPILER) $(CFLAGS) src/cpu/system.c -o tmp/obj/system.o

tmp/obj/util.o:src/utilities/util.c
	$(COMPILER) $(CFLAGS) src/utilities/util.c -o tmp/obj/util.o

tmp/obj/asm_ports.o:src/utilities/util.c
	$(COMPILER) $(CFLAGS) src/boot/asm_ports/asm_ports.c -o tmp/obj/asm_ports.o

tmp/obj/shell.o:src/utilities/shell/shell.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/shell.c -o tmp/obj/shell.o

iso:all 
	grub-mkrescue -o cavOS.iso tmp/

disk:all
	dd if=/dev/zero of=disk.img bs=512 count=131072
	parted disk.img mklabel msdos mkpart primary ext4 2048s 100% set 1 boot on
	losetup /dev/loop101 disk.img
	losetup /dev/loop102 disk.img -o 1048576
	mkdosfs -F32 -f 2 /dev/loop102
	fatlabel /dev/loop102 CAVOS
	mount /dev/loop102 /mnt
	grub-install --root-directory=/mnt --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/loop101
	cp -r tmp/* /mnt/
	umount /mnt
	losetup -d /dev/loop101
	losetup -d /dev/loop102

tools:
	chmod +x getTools.sh
	./getTools.sh
	
clean:
	rm -f tmp/obj/*.o
	rm -r -f tmp/kernel.bin

qemu:
	./qemu.sh
