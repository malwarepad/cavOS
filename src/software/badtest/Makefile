COMPILER = ~/opt/cross/bin/x86_64-cavos-gcc
CFLAGS = -std=gnu99 -Wall -Wextra -ffreestanding -nostartfiles -nostdlib
OUTPUT = a.cav
TARGET = ../../../target/software/

all: clean compile install

compile:
	$(COMPILER) badtest.c start.S -o $(OUTPUT) $(CFLAGS)

install:
	mkdir -p $(TARGET)
	cp $(OUTPUT) $(TARGET)

clean:
	rm -f $(OUTPUT)
