COMPILER = ~/opt/cross/bin/x86_64-cavos-gcc
CFLAGS = -std=gnu99 -w -ffreestanding -nostartfiles -nostdlib
OUTPUT = a
TARGET = ../../../target/usr/bin/

all: clean compile install

compile:
	$(COMPILER) badtest.c start.S -o $(OUTPUT) $(CFLAGS)

install:
	mkdir -p $(TARGET)
	sudo cp $(OUTPUT) $(TARGET)

clean:
	rm -f $(OUTPUT)
