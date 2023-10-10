#ifndef SHELL_H
#define SHELL_H
#include "allocation.h"
#include "disk.h"
#include "fat32.h"
#include "kb.h"
#include "multiboot2.h"
#include "rtc.h"
#include "shell.h"
#include "string.h"
#include "system.h"
#include "tty.h"
#include "types.h"
#include "util.h"

void launch_shell(int n);

#endif
