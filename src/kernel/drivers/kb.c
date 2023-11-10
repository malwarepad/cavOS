#include "../include/kb.h"

#include "../include/system.h"

// Very bare bones, and basic keyboard driver
// Copyright (C) 2023 Panagiotis

char characterTable[] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
    '-',  '=',  0,    0x09, 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    'o',  'p',  '[',  ']',  0,    0,    'a',  's',  'd',  'f',  'g',  'h',
    'j',  'k',  'l',  ';',  '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  0x0F, ' ',  0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x1B, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0x0E, 0x1C, 0,    0,    0,
    0,    0,    0,    0,    0,    '/',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0,
    0,    0,    0,    0,    0,    0,    0,    0x2C,
};

char shiftedCharacterTable[] = {
    0,    0,    '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
    '_',  '+',  0,    0x09, 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    'O',  'P',  '{',  '}',  0,    0,    'A',  'S',  'D',  'F',  'G',  'H',
    'J',  'K',  'L',  ':',  '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  0x0F, ' ',  0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x1B, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0x0E, 0x1C, 0,    0,    0,
    0,    0,    0,    0,    0,    '?',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0,
    0,    0,    0,    0,    0,    0,    0,    0x2C,
};

char *readStr(char *buffstr) {
  uint32_t i = 0;
  bool     reading = true;
  int      shifted = 0;
  int      capsLocked = 0;

  while (reading) {
    if (inportb(0x64) & 0x1) {
      uint8 scanCode = inportb(0x60);

      // Shift checks
      if (shifted == 1 && scanCode & 0x80) {
        if ((scanCode & 0x7F) == 42) // & 0x7F clears the release
          shifted = 0;
      }

      if (scanCode < sizeof(characterTable) && !(scanCode & 0x80)) {
        char character = (shifted || capsLocked)
                             ? shiftedCharacterTable[scanCode]
                             : characterTable[scanCode];

        if (character != 0) { // Normal char
          printfch(character);
          buffstr[i] = character;
          i++;
        } else if (scanCode == 28) // Enter
        {
          buffstr[i] = '\0';
          reading = false;
        } else if (scanCode == 14) // Backspace
        {
          if (i > 0) {
            printfch('\b');
            i--;
            buffstr[i + 1] = 0;
            buffstr[i] = 0;
          }
        } else if (scanCode == 42) { // Shift
          shifted = 1;
        } else if (scanCode == 58) { // Caps lock
          capsLocked = !capsLocked;
        }
      }
    }
  }
  // buffstr[i - 1] = 0;
  return buffstr;
}
