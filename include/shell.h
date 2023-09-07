#ifndef SHELL_H
#define SHELL_H
#include "allocation.h"
#include "disk.h"
#include "fat32.h"
#include "kb.h"
#include "multiboot.h"
#include "shell.h"
#include "string.h"
#include "system.h"
#include "tty.h"
#include "types.h"
#include "util.h"
#include "vga.h"

void launch_shell(int n, multiboot_info_t *mbi);

#endif
