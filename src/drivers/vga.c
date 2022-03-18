#include "../../include/tty.h"
#include "../../include/multiboot.h"

void drawPixel(multiboot_info_t *mbi, uint32 count, uint32 color) { //drawPixel(mbi, 0, 0x7800);
    string screen = (string) mbi->framebuffer_addr;
    //unsigned where = width*mbi->framebuffer_width + height*mbi->framebuffer_pitch;
    screen[count] = color & 255;              // BLUE
    screen[count + 1] = (color >> 8) & 255;   // GREEN
    screen[count + 2] = (color >> 16) & 255;  // RED
}