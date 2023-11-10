COMPILER = ~/opt/cross/bin/i686-elf-gcc
ARCHIVER = ~/opt/cross/bin/i686-elf-ar
CFLAGS = -std=gnu99 -ffreestanding -Wall -Wextra -fno-builtin -nostartfiles -nostdlib -c -fpic

BIN_OUTPUT = main.cav
LIB_OUTPUT = libsystem.a

all: clean compile

compile:
	$(COMPILER) $(CFLAGS) -o $(BIN_OUTPUT) main.c
	$(ARCHIVER) rcs $(LIB_OUTPUT) $(BIN_OUTPUT)

clean:
	rm -f $(LIB_OUTPUT) $(BIN_OUTPUT)