#include <stdbool.h>
#include <stdint.h>

#ifndef TYPES_H
#define TYPES_H

#define low_16(address) (uint16_t)((address) & 0xFFFF)
#define high_16(address) (uint16_t)(((address) >> 16) & 0xFFFF)

#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES_SOFT 1
#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES 1
#include "printf.h"
#include "serial.h"

#endif
