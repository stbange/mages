#include "fb.h"
#include "art.h"
#include "font.h"
#include "palette.h"
#include <stdio.h>
#include <string.h>

void fb_clear(Framebuffer *fb, uint8_t c) { memset(fb->px, c, sizeof fb->px); }

void fb_pixel(Framebuffer *fb, int x, int y, uint8_t c)
{
    if (x >= 0 && y >= 0 && x < SCREEN_W && y < SCREEN_H) fb->px[y][x] = c;
}

void fb_rect(Framebuffer *fb, int x, int y, int w, int h, uint8_t c)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) fb_pixel(fb, x + i, y + j, c);
}

void fb_frame(Framebuffer *fb, int x, int y, int w, int h, uint8_t c)
{
    fb_rect(fb, x, y, w, 1, c);
    fb_rect(fb, x, y + h - 1, w, 1, c);
    fb_rect(fb, x, y, 1, h, c);
    fb_rect(fb, x + w - 1, y, 1, h, c);
}

bool rect_has(const Rect *r, int x, int y)
{
    return x >= r->x && y >= r->y && x < r->x + r->w && y < r->y + r->h;
}

void fb_sprite(Framebuffer *fb, const char *const *rows, int size, int x, int y, int scale,
               int owner_colour, int dark, const Rect *clip)
{
    static const Rect screen = {0, 0, SCREEN_W, SCREEN_H};
    if (!clip) clip = &screen;
    for (int j = 0; j < size; j++) {
        for (int i = 0; i < size; i++) {
            int c = art_pixel(rows[j][i], owner_colour);
            if (c < 0) continue;
            uint8_t pc = palette_darken((uint8_t)c, dark);
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    int px = x + i * scale + sx, py = y + j * scale + sy;
                    if (rect_has(clip, px, py)) fb->px[py][px] = pc;
                }
            }
        }
    }
}

int fb_text_scaled(Framebuffer *fb, int x, int y, const char *s, uint8_t c, int scale)
{
    int x0 = x;
    for (; *s; s++) {
        const uint8_t *g = font_glyph(*s);
        for (int j = 0; j < 7; j++)
            for (int i = 0; i < 5; i++)
                if (g[j] & (0x10 >> i)) fb_rect(fb, x + i * scale, y + j * scale, scale, scale, c);
        x += FONT_W * scale;
    }
    return x - x0;
}

int fb_text(Framebuffer *fb, int x, int y, const char *s, uint8_t c)
{
    return fb_text_scaled(fb, x, y, s, c, 1);
}

int fb_text_width(const char *s) { return (int)strlen(s) * FONT_W; }

void fb_text_centre(Framebuffer *fb, int cx, int y, const char *s, uint8_t c)
{
    fb_text(fb, cx - fb_text_width(s) / 2, y, s, c);
}

bool fb_write_ppm(const Framebuffer *fb, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            uint32_t rgb = PALETTE_RGB[fb->px[y][x] & 15];
            uint8_t b[3] = {(uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb};
            fwrite(b, 1, 3, f);
        }
    }
    fclose(f);
    return true;
}
