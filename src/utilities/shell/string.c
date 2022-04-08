#include "../../../include/string.h"
#include "../../../include/vga.h"
#include "../../../include/util.h"

// String management file
// Copyright (C) 2022 Panagiotis

uint16 strlength(string ch)
{
        uint16 i = 0;           //Changed counter to 0
        while(ch[i++]);
        return i-1;               //Changed counter to i instead of i--
}

uint8 cmdLength(string ch) {
        uint8 i = 0;
        uint8 size = 0;
        uint8 realSize = strlength(ch);
        uint8 hasSpace = 0;

        string space;
        space[0] = ' ';

        for (i; i <= realSize; i++) {
                if (hasSpace == 0) {
                        if (ch[i] != space[0]) {
                                size++;
                        } else {
                                hasSpace = 1;
                        }
                }
        }
        
        if (hasSpace != 1) {
                size--;
        }

        size--;
        return size;
}

uint8 isStringEmpty(string ch1) {
        if (ch1[0] == '1') {return 0;}
        else if (ch1[0] == '2') {return 0;}
        else if (ch1[0] == '3') {return 0;}
        else if (ch1[0] == '4') {return 0;}
        else if (ch1[0] == '5') {return 0;}
        else if (ch1[0] == '6') {return 0;}
        else if (ch1[0] == '7') {return 0;}
        else if (ch1[0] == '8') {return 0;}
        else if (ch1[0] == '9') {return 0;}
        else if (ch1[0] == '0') {return 0;}
        else if (ch1[0] == '-') {return 0;}
        else if (ch1[0] == '=') {return 0;}
        else if (ch1[0] == 'q') {return 0;}
        else if (ch1[0] == 'w') {return 0;}
        else if (ch1[0] == 'e') {return 0;}
        else if (ch1[0] == 'r') {return 0;}
        else if (ch1[0] == 't') {return 0;}
        else if (ch1[0] == 'y') {return 0;}
        else if (ch1[0] == 'u') {return 0;}
        else if (ch1[0] == 'i') {return 0;}
        else if (ch1[0] == 'o') {return 0;}
        else if (ch1[0] == 'p') {return 0;}
        else if (ch1[0] == '[') {return 0;}
        else if (ch1[0] == ']') {return 0;}
        else if (ch1[0] == '\\') {return 0;}
        else if (ch1[0] == 'a') {return 0;}
        else if (ch1[0] == 's') {return 0;}
        else if (ch1[0] == 'd') {return 0;}
        else if (ch1[0] == 'f') {return 0;}
        else if (ch1[0] == 'g') {return 0;}
        else if (ch1[0] == 'h') {return 0;}
        else if (ch1[0] == 'j') {return 0;}
        else if (ch1[0] == 'k') {return 0;}
        else if (ch1[0] == 'l') {return 0;}
        else if (ch1[0] == ';') {return 0;}
        else if (ch1[0] == '\'') {return 0;}
        else if (ch1[0] == 'z') {return 0;}
        else if (ch1[0] == 'x') {return 0;}
        else if (ch1[0] == 'c') {return 0;}
        else if (ch1[0] == 'v') {return 0;}
        else if (ch1[0] == 'b') {return 0;}
        else if (ch1[0] == 'n') {return 0;}
        else if (ch1[0] == 'm') {return 0;}
        else if (ch1[0] == ',') {return 0;}
        else if (ch1[0] == '.') {return 0;}
        else if (ch1[0] == '/') {return 0;}
        else {
                return 1;
        }
}

uint8 strEql(string ch1,string ch2)
{
        uint8 result = 1;
        uint8 size = strlength(ch1);
        if(size != strlength(ch2)) result =0;
        else
        {
        uint8 i = 0;
        for(i;i<=size;i++)
        {
                if(ch1[i] != ch2[i]) result = 0;
        }
        }
        return result;
}

uint8 cmdEql(string ch1, string ch2) {
        uint8 res = 1;
        uint8 size = cmdLength(ch1);

        if (size != cmdLength(ch2)) res = 0;
        else {
                uint8 i = 0;
                for (i; i <= size; i++) {
                        if (ch1[i] != ch2[i]) {res = 0;}
                }
        }

        return res;
}

string argSrch(string str, uint8 index, uint8 full) {
    string res;
    uint8 str_size = strlength(str);
    uint8 start = 0;
    uint8 end = 0;

    /* Find space  N1*/
    uint8 found_cnt = 0;
    for (int i = 0; i <= str_size; i++) {
        if (str[i] == ' ') {
            found_cnt++;
            if (found_cnt == index) {
                start = i + 1;
            }
        }
    }
    if (found_cnt == 0) {
        return 0;
    }

    /* Find space  N2*/
    int found_cnt2 = 0;
    for (int i = 0; i <= str_size; i++) {
        if (str[i] == ' ') {
            found_cnt2++;
            if (found_cnt2 == (index + 1)) {
                end = i - 1;
            }
        }
    }
    if (found_cnt == found_cnt2) {
        end = str_size;
    }

    for (int i = start; i <= end; i++) {
        res[i - start] = str[i];
    }

    if (full == 1) {
        end = str_size;
    }

    return res;
}
