COMPILER = ~/opt/cross/bin/x86_64-cavos-gcc
CFLAGS = -std=gnu99 -Wall -Wextra -static
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
