#include "../../include/kb.h"
#include "../../include/vga.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"
#include "../../include/multiboot.h"

int kmain(uint32 magic, multiboot_info_t *mbi)
{
    (void) magic; /* Silence compiler warning since magic isn't used */
	isr_install();
	clearScreen();
	printf("Welcome to cavOS! The OS that reminds you of how good computers \nwere back then.. Anyway, just execute any command you want\n'help' is your friend :)\n\n");
	launch_shell(0, mbi);
    clearScreen();
    //draw(mbi, 10, 255, 255, 255);
    //while (1==1) {}
}
