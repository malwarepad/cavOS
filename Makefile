COMPILER = gcc
LINKER = ld
ASSEMBLER = nasm
CFLAGS = -m32 -c -ffreestanding -w
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T src/boot/link.ld

OBJS = tmp/obj/kasm.o tmp/obj/kc.o tmp/obj/idt.o tmp/obj/isr.o tmp/obj/kb.o tmp/obj/tty.o tmp/obj/vga.o tmp/obj/string.o tmp/obj/system.o tmp/obj/util.o tmp/obj/shell.o
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

tmp/obj/string.o:src/utilities/shell/string.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/string.c -o tmp/obj/string.o

tmp/obj/system.o:src/cpu/system.c
	$(COMPILER) $(CFLAGS) src/cpu/system.c -o tmp/obj/system.o

tmp/obj/util.o:src/utilities/util.c
	$(COMPILER) $(CFLAGS) src/utilities/util.c -o tmp/obj/util.o
	
tmp/obj/shell.o:src/utilities/shell/shell.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/shell.c -o tmp/obj/shell.o

build:all 
	grub-mkrescue -o cavOS.iso tmp/
	
clear:
	rm -f tmp/obj/*.o
	rm -r -f tmp/kernel.bin

qemu:
	./qemu.sh
