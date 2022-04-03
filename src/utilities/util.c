#include "../../include/util.h"
#include "../../include/tty.h"

// Utilities used inside source code
// Copyright (C) 2022 Panagiotis

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
string int_to_ascii(int n, char str[]) {          
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

uint8 check_string(string str) {
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
			str[i] == '-' ||
			str[i] == '=' ||
			str[i] == '0' ||
			str[i] == '!' ||
			str[i] == '@' ||
			str[i] == '#' ||
			str[i] == '$' ||
			str[i] == '%' ||
			str[i] == '^' ||
			str[i] == '&' ||
			str[i] == '*' ||
			str[i] == '(' ||
			str[i] == ')' ||
			str[i] == '_' ||
			str[i] == '+' ||
			str[i] == 'q' ||
			str[i] == 'w' ||
			str[i] == 'e' ||
			str[i] == 'r' ||
			str[i] == 't' ||
			str[i] == 'y' ||
			str[i] == 'u' ||
			str[i] == 'i' ||
			str[i] == 'o' ||
			str[i] == 'p' ||
			str[i] == '[' ||
			str[i] == ']' ||
			str[i] == '\\' ||
			str[i] == 'Q' ||
			str[i] == 'W' ||
			str[i] == 'E' ||
			str[i] == 'R' ||
			str[i] == 'T' ||
			str[i] == 'Y' ||
			str[i] == 'U' ||
			str[i] == 'I' ||
			str[i] == 'O' ||
			str[i] == 'P' ||
			str[i] == '{' ||
			str[i] == '}' ||
			str[i] == '|' ||
			str[i] == 'a' ||
			str[i] == 's' ||
			str[i] == 'd' ||
			str[i] == 'f' ||
			str[i] == 'g' ||
			str[i] == 'h' ||
			str[i] == 'j' ||
			str[i] == 'k' ||
			str[i] == 'l' ||
			str[i] == ';' ||
			str[i] == '\'' ||
			str[i] == 'A' ||
			str[i] == 'S' ||
			str[i] == 'D' ||
			str[i] == 'F' ||
			str[i] == 'G' ||
			str[i] == 'H' ||
			str[i] == 'J' ||
			str[i] == 'K' ||
			str[i] == 'L' ||
			str[i] == ':' ||
			str[i] == '"' ||
			str[i] == 'z' ||
			str[i] == 'x' ||
			str[i] == 'c' ||
			str[i] == 'v' ||
			str[i] == 'b' ||
			str[i] == 'n' ||
			str[i] == 'm' ||
			str[i] == ',' ||
			str[i] == '.' ||
			str[i] == '/' ||
			str[i] == 'Z' ||
			str[i] == 'X' ||
			str[i] == 'C' ||
			str[i] == 'V' ||
			str[i] == 'B' ||
			str[i] == 'N' ||
			str[i] == 'M' ||
			str[i] == '<' ||
			str[i] == '>' ||
			str[i] == '?'
		) {
			res = 1;
		}
	}

	return res;
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

void * malloc(int nbytes)
{
	char variable[nbytes];
	return &variable;
}
