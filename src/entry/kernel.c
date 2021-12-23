#include "../../include/kb.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"

#include "../../include/mathf.h"

int kmain()
{
	isr_install();
	clearScreen();
	printf("Welcome to cavOS! The OS that reminds you of how good computers \nwere back then.. Anyway, just execute any command you want\n'help' is your friend :)\n\n");
    launch_shell(0);
}
