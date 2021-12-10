#include "../../include/system.h"

uint8 inportb (uint16 _port)
{
    	uint8 rv;
    	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    	return rv;
}

void outportb (uint16 _port, uint8 _data)
{
	__asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

void sleep(uint8 times) {
	for (uint8 i = 0; i < times * 1; i++) {
		asm("hlt");
	}
}
