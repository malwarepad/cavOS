#include "multiboot.h"
#include "tty.h"

void drawPixel(multiboot_info_t *mbi, uint32 count, uint32 r, uint32 g, uint32 b);
void draw(multiboot_info_t *mbi, uint32 count, uint8 r, uint32 g, uint32 b);