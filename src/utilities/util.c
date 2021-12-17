#include "../../include/util.h"
#include "../../include/screen.h"

void memory_copy(char *source, char *dest, int nbytes) {
    int i;
    for (i = 0; i < nbytes; i++) {
        *(dest + i) = *(source + i);             //    dest[i] = source[i]
    }
}

void memory_set(uint8 *dest, uint8 val, uint32 len) {
    uint8 *temp = (uint8 *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

/**
 * K&R implementation
 */
void int_to_ascii(int n, char str[]) {          
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do {
        str[i++] = n % 10 + '0';         
    } while ((n /= 10) > 0);

    if (sign < 0) str[i++] = '-';
    str[i] = '\0';

    /* TODO: implement "reverse" */
    return str;
}
string int_to_string(int n)
{
	string ch = malloc(50);
	int_to_ascii(n,ch);
	int len = strlength(ch);
	int i = 0;
	int j = len - 1;
	while(i<(len/2 + len%2))
	{
		char tmp = ch[i];
		ch[i] = ch[j];
		ch[j] = tmp;
		i++;
		j--;
	}
	return ch;
}

int str_to_int(string ch)
{
	int n = 0;
	int p = 1;
	int strlen = strlength(ch);
	int i;
	for (i = strlen-1;i>=0;i--)
	{
		n += ((int)(ch[i] - '0')) * p;
		p *= 10;
	}
	return n;
}

string char_to_string(char ch) {
	string str;
	str[0] = ch;

	return str;
}

uint8 check_string_numbers(string str) {
	uint8 size = strlength(str);
	uint8 res = 0;
	for (uint8 i = 0; i <= size; i++) {
		if (
			str[i] == '1' ||
			str[i] == '2' ||
			str[i] == '3' ||
			str[i] == '4' ||
			str[i] == '5' ||
			str[i] == '6' ||
			str[i] == '7' ||
			str[i] == '8' ||
			str[i] == '9' ||
			str[i] == '0'
		) {
			res = 1;
		}
	}
	return res;
}

uint8 string_to_int_corr(const char *str) {
    uint8 value = 0;

    for(int counter = strlength(str)-1, multiplier = 1; !(counter < 0); --counter, multiplier *= 10) {
        value += (str[counter] - 0x30) * multiplier;
    }
   
    return value;
}

void * malloc(int nbytes)
{
	char variable[nbytes];
	return &variable;
}
