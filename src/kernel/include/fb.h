#ifndef FB_H
#define FB_H

#include "types.h"
#include "vfs.h"

extern uint8_t *framebuffer;

extern uint16_t framebufferWidth;
extern uint16_t framebufferHeight;
extern uint32_t framebufferPitch;

#define drawPixel(x, y, r, g, b)                                               \
  do {                                                                         \
    framebuffer[((x) + (y) * framebufferWidth) * 4] = (b);                     \
    framebuffer[((x) + (y) * framebufferWidth) * 4 + 1] = (g);                 \
    framebuffer[((x) + (y) * framebufferWidth) * 4 + 2] = (r);                 \
    framebuffer[((x) + (y) * framebufferWidth) * 4 + 3] = 0;                   \
  } while (0)

void drawRect(int x, int y, int w, int h, int r, int g, int b);
void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b);

extern SpecialHandlers fb0;

/* Linux... oh, linux */
#define FBIOGET_VSCREENINFO 0x4600

struct fb_bitfield {
  uint32_t offset;    /* beginning of bitfield	*/
  uint32_t length;    /* length of bitfield		*/
  uint32_t msb_right; /* != 0 : Most significant bit is */
                      /* right */
};

struct fb_var_screeninfo {
  uint32_t xres; /* visible resolution		*/
  uint32_t yres;
  uint32_t xres_virtual; /* virtual resolution		*/
  uint32_t yres_virtual;
  uint32_t xoffset; /* offset from virtual to visible */
  uint32_t yoffset; /* resolution			*/

  uint32_t bits_per_pixel;  /* guess what			*/
  uint32_t grayscale;       /* 0 = color, 1 = grayscale,	*/
                            /* >1 = FOURCC			*/
  struct fb_bitfield red;   /* bitfield in fb mem if true color, */
  struct fb_bitfield green; /* else only length is significant */
  struct fb_bitfield blue;
  struct fb_bitfield transp; /* transparency			*/

  uint32_t nonstd; /* != 0 Non standard pixel format */

  uint32_t activate; /* see FB_ACTIVATE_*		*/

  uint32_t height; /* height of picture in mm    */
  uint32_t width;  /* width of picture in mm     */

  uint32_t accel_flags; /* (OBSOLETE) see fb_info.flags */

  /* Timing: All values in pixclocks, except pixclock (of course) */
  uint32_t pixclock;     /* pixel clock in ps (pico seconds) */
  uint32_t left_margin;  /* time from sync to picture	*/
  uint32_t right_margin; /* time from picture to sync	*/
  uint32_t upper_margin; /* time from sync to picture	*/
  uint32_t lower_margin;
  uint32_t hsync_len;   /* length of horizontal sync	*/
  uint32_t vsync_len;   /* length of vertical sync	*/
  uint32_t sync;        /* see FB_SYNC_*		*/
  uint32_t vmode;       /* see FB_VMODE_*		*/
  uint32_t rotate;      /* angle we rotate counter clockwise */
  uint32_t colorspace;  /* colorspace for FOURCC-based modes */
  uint32_t reserved[4]; /* Reserved for future compatibility */
};

#endif