#include "../include/string.h"
#include "../include/screen.h"
#include "../include/util.h"

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

uint8 searchArg1(string ch1, string ch2) {
        uint8 sizeScnd = strlength(ch2);
        uint8 res = 1;
        uint8 starts = whereSpace1(ch1) + 1;
        uint8 end = strlength(ch1);
        uint8 times = end - starts;
        string data;

        for (uint8 i = 0; i <= times; i++) {
                data[i] = ch1[starts + i];
        }

        printf(data);
}
