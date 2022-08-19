#include "../../include/kb.h"
#include "../../include/system.h"

// Very bare bones, and basic keyboard driver 
// Copyright (C) 2022 Panagiotis

string readStr() {
    char buff;
    string buffstr = (string) malloc(200);
    uint8 i = 0;
    uint8 reading = 1;
    uint8 readTmp = 1;
    while(reading)
    {
        if(inportb(0x64) & 0x1)
        {
            //printf(int_to_string(inportb(0x60)));
            switch(inportb(0x60))
            {
                /*case 1:
                printfch('(char)27);           Escape button
                buffstr[i] = (char)27;
                i++;
                break;*/
                case 2:
                        printfch('1');
                        buffstr[i] = '1';
                        i++;
                        break;
                case 3:
                        printfch('2');
                        buffstr[i] = '2';
                        i++;
                        break;
                case 4:
                        printfch('3');
                        buffstr[i] = '3';
                        i++;
                        break;
                case 5:
                        printfch('4');
                        buffstr[i] = '4';
                        i++;
                        break;
                case 6:
                        printfch('5');
                        buffstr[i] = '5';
                        i++;
                        break;
                case 7:
                        printfch('6');
                        buffstr[i] = '6';
                        i++;
                        break;
                case 8:
                        printfch('7');
                        buffstr[i] = '7';
                        i++;
                        break;
                case 9:
                        printfch('8');
                        buffstr[i] = '8';
                        i++;
                        break;
                case 10:
                        printfch('9');
                        buffstr[i] = '9';
                        i++;
                        break;
                case 11:
                        printfch('0');
                        buffstr[i] = '0';
                        i++;
                        break;
                case 12:
                        printfch('-');
                        buffstr[i] = '-';
                        i++;
                        break;
                case 13:
                        printfch('=');
                        buffstr[i] = '=';
                        i++;
                        break;
                case 14:
                        if (i == 0) {
                                break;
                        } else {
                                printfch('\b');
                                i--;
                                buffstr[i+1] = 0;
                                buffstr[i] = 0;
                                break;
                        }

                        /*
                        i--;
                        if(i<0)
                        {
                                i = 0;
                        }
                        buffstr[i+1] = 0;
                        buffstr[i] = 0;
                        break;
                        */
        /* case 15:
                        printfch('\t');          Tab button
                        buffstr[i] = '\t';
                        i++;
                        break;*/
                case 16:
                        printfch('q');
                        buffstr[i] = 'q';
                        i++;
                        break;
                case 17:
                        printfch('w');
                        buffstr[i] = 'w';
                        i++;
                        break;
                case 18:
                        printfch('e');
                        buffstr[i] = 'e';
                        i++;
                        break;
                case 19:
                        printfch('r');
                        buffstr[i] = 'r';
                        i++;
                        break;
                case 20:
                        printfch('t');
                        buffstr[i] = 't';
                        i++;
                        break;
                case 21:
                        printfch('y');
                        buffstr[i] = 'y';
                        i++;
                        break;
                case 22:
                        printfch('u');
                        buffstr[i] = 'u';
                        i++;
                        break;
                case 23:
                        printfch('i');
                        buffstr[i] = 'i';
                        i++;
                        break;
                case 24:
                        printfch('o');
                        buffstr[i] = 'o';
                        i++;
                        break;
                case 25:
                        printfch('p');
                        buffstr[i] = 'p';
                        i++;
                        break;
                case 26:
                        printfch('[');
                        buffstr[i] = '[';
                        i++;
                        break;
                case 27:
                        printfch(']');
                        buffstr[i] = ']';
                        i++;
                        break;
                case 28:
                // printfch('\n');
                // buffstr[i] = '\n';
                        i++;
                reading = 0;
                        break;
        /*  case 29:
                        printfch('q');           Left Control
                        buffstr[i] = 'q';
                        i++;
                        break;*/
                case 30:
                        printfch('a');
                        buffstr[i] = 'a';
                        i++;
                        break;
                case 31:
                        printfch('s');
                        buffstr[i] = 's';
                        i++;
                        break;
                case 32:
                        printfch('d');
                        buffstr[i] = 'd';
                        i++;
                        break;
                case 33:
                        printfch('f');
                        buffstr[i] = 'f';
                        i++;
                        break;
                case 34:
                        printfch('g');
                        buffstr[i] = 'g';
                        i++;
                        break;
                case 35:
                        printfch('h');
                        buffstr[i] = 'h';
                        i++;
                        break;
                case 36:
                        printfch('j');
                        buffstr[i] = 'j';
                        i++;
                        break;
                case 37:
                        printfch('k');
                        buffstr[i] = 'k';
                        i++;
                        break;
                case 38:
                        printfch('l');
                        buffstr[i] = 'l';
                        i++;
                        break;
                case 39:
                        printfch(';');
                        buffstr[i] = ';';
                        i++;
                        break;
                case 40:
                        printfch((char)44);               //   Single quote (')
                        buffstr[i] = (char)44;
                        i++;
                        break;
                case 41:
                        printfch((char)44);               // Back tick (`)
                        buffstr[i] = (char)44;
                        i++;
                        break;
                case 42:                                 //Left shift
                        /*do {
                                printfch('Q');
                        } if (inportb(0x60) != 170);
                        break;*/

                        while (inportb(0x60) != 170) {
                                if(inportb(0x64) & 0x1) {
                                        switch(inportb(0x60)) {
                                                case 2:
                                                        printfch('!');
                                                        buffstr[i] = '!';
                                                        i++;
                                                        break;
                                                case 3:
                                                        printfch('$');
                                                        buffstr[i] = '@';
                                                        i++;
                                                        break;
                                                case 4:
                                                        printfch('$');
                                                        buffstr[i] = '#';
                                                        i++;
                                                        break;
                                                case 5:
                                                        printfch('$');
                                                        buffstr[i] = '$';
                                                        i++;
                                                        break;
                                                case 6:
                                                        printfch('%');
                                                        buffstr[i] = '%';
                                                        i++;
                                                        break;
                                                case 7:
                                                        printfch('^');
                                                        buffstr[i] = '^';
                                                        i++;
                                                        break;
                                                case 8:
                                                        printfch('&');
                                                        buffstr[i] = '&';
                                                        i++;
                                                        break;
                                                case 9:
                                                        printfch('*');
                                                        buffstr[i] = '*';
                                                        i++;
                                                        break;
                                                case 10:
                                                        printfch('(');
                                                        buffstr[i] = '(';
                                                        i++;
                                                        break;
                                                case 11:
                                                        printfch(')');
                                                        buffstr[i] = ')';
                                                        i++;
                                                        break;
                                                case 12:
                                                        printfch('_');
                                                        buffstr[i] = '_';
                                                        i++;
                                                        break;
                                                case 13:
                                                        printfch('+');
                                                        buffstr[i] = '+';
                                                        i++;
                                                        break;
                                                case 14:
                                                        if (i == 0) {
                                                                break;
                                                        } else {
                                                                printfch('\b');
                                                                i--;
                                                                buffstr[i+1] = 0;
                                                                buffstr[i] = 0;
                                                                break;
                                                        }

                                                        /*
                                                        i--;
                                                        if(i<0)
                                                        {
                                                                i = 0;
                                                        }
                                                        buffstr[i+1] = 0;
                                                        buffstr[i] = 0;
                                                        break;
                                                        */
                                        /* case 15:
                                                        printfch('\t');          Tab button
                                                        buffstr[i] = '\t';
                                                        i++;
                                                        break;*/
                                                case 16:
                                                        printfch('Q');
                                                        buffstr[i] = 'Q';
                                                        i++;
                                                        break;
                                                case 17:
                                                        printfch('W');
                                                        buffstr[i] = 'W';
                                                        i++;
                                                        break;
                                                case 18:
                                                        printfch('E');
                                                        buffstr[i] = 'E';
                                                        i++;
                                                        break;
                                                case 19:
                                                        printfch('R');
                                                        buffstr[i] = 'R';
                                                        i++;
                                                        break;
                                                case 20:
                                                        printfch('T');
                                                        buffstr[i] = 'T';
                                                        i++;
                                                        break;
                                                case 21:
                                                        printfch('Y');
                                                        buffstr[i] = 'Y';
                                                        i++;
                                                        break;
                                                case 22:
                                                        printfch('U');
                                                        buffstr[i] = 'U';
                                                        i++;
                                                        break;
                                                case 23:
                                                        printfch('I');
                                                        buffstr[i] = 'I';
                                                        i++;
                                                        break;
                                                case 24:
                                                        printfch('O');
                                                        buffstr[i] = 'O';
                                                        i++;
                                                        break;
                                                case 25:
                                                        printfch('P');
                                                        buffstr[i] = 'P';
                                                        i++;
                                                        break;
                                                case 26:
                                                        printfch('{');
                                                        buffstr[i] = '{';
                                                        i++;
                                                        break;
                                                case 27:
                                                        printfch('}');
                                                        buffstr[i] = '}';
                                                        i++;
                                                        break;
                                                case 28:
                                                // printfch('\n');
                                                // buffstr[i] = '\n';
                                                        i++;
                                                reading = 0;
                                                        break;
                                        /*  case 29:
                                                        printfch('q');           Left Control
                                                        buffstr[i] = 'q';
                                                        i++;
                                                        break;*/
                                                case 30:
                                                        printfch('A');
                                                        buffstr[i] = 'A';
                                                        i++;
                                                        break;
                                                case 31:
                                                        printfch('S');
                                                        buffstr[i] = 'S';
                                                        i++;
                                                        break;
                                                case 32:
                                                        printfch('D');
                                                        buffstr[i] = 'D';
                                                        i++;
                                                        break;
                                                case 33:
                                                        printfch('F');
                                                        buffstr[i] = 'F';
                                                        i++;
                                                        break;
                                                case 34:
                                                        printfch('G');
                                                        buffstr[i] = 'G';
                                                        i++;
                                                        break;
                                                case 35:
                                                        printfch('H');
                                                        buffstr[i] = 'H';
                                                        i++;
                                                        break;
                                                case 36:
                                                        printfch('J');
                                                        buffstr[i] = 'J';
                                                        i++;
                                                        break;
                                                case 37:
                                                        printfch('K');
                                                        buffstr[i] = 'K';
                                                        i++;
                                                        break;
                                                case 38:
                                                        printfch('L');
                                                        buffstr[i] = 'L';
                                                        i++;
                                                        break;
                                                case 39:
                                                        printfch(':');
                                                        buffstr[i] = ':';
                                                        i++;
                                                        break;
                                                case 40:
                                                        printfch((char)44);               //   Single quote (')
                                                        buffstr[i] = (char)44;
                                                        i++;
                                                        break;
                                                case 41:
                                                        printfch((char)44);               // Back tick (`)
                                                        buffstr[i] = (char)44;
                                                        i++;
                                                        break;
                                                //case 42:                                   Left Shift

                                                /*case 43:                                 \ (< for somekeyboards)
                                                        printfch((char)92);
                                                        buffstr[i] = 'q';
                                                        i++;
                                                        break;*/
                                                case 44:
                                                        printfch('Z');
                                                        buffstr[i] = 'Z';
                                                        i++;
                                                        break;
                                                case 45:
                                                        printfch('X');
                                                        buffstr[i] = 'X';
                                                        i++;
                                                        break;
                                                case 46:
                                                        printfch('C');
                                                        buffstr[i] = 'C';
                                                        i++;
                                                        break;
                                                case 47:
                                                        printfch('V');
                                                        buffstr[i] = 'V';
                                                        i++;
                                                        break;
                                                case 48:
                                                        printfch('B');
                                                        buffstr[i] = 'B';
                                                        i++;
                                                        break;
                                                case 49:
                                                        printfch('N');
                                                        buffstr[i] = 'N';
                                                        i++;
                                                        break;
                                                case 50:
                                                        printfch('M');
                                                        buffstr[i] = 'M';
                                                        i++;
                                                        break;
                                                case 51:
                                                        printfch('<');
                                                        buffstr[i] = '<';
                                                        i++;
                                                        break;
                                                case 52:
                                                        printfch('>');
                                                        buffstr[i] = '>';
                                                        i++;
                                                        break;
                                                case 53:
                                                        printfch('?');
                                                        buffstr[i] = '?';
                                                        i++;
                                                        break;
                                                /*case 54:
                                                        printfch('.');
                                                        buffstr[i] = '.';
                                                        i++;
                                                        break;*/
                                                /*case 55:
                                                        printfch('?');
                                                        buffstr[i] = '?';
                                                        i++;
                                                        break;
                                                case 56:
                                                        printfch(' ');           Right shift
                                                        buffstr[i] = ' ';
                                                        i++;
                                                        break;*/
                                                case 57:
                                                        printfch(' ');
                                                        buffstr[i] = ' ';
                                                        i++;
                                                        break;
                                        }
                                }
                        }
                        break;

                /*case 43:                                 \ (< for somekeyboards)
                        printfch((char)92);
                        buffstr[i] = 'q';
                        i++;
                        break;*/
                case 44:
                        printfch('z');
                        buffstr[i] = 'z';
                        i++;
                        break;
                case 45:
                        printfch('x');
                        buffstr[i] = 'x';
                        i++;
                        break;
                case 46:
                        printfch('c');
                        buffstr[i] = 'c';
                        i++;
                        break;
                case 47:
                        printfch('v');
                        buffstr[i] = 'v';
                        i++;
                        break;
                case 48:
                        printfch('b');
                        buffstr[i] = 'b';
                        i++;
                        break;
                case 49:
                        printfch('n');
                        buffstr[i] = 'n';
                        i++;
                        break;
                case 50:
                        printfch('m');
                        buffstr[i] = 'm';
                        i++;
                        break;
                case 51:
                        printfch(',');
                        buffstr[i] = ',';
                        i++;
                        break;
                case 52:
                        printfch('.');
                        buffstr[i] = '.';
                        i++;
                        break;
                case 53:
                        printfch('/');
                        buffstr[i] = '/';
                        i++;
                        break;
                /*case 54:
                        printfch('.');
                        buffstr[i] = '.';
                        i++;
                        break;
                case 55:
                        printfch('/');
                        buffstr[i] = '/';
                        i++;
                        break;*/
        /*case 56:
                        printfch(' ');           Right shift
                        buffstr[i] = ' ';
                        i++;
                        break;*/
                case 57:
                        printfch(' ');
                        buffstr[i] = ' ';
                        i++;
                        break;
            }
        }
    }
    buffstr[i-1] = 0;
    return buffstr;
}
