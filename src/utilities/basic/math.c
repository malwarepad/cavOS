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

	for (uint8 i = 0; i <= size; i++) {
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

uint8 math_check_possible_equations(string str) {
	uint8 res = 0;
	uint8 size = strlength(str);

	for (uint8 i = 0; i <= size; i++) {
		if (str[i] == '+' || str[i] == '-' || str[i] == '*' || str[i] == '/') {
			res = 1;
		}
	}

	return res;
}

uint8 math_check_possible_equations_advanced(string str, uint8 type) {
	/*
		Documentation for type:
		0: +
		1: -
		2: *
		3: /
	*/

	uint8 position = -1;
	uint8 size = strlength(str);

	for (uint8 i = 0; i <= size; i++) {
		switch (type)
		{
			case 0:
				if (str[i] == '+') {
					position = i;
				}
				break;

			case 1:
				if (str[i] == '-') {
					position = i;
				}
				break;

			case 2:
				if (str[i] == '*') {
					position = i;
				}
				break;

			case 3:
				if (str[i] == '/') {
					position = i;
				}
				break;
		
			default:
				break;
			}
	}

	return position; // position is -1 if it found nothing existing
}

uint8 math_check_previous_equations(string str, uint8 position) {
	uint8 res = 0;
	//uint8 size = strlength(str);

	for (uint8 i = 0; i < position; i++) {
		if (str[i] == '+' || str[i] == '-' || str[i] == '*' || str[i] == '/') {
			res = 1;
		}
	}

	return res;
}
