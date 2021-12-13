#include "../../../include/shell.h"
#include "../../../include/math.h"

void launch_shell(int n)
{
	string ch = (string) malloc(200); // util.h
    string data[64];
	string prompt = "$ ";
	int counter = 0;
	do
	{
			//printf("$ ");
			printf("%s", prompt);
			//printf(int_to_string(n));
			//printf(")> ");
		    ch = readStr(); //memory_copy(readStr(), ch,100);
		    if(cmdEql(ch,"cmd"))
		    {
		            printf("\nYou are already in cmd. A new recursive shell is opened\n");
					launch_shell(n+1);
		    }
		    else if(cmdEql(ch,"clear"))
		    {
		            clearScreen();
		    }
		    else if(cmdEql(ch,"exit"))
		    {
		    	//printf("\nGood Bye!\n");
		    }
		    else if(cmdEql(ch,"echo"))
		    {
		    	echo(ch);
		    }
			else if(cmdEql(ch,"test"))
		    {
				test(ch);
		    }
			else if(cmdEql(ch, "snake"))
			{
				//clearScreen();
				//snake(80, 25);
			}
		    else if(cmdEql(ch,"help"))
		    {
		    	help();
		    }
		    else if(cmdEql(ch,"color"))
		    {
		    	set_background_color();
		    }
			else if(cmdEql(ch,"fetch"))
		    {
				fetch();
		    }

		    else
		    {
				printf("\n%s isn't a valid command\n", ch);
		        //printf("$ ");
		    }
	} while (!cmdEql(ch,"exit"));
}

void test(string ch) {
	//a
}

/*void snake(uint8 rows, uint8 cols) {
	uint8 isPlaying = 1;

	while (isPlaying) {
		char prefixR = '>';
		char prefixL = '<';
		char seperator = '=';

		uint8 posX = 1; // Horizontal angle
		uint8 posY = 1; // Vertical angle
		string cursorCurrent;
		cursorCurrent[0] = seperator;
		cursorCurrent[1] = prefixR;

		clearScreen();
		printf("%s", cursorCurrent);
		char test = readWASD();

		if (test == 'q') {
			isPlaying--;
			clearScreen();
		}
	}
}*/

void echo(string ch)
{
	string arg = whatIsArgMain(ch);

	printf("\n%s\n", arg);
}

int fibo(int n)
{
	if(n <2)
		return 1;
	else
		return fibo(n-1) + fibo(n-2);
}
void printf_matrix(int matrix[][100],int rows,int cols)
{
	int i =0;
	int j = 0;
	for (i = 0;i<rows;i++)
	{
		for(j =0;j<cols;j++)
		{
			printf(int_to_string(matrix[i][j]));
			printf("   ");
		}
		printf("\n");
	}
}
void set_background_color()
{
	printf("\nColor codes : ");
	printf("\n0 : black");
	printf_colored("\n1 : blue",1,0);   // screen.h
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

void fetch() {
	printf("\nname: cavOS");
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
/*
	printf("\n\nJoke Commands:");
	printf("\nspam      : Spam amogus to the shell");
*/

	printf("\n");
}
