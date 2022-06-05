#!/bin/bash

# Exit (with output 1) if any commands do not succeed
set -e
set -o pipefail

# Check if there were 0 arguments supplied
if [ $# -eq 0 ]; then
    echo "No arguments supplied"
    exit 1
fi

# Check for argument "build"
if [ $1 = "build" ]; then
  # Set command tracing
  set -o xtrace

  # Variables
  COMPILER="gcc"
  LINKER="ld"
  ASSEMBLER="nasm"
  CFLAGS="-m32 -c -ffreestanding -w"
  ASFLAGS="-f elf32"
  LDFLAGS="-m elf_i386 -T src/boot/link.ld"
  OBJS=""
  OUTPUT="tmp/boot/kernel.bin"

  # Directory creations
  mkdir tmp/ -p
  mkdir tmp/boot/ -p
  mkdir tmp/obj/ -p

  # Assembly
  for i in $(find . -name *.asm); do
    ${ASSEMBLER} ${ASFLAGS} -o "tmp/obj/$( echo $i | sed 's/.*\///g;s/.asm/_asm.o/g' )" "${i}"
    OBJS+=" tmp/obj/$( echo "${i}" | sed 's/.*\///g;s/.asm/_asm.o/g' )"
  done

  # C code
  for i in $(find . -name *.c); do
    ${COMPILER} ${CFLAGS} "${i}" -o "tmp/obj/$( echo "${i}" | sed 's/.*\///g;s/.c/.o/g' )"
    OBJS+=" tmp/obj/$( echo "${i}" | sed 's/.*\///g;s/.c/.o/g' )"
  done

  # Ld linking
  ${LINKER} ${LDFLAGS} -o ${OUTPUT} ${OBJS}

  # ISO Creation
  grub-mkrescue -o cavOS.iso tmp/

# Check for argument "clear"
elif [ $1 = "clear" ]; then
  # Set command tracing
  set -o xtrace

	rm -f tmp/obj/*.o
	rm -r -f tmp/kernel.bin
elif [ $1 = "qemu" ]; then
  if ! command -v qemu-system-x86_64 &> /dev/null; then
  	echo -e "The 'qemu-system-x86_64' emulator is not installed! It is required to test the compiled ISO."
  	exit 1
  fi

  qemu-system-x86_64 -cdrom cavOS.iso -m 512M &> /dev/null
fi
