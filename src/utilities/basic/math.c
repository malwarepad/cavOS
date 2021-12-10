#include "../../../include/string.h"
#include "../../../include/screen.h"
#include "../../../include/util.h"

uint8 math_add(uint8 num1, uint8 num2) {
	return (num1 + num2);
}

uint8 math_remove(uint8 num1, uint8 num2) {
	return (num1 - num2);
}

uint8 math_epi(uint8 num1, uint8 num2) {
	return (num1 * num2);
}

uint8 math_dia(uint8 num1, uint8 num2) {
	return (num1 / num2);
}

uint8 math_get_current_equation(string str) {
	uint8 size = strlength(str);
	uint8 track = 0;
	uint8 res = -1;

	for (uint8 i = 0; i < size; i++) {
		if (track == 0) {
			if (str[i] == '+') {
				track++;
				res = 0;
			} else if (str[i] == '-') {
				track++;
				res = 1;
			} else if (str[i] == '*') {
				track++;
				res = 2;
			} else if (str[i] == '/') {
				track++;
				res = 3;
			}
		}
	}

	return res;
}
