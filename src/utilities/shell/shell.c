#include "../../../include/shell.h"
#include "../../../include/multiboot.h"
#include "../../../include/vga.h"

// Shell driver
// Copyright (C) 2022 Panagiotis

void launch_shell(int n, multiboot_info_t *mbi)
{
	string ch = (string) malloc(200);
    string data[64];
	string prompt = "$ ";
	do
	{
		printf(mbi, prompt);
		ch = readStr(mbi); //memory_copy(readStr(mbi), ch,100);
		if(cmdEql(ch,"cmd"))
		{
			printf(mbi, "\nYou are already in cmd. A new recursive shell is opened\n");
			launch_shell(n+1, mbi);
		}
		else if(cmdEql(ch,"clear"))
		{
			clearScreen(mbi);
		}
		else if(cmdEql(ch,"echo"))
		{
			echo(mbi, ch);
		}
		else if(cmdEql(ch,"help"))
		{
			help(mbi);
		}
		else if(cmdEql(ch,"fetch"))
		{
			fetch(mbi);
		}
		else if(cmdEql(ch, "draw")) {
			drawRect(mbi, 10, 10, 500, 200, 255, 255, 255);
		}
		else
		{
			printf(mbi, "\n%s isn't a valid command\n", ch);
			if(check_string(ch) && !cmdEql(ch,"exit")) {
			} else {
				printf(mbi, "\n");
			}
		}
	} while (!cmdEql(ch,"exit"));
}

void echo(multiboot_info_t *mbi, string ch)
{
	if (argSrch(ch, 1, 1) != 0) {
		printf(mbi, "\n%s\n", argSrch(ch, 1, 1));
	}
	else {
		printf(mbi, "\nEcho requires an argument!\n");
	}
}

void fetch(multiboot_info_t *mbi) {
	printf(mbi, "\nname: cavOS");
	printf(mbi, "\nmemory: %dMB", ((mbi->mem_upper) / 1024));
	printf(mbi, "\n");
}

void help(multiboot_info_t *mbi)
{
	printf(mbi, "\ncmd       : Launch a new recursive Shell");
	printf(mbi, "\nclear     : Clears the screen");
	printf(mbi, "\necho      : Reprintf a given text");
	printf(mbi, "\nexit      : Quits the current shell");
	printf(mbi, "\nfetch     : Brings you some system information");

	printf(mbi, "\n");
}
