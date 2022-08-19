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
			printf("%s", prompt);
		    ch = readStr(); //memory_copy(readStr(), ch,100);
		    if(strEql(ch,"cmd"))
		    {
		            printf("\nYou are already in cmd. A new recursive shell is opened\n");
					launch_shell(n+1, mbi);
		    }
		    else if(strEql(ch,"clear"))
		    {
		            clearScreen();
		    }
		    else if(strEql(ch,"echo"))
		    {
		    	echo(ch);
		    }
		    else if(strEql(ch,"help"))
		    {
		    	help();
		    }
		    else if(strEql(ch,"color"))
		    {
		    	set_background_color();
		    }
            else if(strEql(ch,"cwm")) {
                printf("\n%s\n",
                       "After taking some time off the project, I realized I was putting my time and resources on the wrong places... From now on, I will perfect the basics before trying to create something that requires them! Part of that is the window manager (cwm) which I will temporarily remove from the operating system.");
            }
			else if(strEql(ch,"fetch"))
		    {
				fetch(mbi);
		    }
		    else
		    {
				if(check_string(ch) && !strEql(ch,"exit")) {
					printf("\n%s isn't a valid command\n", ch);
				} else {
					printf("\n");
				}
		    }
	} while (!strEql(ch,"exit"));
}

void echo(string ch)
{
	printf("\nInsert argument 1: ");
    printf("\n%s\n", readStr());
}

void set_background_color()
{
	printf("\nColor codes : ");
	printf("\n0 : black");
	printf_colored("\n1 : blue",1,0);   // tty.h
	printf_colored("\n2 : green",2,0);
	printf_colored("\n3 : cyan",3,0);
	printf_colored("\n4 : red",4,0);
	printf_colored("\n5 : purple",5,0);
	printf_colored("\n6 : orange",6,0);
	printf_colored("\n7 : grey",7,0);
	printf_colored("\n8 : dark grey",8,0);
	printf_colored("\n9 : blue light",9,0);
	printf_colored("\n10 : green light",10,0);
	printf_colored("\n11 : blue lighter",11,0);
	printf_colored("\n12 : red light",12,0);
	printf_colored("\n13 : rose",13,0);
	printf_colored("\n14 : yellow",14,0);
	printf_colored("\n15 : white",15,0);

	printf("\n\n Text color ? : ");
	int text_color = str_to_int(readStr());
	printf("\n\n Background color ? : ");
	int bg_color = str_to_int(readStr());
	set_screen_color(text_color,bg_color);
	clearScreen();
}

void fetch(multiboot_info_t *mbi) {
	printf("\nname: cavOS");
	printf("\nmemory: %dMB", ((mbi->mem_upper) / 1024));
	printf("\n");
}

void help()
{
	printf("\ncmd       : Launch a new recursive Shell");
	printf("\nclear     : Clears the screen");
	printf("\necho      : Reprintf a given text");
	printf("\nexit      : Quits the current shell");
	printf("\ncolor     : Changes the colors of the terminal");
	printf("\nfetch     : Brings you some system information");

	printf("\n");
}
