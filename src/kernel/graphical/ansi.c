#include <ansi.h>
#include <console.h>
#include <fb.h>

// ANSI-compliant terminal stuff
// Copyright (C) 2024 Panagiotis

#define IS_VALID_NUMBER(c) (((c) >= 48 && (c) <= 57))

typedef struct ANSIcolors {
  uint8_t rgb[3];
} ANSIcolors;

ANSIcolors ansiColors[] = {
    {{0, 0, 0}},      {{170, 0, 0}},    {{0, 170, 0}},   {{170, 85, 0}},
    {{0, 0, 170}},    {{170, 0, 170}},  {{0, 170, 170}}, {{170, 170, 170}},
    {{69, 69, 69}},   {{69, 69, 69}},   {{85, 85, 85}},  {{255, 85, 85}},
    {{85, 255, 85}},  {{255, 255, 85}}, {{85, 85, 255}}, {{255, 85, 255}},
    {{85, 255, 255}}, {{255, 255, 255}}};

bool    asciiQuestionmark = true; //  ?
bool    asciiEscaping = false;    // 0x1B // \033
bool    asciiInside = false;      //  [   // 0x5B
int64_t asciiChar1 = 0;           // any  // any
bool    asciiFirstDone = false;   // any  // any
int64_t asciiChar2 = 0;           // any  // any

// true = done with
bool asciiProcess(int charnum) {
  if (!asciiFirstDone && charnum == '?') {
    asciiQuestionmark = true;
    return false;
  }

  bool validNumber = IS_VALID_NUMBER(charnum);
  if (validNumber) {
    int target = charnum - '0';
    if (!asciiFirstDone)
      asciiChar1 = asciiChar1 * 10 + target;
    else
      asciiChar2 = asciiChar2 * 10 + target;

    return false;
  }

  if (charnum == ';') {
    if (asciiFirstDone)
      return true;

    asciiFirstDone = true;
    return false;
  }

  // end it all (asciiWaitingChar != 0xff && validAsciiChar)
  eraseBull();
  switch (charnum) {
  case 'l':
    if (asciiChar1 == 25 && asciiQuestionmark) {
      eraseBull();
      cursorHidden = true;
    }
    break;
  case 'h':
    if (asciiChar1 == 25 && asciiQuestionmark) {
      cursorHidden = false;
      updateBull();
    }
    break;
  case 'm':
    if (!asciiChar1 && !asciiChar2) {
      changeBg(0, 0, 0);
      changeTextColor(255, 255, 255);
      break;
    }
    if (asciiChar1 >= 100) {
      asciiChar2 = asciiChar1 - (100 - 40);
      asciiChar1 = 1;
    }
    // debugf("\\033[ (%d) (%d) m\n", asciiChar1, asciiChar2);
    bool extra = asciiChar1 == 1;
    bool do2 = asciiChar2 >= 30;
    int  controlling = do2 ? asciiChar2 : asciiChar1;
    if (controlling < 30)
      break; // bold and/or reset?
    bool        bg = (controlling) >= 40;
    ANSIcolors *colors =
        &ansiColors[(controlling)-30 - (bg ? 10 : 0) + (extra ? 10 : 0)];
    (bg ? changeBg : changeTextColor)(colors->rgb[0], colors->rgb[1],
                                      colors->rgb[2]);
    break;
  case 'H':
    if (!asciiChar1)
      asciiChar1 = 1;
    if (!asciiChar2)
      asciiChar2 = 1;
    height = (asciiChar1 - 1) * TTY_CHARACTER_HEIGHT;
    width = (asciiChar2 - 1) * TTY_CHARACTER_WIDTH;
    break;

  case 'C':
    if (!asciiChar1)
      asciiChar1 = 1;
    width += (asciiChar1 - 1) * TTY_CHARACTER_WIDTH;
    break;

  case 'd':
    if (!asciiChar1)
      asciiChar1 = 1;
    height = (asciiChar1 - 1) * TTY_CHARACTER_HEIGHT;
    break;

  case 'G':
    if (!asciiChar1)
      asciiChar1 = 1;
    width = (asciiChar1 - 1) * TTY_CHARACTER_WIDTH;
    break;

  case 'A':
    if (!asciiChar1)
      asciiChar1 = 1;
    height -= (asciiChar1 - 2) * TTY_CHARACTER_HEIGHT;
    if (height > fb.height)
      height = 0;
    break;

  case 'J':
    switch (asciiChar1) {
    case 0: { // no
      int restWidth = fb.width - width;
      if (restWidth > 0)
        drawRect(width, height, restWidth, TTY_CHARACTER_HEIGHT, bg_color[0],
                 bg_color[1], bg_color[2]);
      int restHeight = fb.height - (height + TTY_CHARACTER_HEIGHT);
      if (restHeight > 0)
        drawRect(0, height + TTY_CHARACTER_HEIGHT, fb.width, fb.height,
                 bg_color[0], bg_color[1], bg_color[2]);
      updateBull();
      break;
    }

    case 2:
      clearScreen();
      break;

    case 3: // + scrollback clear
      clearScreen();
      break;
    }
    break;
  default:
    // debugf("%c\n", charnum);
    break;
  }

  updateBull();
  return true;
}

void ansiReset() {
  asciiEscaping = false;
  asciiInside = false;
  asciiQuestionmark = false;

  asciiFirstDone = false;

  asciiChar1 = 0;
  asciiChar2 = 0;
}

// true = don't echo it out
bool ansiHandle(int charnum) {
  if (charnum == 0x1B) { // \033
    ansiReset();
    asciiEscaping = true;
    return true;
  } else if (asciiEscaping && charnum == '[') { // 0x5B
    ansiReset();
    asciiEscaping = true;
    asciiInside = true;
    return true;
  } else if (asciiEscaping && asciiInside) {
    if (asciiProcess(charnum))
      ansiReset();
    return true;
  }

  return false;
}