#include "../../../include/string.h"
#include "../../../include/screen.h"
#include "../../../include/util.h"

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

uint8 whereSpace1(string ch1) {
        uint8 size = strlength(ch1);
        uint8 position = 0;
        uint8 foundAnything = 0;

        for (uint8 i = 0; i <= size; i++) {
                if (foundAnything == 0) {
                        if (ch1[i] == ' ') {
                                position = i;
                                foundAnything = 1;
                        }
                }
        }

        if (foundAnything == 0) {
                return 0;
        } else {return position;}
}

uint8 searchArgMain(string ch1, string ch2) {
        uint8 sizeScnd = strlength(ch2);
        uint8 res = 1;
        uint8 starts = whereSpace1(ch1) + 1;
        uint8 end = strlength(ch1);
        uint8 times = end - starts;
        string data;

        for (uint8 i = 0; i <= times; i++) {
                data[i] = ch1[starts + i];
        }

        uint8 dataSize = strlength(data);

        if (dataSize != sizeScnd) res = 0;
        else {
                uint8 i = 0;
                for (i; i <= dataSize; i++) {
                        if (data[i] != ch2[i]) {res = 0;}
                }
        }

        return res;
}

uint8 whatIsArgMain(string ch1) {
        uint8 res = 1;
        uint8 starts = whereSpace1(ch1) + 1;
        uint8 end = strlength(ch1);
        uint8 times = end - starts;
        string data = (string) malloc(200);

        for (uint8 i = 0; i <= times; i++) {
                data[i] = ch1[starts + i];
        }

        return &data;
}
