COMPILER = ~/opt/cross/bin/i686-elf-gcc
CFLAGS = -std=gnu99 -ffreestanding -Wall -Wextra -fno-builtin -nostartfiles -nostdlib -L../../libs/system -I../../libs/system -lsystem -L../../libs/newlib/cavos-out/i686-cavos/lib -I../../libs/newlib/cavos-out/i686-cavos/include -l:libc.a
OUTPUT = testing.cav
TARGET = ../../../target/software/

all: clean compile install

compile:
	$(COMPILER) main.c utils.c -o $(OUTPUT) $(CFLAGS)

install:
	mkdir -p $(TARGET)
	cp $(OUTPUT) $(TARGET)

clean:
	rm -f $(OUTPUT)
