#include "../../include/tty.h"
#include "../../include/multiboot.h"

void drawPixel(multiboot_info_t *mbi, uint32 count, uint32 r, uint32 g, uint32 b) { //drawPixel(mbi, 0, 0x7800);
    string screen = (string) mbi->framebuffer_addr;
    //unsigned where = width*mbi->framebuffer_width + height*mbi->framebuffer_pitch;
    
	/*screen[count] = color & 255;              // BLUE
    screen[count + 1] = (color >> 8) & 255;   // GREEN
    screen[count + 2] = (color >> 16) & 255;  // RED*/

	screen[count] = r;              // BLUE
    screen[count + 1] = g;   // GREEN
    screen[count + 2] = b;  // RED
}

void draw(multiboot_info_t *mbi, uint32 count, uint8 r, uint32 g, uint32 b) {
    string screen = (string) mbi->framebuffer_addr;
	uint8 cnt_final = count * 3;

	screen[cnt_final] = r;              // BLUE
    screen[cnt_final + 1] = g;   // GREEN
    screen[cnt_final + 2] = b;  // RED
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
