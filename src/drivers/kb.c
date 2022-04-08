#include "../../include/kb.h"
#include "../../include/system.h"
#include "../../include/vga.h"
#include "../../include/multiboot.h"

// Very bare bones, and basic keyboard driver 
// Copyright (C) 2022 Panagiotis

string readStr(multiboot_info_t *mbi) {
    char buff;
    string buffstr = (string) malloc(200);
    uint8 i = 0;
    uint8 reading = 1;
    uint8 readTmp = 1;
    while(reading)
    {
        if(inportb(0x64) & 0x1)
        {
            //printf_arr(int_to_string(inportb(0x60)));
            switch(inportb(0x60))
            {
                /*case 1:
                drawText(mbi, '(char)27);           Escape button
                buffstr[i] = (char)27;
                i++;
                break;*/
                case 2:
                        drawText(mbi, '1');
                        buffstr[i] = '1';
                        i++;
                        break;
                case 3:
                        drawText(mbi, '2');
                        buffstr[i] = '2';
                        i++;
                        break;
                case 4:
                        drawText(mbi, '3');
                        buffstr[i] = '3';
                        i++;
                        break;
                case 5:
                        drawText(mbi, '4');
                        buffstr[i] = '4';
                        i++;
                        break;
                case 6:
                        drawText(mbi, '5');
                        buffstr[i] = '5';
                        i++;
                        break;
                case 7:
                        drawText(mbi, '6');
                        buffstr[i] = '6';
                        i++;
                        break;
                case 8:
                        drawText(mbi, '7');
                        buffstr[i] = '7';
                        i++;
                        break;
                case 9:
                        drawText(mbi, '8');
                        buffstr[i] = '8';
                        i++;
                        break;
                case 10:
                        drawText(mbi, '9');
                        buffstr[i] = '9';
                        i++;
                        break;
                case 11:
                        drawText(mbi, '0');
                        buffstr[i] = '0';
                        i++;
                        break;
                case 12:
                        drawText(mbi, '-');
                        buffstr[i] = '-';
                        i++;
                        break;
                case 13:
                        drawText(mbi, '=');
                        buffstr[i] = '=';
                        i++;
                        break;
                case 14:
                        if (i == 0) {
                                break;
                        } else {
                                drawText(mbi, '\b');
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
                        drawText(mbi, '\t');          Tab button
                        buffstr[i] = '\t';
                        i++;
                        break;*/
                case 16:
                        drawText(mbi, 'q');
                        buffstr[i] = 'q';
                        i++;
                        break;
                case 17:
                        drawText(mbi, 'w');
                        buffstr[i] = 'w';
                        i++;
                        break;
                case 18:
                        drawText(mbi, 'e');
                        buffstr[i] = 'e';
                        i++;
                        break;
                case 19:
                        drawText(mbi, 'r');
                        buffstr[i] = 'r';
                        i++;
                        break;
                case 20:
                        drawText(mbi, 't');
                        buffstr[i] = 't';
                        i++;
                        break;
                case 21:
                        drawText(mbi, 'y');
                        buffstr[i] = 'y';
                        i++;
                        break;
                case 22:
                        drawText(mbi, 'u');
                        buffstr[i] = 'u';
                        i++;
                        break;
                case 23:
                        drawText(mbi, 'i');
                        buffstr[i] = 'i';
                        i++;
                        break;
                case 24:
                        drawText(mbi, 'o');
                        buffstr[i] = 'o';
                        i++;
                        break;
                case 25:
                        drawText(mbi, 'p');
                        buffstr[i] = 'p';
                        i++;
                        break;
                case 26:
                        drawText(mbi, '[');
                        buffstr[i] = '[';
                        i++;
                        break;
                case 27:
                        drawText(mbi, ']');
                        buffstr[i] = ']';
                        i++;
                        break;
                case 28:
                // drawText(mbi, '\n');
                // buffstr[i] = '\n';
                        i++;
                reading = 0;
                        break;
        /*  case 29:
                        drawText(mbi, 'q');           Left Control
                        buffstr[i] = 'q';
                        i++;
                        break;*/
                case 30:
                        drawText(mbi, 'a');
                        buffstr[i] = 'a';
                        i++;
                        break;
                case 31:
                        drawText(mbi, 's');
                        buffstr[i] = 's';
                        i++;
                        break;
                case 32:
                        drawText(mbi, 'd');
                        buffstr[i] = 'd';
                        i++;
                        break;
                case 33:
                        drawText(mbi, 'f');
                        buffstr[i] = 'f';
                        i++;
                        break;
                case 34:
                        drawText(mbi, 'g');
                        buffstr[i] = 'g';
                        i++;
                        break;
                case 35:
                        drawText(mbi, 'h');
                        buffstr[i] = 'h';
                        i++;
                        break;
                case 36:
                        drawText(mbi, 'j');
                        buffstr[i] = 'j';
                        i++;
                        break;
                case 37:
                        drawText(mbi, 'k');
                        buffstr[i] = 'k';
                        i++;
                        break;
                case 38:
                        drawText(mbi, 'l');
                        buffstr[i] = 'l';
                        i++;
                        break;
                case 39:
                        drawText(mbi, ';');
                        buffstr[i] = ';';
                        i++;
                        break;
                case 40:
                        drawText(mbi, (char)44);               //   Single quote (')
                        buffstr[i] = (char)44;
                        i++;
                        break;
                case 41:
                        drawText(mbi, (char)44);               // Back tick (`)
                        buffstr[i] = (char)44;
                        i++;
                        break;
                case 42:                                 //Left shift
                        /*do {
                                drawText(mbi, 'Q');
                        } if (inportb(0x60) != 170);
                        break;*/

                        while (inportb(0x60) != 170) {
                                if(inportb(0x64) & 0x1) {
                                        switch(inportb(0x60)) {
                                                case 2:
                                                        drawText(mbi, '!');
                                                        buffstr[i] = '!';
                                                        i++;
                                                        break;
                                                case 3:
                                                        drawText(mbi, '$');
                                                        buffstr[i] = '@';
                                                        i++;
                                                        break;
                                                case 4:
                                                        drawText(mbi, '$');
                                                        buffstr[i] = '#';
                                                        i++;
                                                        break;
                                                case 5:
                                                        drawText(mbi, '$');
                                                        buffstr[i] = '$';
                                                        i++;
                                                        break;
                                                case 6:
                                                        drawText(mbi, '%');
                                                        buffstr[i] = '%';
                                                        i++;
                                                        break;
                                                case 7:
                                                        drawText(mbi, '^');
                                                        buffstr[i] = '^';
                                                        i++;
                                                        break;
                                                case 8:
                                                        drawText(mbi, '&');
                                                        buffstr[i] = '&';
                                                        i++;
                                                        break;
                                                case 9:
                                                        drawText(mbi, '*');
                                                        buffstr[i] = '*';
                                                        i++;
                                                        break;
                                                case 10:
                                                        drawText(mbi, '(');
                                                        buffstr[i] = '(';
                                                        i++;
                                                        break;
                                                case 11:
                                                        drawText(mbi, ')');
                                                        buffstr[i] = ')';
                                                        i++;
                                                        break;
                                                case 12:
                                                        drawText(mbi, '_');
                                                        buffstr[i] = '_';
                                                        i++;
                                                        break;
                                                case 13:
                                                        drawText(mbi, '+');
                                                        buffstr[i] = '+';
                                                        i++;
                                                        break;
                                                case 14:
                                                        if (i == 0) {
                                                                break;
                                                        } else {
                                                                drawText(mbi, '\b');
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
                                                        drawText(mbi, '\t');          Tab button
                                                        buffstr[i] = '\t';
                                                        i++;
                                                        break;*/
                                                case 16:
                                                        drawText(mbi, 'Q');
                                                        buffstr[i] = 'Q';
                                                        i++;
                                                        break;
                                                case 17:
                                                        drawText(mbi, 'W');
                                                        buffstr[i] = 'W';
                                                        i++;
                                                        break;
                                                case 18:
                                                        drawText(mbi, 'E');
                                                        buffstr[i] = 'E';
                                                        i++;
                                                        break;
                                                case 19:
                                                        drawText(mbi, 'R');
                                                        buffstr[i] = 'R';
                                                        i++;
                                                        break;
                                                case 20:
                                                        drawText(mbi, 'T');
                                                        buffstr[i] = 'T';
                                                        i++;
                                                        break;
                                                case 21:
                                                        drawText(mbi, 'Y');
                                                        buffstr[i] = 'Y';
                                                        i++;
                                                        break;
                                                case 22:
                                                        drawText(mbi, 'U');
                                                        buffstr[i] = 'U';
                                                        i++;
                                                        break;
                                                case 23:
                                                        drawText(mbi, 'I');
                                                        buffstr[i] = 'I';
                                                        i++;
                                                        break;
                                                case 24:
                                                        drawText(mbi, 'O');
                                                        buffstr[i] = 'O';
                                                        i++;
                                                        break;
                                                case 25:
                                                        drawText(mbi, 'P');
                                                        buffstr[i] = 'P';
                                                        i++;
                                                        break;
                                                case 26:
                                                        drawText(mbi, '{');
                                                        buffstr[i] = '{';
                                                        i++;
                                                        break;
                                                case 27:
                                                        drawText(mbi, '}');
                                                        buffstr[i] = '}';
                                                        i++;
                                                        break;
                                                case 28:
                                                // drawText(mbi, '\n');
                                                // buffstr[i] = '\n';
                                                        i++;
                                                reading = 0;
                                                        break;
                                        /*  case 29:
                                                        drawText(mbi, 'q');           Left Control
                                                        buffstr[i] = 'q';
                                                        i++;
                                                        break;*/
                                                case 30:
                                                        drawText(mbi, 'A');
                                                        buffstr[i] = 'A';
                                                        i++;
                                                        break;
                                                case 31:
                                                        drawText(mbi, 'S');
                                                        buffstr[i] = 'S';
                                                        i++;
                                                        break;
                                                case 32:
                                                        drawText(mbi, 'D');
                                                        buffstr[i] = 'D';
                                                        i++;
                                                        break;
                                                case 33:
                                                        drawText(mbi, 'F');
                                                        buffstr[i] = 'F';
                                                        i++;
                                                        break;
                                                case 34:
                                                        drawText(mbi, 'G');
                                                        buffstr[i] = 'G';
                                                        i++;
                                                        break;
                                                case 35:
                                                        drawText(mbi, 'H');
                                                        buffstr[i] = 'H';
                                                        i++;
                                                        break;
                                                case 36:
                                                        drawText(mbi, 'J');
                                                        buffstr[i] = 'J';
                                                        i++;
                                                        break;
                                                case 37:
                                                        drawText(mbi, 'K');
                                                        buffstr[i] = 'K';
                                                        i++;
                                                        break;
                                                case 38:
                                                        drawText(mbi, 'L');
                                                        buffstr[i] = 'L';
                                                        i++;
                                                        break;
                                                case 39:
                                                        drawText(mbi, ':');
                                                        buffstr[i] = ':';
                                                        i++;
                                                        break;
                                                case 40:
                                                        drawText(mbi, (char)44);               //   Single quote (')
                                                        buffstr[i] = (char)44;
                                                        i++;
                                                        break;
                                                case 41:
                                                        drawText(mbi, (char)44);               // Back tick (`)
                                                        buffstr[i] = (char)44;
                                                        i++;
                                                        break;
                                                //case 42:                                   Left Shift

                                                /*case 43:                                 \ (< for somekeyboards)
                                                        drawText((char)92);
                                                        buffstr[i] = 'q';
                                                        i++;
                                                        break;*/
                                                case 44:
                                                        drawText(mbi, 'Z');
                                                        buffstr[i] = 'Z';
                                                        i++;
                                                        break;
                                                case 45:
                                                        drawText(mbi, 'X');
                                                        buffstr[i] = 'X';
                                                        i++;
                                                        break;
                                                case 46:
                                                        drawText(mbi, 'C');
                                                        buffstr[i] = 'C';
                                                        i++;
                                                        break;
                                                case 47:
                                                        drawText(mbi, 'V');
                                                        buffstr[i] = 'V';
                                                        i++;
                                                        break;
                                                case 48:
                                                        drawText(mbi, 'B');
                                                        buffstr[i] = 'B';
                                                        i++;
                                                        break;
                                                case 49:
                                                        drawText(mbi, 'N');
                                                        buffstr[i] = 'N';
                                                        i++;
                                                        break;
                                                case 50:
                                                        drawText(mbi, 'M');
                                                        buffstr[i] = 'M';
                                                        i++;
                                                        break;
                                                case 51:
                                                        drawText(mbi, '<');
                                                        buffstr[i] = '<';
                                                        i++;
                                                        break;
                                                case 52:
                                                        drawText(mbi, '>');
                                                        buffstr[i] = '>';
                                                        i++;
                                                        break;
                                                case 53:
                                                        drawText(mbi, '?');
                                                        buffstr[i] = '?';
                                                        i++;
                                                        break;
                                                /*case 54:
                                                        drawText(mbi, '.');
                                                        buffstr[i] = '.';
                                                        i++;
                                                        break;*/
                                                /*case 55:
                                                        drawText(mbi, '?');
                                                        buffstr[i] = '?';
                                                        i++;
                                                        break;
                                                case 56:
                                                        drawText(mbi, ' ');           Right shift
                                                        buffstr[i] = ' ';
                                                        i++;
                                                        break;*/
                                                case 57:
                                                        drawText(mbi, ' ');
                                                        buffstr[i] = ' ';
                                                        i++;
                                                        break;
                                        }
                                }
                        }
                        break;

                /*case 43:                                 \ (< for somekeyboards)
                        drawText((char)92);
                        buffstr[i] = 'q';
                        i++;
                        break;*/
                case 44:
                        drawText(mbi, 'z');
                        buffstr[i] = 'z';
                        i++;
                        break;
                case 45:
                        drawText(mbi, 'x');
                        buffstr[i] = 'x';
                        i++;
                        break;
                case 46:
                        drawText(mbi, 'c');
                        buffstr[i] = 'c';
                        i++;
                        break;
                case 47:
                        drawText(mbi, 'v');
                        buffstr[i] = 'v';
                        i++;
                        break;
                case 48:
                        drawText(mbi, 'b');
                        buffstr[i] = 'b';
                        i++;
                        break;
                case 49:
                        drawText(mbi, 'n');
                        buffstr[i] = 'n';
                        i++;
                        break;
                case 50:
                        drawText(mbi, 'm');
                        buffstr[i] = 'm';
                        i++;
                        break;
                case 51:
                        drawText(mbi, ',');
                        buffstr[i] = ',';
                        i++;
                        break;
                case 52:
                        drawText(mbi, '.');
                        buffstr[i] = '.';
                        i++;
                        break;
                case 53:
                        drawText(mbi, '/');
                        buffstr[i] = '/';
                        i++;
                        break;
                /*case 54:
                        drawText(mbi, '.');
                        buffstr[i] = '.';
                        i++;
                        break;
                case 55:
                        drawText(mbi, '/');
                        buffstr[i] = '/';
                        i++;
                        break;*/
        /*case 56:
                        drawText(mbi, ' ');           Right shift
                        buffstr[i] = ' ';
                        i++;
                        break;*/
                case 57:
                        drawText(mbi, ' ');
                        buffstr[i] = ' ';
                        i++;
                        break;
            }
        }
    }
    buffstr[i-1] = 0;
    return buffstr;
}
