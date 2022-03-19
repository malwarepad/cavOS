#include "../../include/tty.h"
#include "../../include/multiboot.h"

void drawPixel(multiboot_info_t *mbi, uint32 count, uint32 color) { //drawPixel(mbi, 0, 0x7800);
    string screen = (string) mbi->framebuffer_addr;
    //unsigned where = width*mbi->framebuffer_width + height*mbi->framebuffer_pitch;
    
	screen[count] = color & 255;              // BLUE
    screen[count + 1] = (color >> 8) & 255;   // GREEN
    screen[count + 2] = (color >> 16) & 255;  // RED
}

static void fillrect(multiboot_info_t *mbi, unsigned char r, unsigned char g, unsigned   char b, unsigned char w, unsigned char h) {
    string where = (string) mbi->framebuffer_addr;
    int i, j;
 
    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            //putpixel(vram, 64 + j, 64 + i, (r << 16) + (g << 8) + b);
            where[j*mbi->framebuffer_width] = r;
            where[j*mbi->framebuffer_width + 1] = g;
            where[j*mbi->framebuffer_width + 2] = b;
        }
        where+=mbi->framebuffer_pitch;
    }
}
