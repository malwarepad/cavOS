#include <ansi.h>
#include <console.h>
#include <fb.h>

// ANSI-compliant terminal stuff
// Copyright (C) 2024 Panagiotis

void ansiHandle(int charnum) {
  switch (charnum) {
  case 'H':
    width = 0;
    height = 0;
    break;

  case 'J': {
    int restWidth = framebufferWidth - (width + TTY_CHARACTER_WIDTH);
    if (restWidth > 0)
      drawRect(width, height, restWidth, TTY_CHARACTER_HEIGHT, bg_color[0],
               bg_color[1], bg_color[2]);
    int restHeight = framebufferHeight - (height + TTY_CHARACTER_HEIGHT);
    if (restHeight > 0)
      drawRect(0, height + TTY_CHARACTER_HEIGHT, framebufferWidth,
               framebufferHeight, bg_color[0], bg_color[1], bg_color[2]);
    updateBull();
    break;
  }

  default:
    break;
  }
}