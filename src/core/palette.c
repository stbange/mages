#include "palette.h"

const uint32_t PALETTE_RGB[C_COUNT] = {
    0x000000, 0xFFFFFF, 0x68372B, 0x70A4B2, 0x6F3D86, 0x588D43, 0x352879, 0xB8C76F,
    0x6F4F25, 0x433900, 0x9A6759, 0x444444, 0x6C6C6C, 0x9AD284, 0x6C5EB5, 0x959595,
};

const uint8_t PALETTE_DARKER[C_COUNT] = {
    /* black  */ C_BLACK,  /* white  */ C_LGREY, /* red    */ C_BROWN,
    /* cyan   */ C_LBLUE,  /* purple */ C_BLUE,  /* green  */ C_DGREY,
    /* blue   */ C_BLACK,  /* yellow */ C_ORANGE, /* orange */ C_BROWN,
    /* brown  */ C_BLACK,  /* lred   */ C_RED,   /* dgrey  */ C_BLACK,
    /* grey   */ C_DGREY,  /* lgreen */ C_GREEN, /* lblue  */ C_BLUE,
    /* lgrey  */ C_GREY,
};

uint8_t palette_darken(uint8_t c, int steps)
{
    c &= 15;
    while (steps-- > 0) c = PALETTE_DARKER[c];
    return c;
}
