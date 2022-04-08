#ifndef SHELL_H
#define SHELL_H
#include "system.h"
#include "string.h"
#include "kb.h"
#include "vga.h"
#include "types.h"
#include "util.h"
#include "multiboot.h"

void launch_shell(int n, multiboot_info_t *mbi);

#endif
