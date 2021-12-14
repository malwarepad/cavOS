#ifndef MATH_H
#define MATH_H

#include "types.h"

uint8 math_add(uint8 num1, uint8 num2);
uint8 math_remove(uint8 num1, uint8 num2);
uint8 math_epi(uint8 num1, uint8 num2);
uint8 math_dia(uint8 num1, uint8 num2);
uint8 math_get_current_equation(string str);
uint8 math_check_possible_equations(string str);
uint8 math_check_possible_equations_advanced(string str, uint8 type);
uint8 math_check_previous_equations(string str, uint8 position);

#endif