COMPILER = ~/opt/cross/bin/x86_64-cavos-gcc
CFLAGS = -std=gnu99 -Wall -Wextra -static
OUTPUT = drawimg
TARGET = ../../../target/usr/bin/

all: clean compile install

compile:
	$(COMPILER) main.c -o $(OUTPUT) $(CFLAGS)

install:
	mkdir -p $(TARGET)
	sudo cp $(OUTPUT) $(TARGET)

clean:
	rm -f $(OUTPUT)
