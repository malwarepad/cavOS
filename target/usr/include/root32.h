#define _ROOT_32_

/*                

   Root32 module for the CavOS.
   file: root32.h


   Made by Anti-Corporate (C) 2026-2027.


             
   
   
   */

#include "bootloader.h"
#include "limine.h"
#include "printf.h"
#include "linux.h"
#define TIME_VOLATILE volatile static void
// Can i use void, static and volatile at the same time?
// Uh, i think yes.

TIME_VOLATILE KEWrite(char *arg_serial[])
  // This stands for Kernel Write.
{
  printf(arg_serial, 0x0d, 0x0a, 0x00);
  // different of other prints, this one can only print a
  // full string.
};
