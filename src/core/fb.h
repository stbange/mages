/* 320x200 indexed-colour framebuffer and drawing primitives. */
#ifndef MAGES_FB_H
#define MAGES_FB_H
#include "defs.h"

typedef struct { uint8_t px[SCREEN_H][SCREEN_W]; } Framebuffer;
typedef struct { int x, y, w, h; } Rect;

void fb_clear(Framebuffer *fb, uint8_t c);
void fb_pixel(Framebuffer *fb, int x, int y, uint8_t c);
void fb_rect(Framebuffer *fb, int x, int y, int w, int h, uint8_t c);
void fb_frame(Framebuffer *fb, int x, int y, int w, int h, uint8_t c);
/* Draw a size x size sprite at (x,y), scaled by `scale`, clipped to clip
   (NULL = screen). dark = number of darkening steps. */
void fb_sprite(Framebuffer *fb, const char *const *rows, int size, int x, int y, int scale,
               int owner_colour, int dark, const Rect *clip);
/* Text with the 5x7 font; returns width in pixels. */
int fb_text(Framebuffer *fb, int x, int y, const char *s, uint8_t c);
int fb_text_scaled(Framebuffer *fb, int x, int y, const char *s, uint8_t c, int scale);
int fb_text_width(const char *s);
/* Text centred horizontally on cx. */
void fb_text_centre(Framebuffer *fb, int cx, int y, const char *s, uint8_t c);
bool rect_has(const Rect *r, int x, int y);
/* Write as binary PPM; returns false on I/O error. */
bool fb_write_ppm(const Framebuffer *fb, const char *path);

#endif
