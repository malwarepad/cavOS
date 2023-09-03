#ifndef SHELL_H
#define SHELL_H
#include "system.h"
#include "string.h"
#include "kb.h"
#include "tty.h"
#include "types.h"
#include "util.h"
#include "multiboot.h"
#include "disk.h"
#include "fat32.h"
#include "shell.h"
#include "vga.h"

void launch_shell(int n, multiboot_info_t *mbi);

#endif
