#include "plm81.h"
/* syntax analyzer tables*/

// clang-format on

const uint8_t c1[][13] = {
    /*   1 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x0A, 0x20, 0xA2, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*   2 */ { 0xA8, 0xAA, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0xEC, 0x08, 0xC0, 0x02, 0x00 },
    /*   3 */ { 0xC0, 0x00, 0x30, 0x0C, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x30 },
    /*   4 */ { 0xC0, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x28, 0x30 },
    /*   5 */ { 0x08, 0x10, 0x00, 0xA0, 0x02, 0x08, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x22 },
    /*   6 */ { 0xC0, 0x00, 0x30, 0x0C, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x30 },
    /*   7 */ { 0x80, 0x10, 0x21, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*   8 */ { 0x80, 0x10, 0x20, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*   9 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  10 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  11 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  12 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  13 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  14 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  15 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x10 },
    /*  16 */ { 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  17 */ { 0x80, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*  18 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  19 */ { 0x80, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*  20 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x00, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  21 */ { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  22 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  23 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  24 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  25 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  26 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  27 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x00, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  28 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x00, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  29 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  30 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  31 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x20 },
    /*  32 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  33 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  34 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  35 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  36 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 },
    /*  37 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  38 */ { 0x80, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*  39 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  40 */ { 0x48, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  41 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  42 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  43 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  44 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  45 */ { 0x3C, 0xA6, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  46 */ { 0x28, 0xA2, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  47 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 },
    /*  48 */ { 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x80, 0x80, 0x00 },
    /*  49 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 },
    /*  50 */ { 0xF8, 0xAF, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0xA9, 0x09, 0x80, 0x00, 0x04 },
    /*  51 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  52 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  53 */ { 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  54 */ { 0x28, 0xA2, 0x6A, 0x01, 0xA8, 0xA0, 0x84, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  55 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  56 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  57 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x10 },
    /*  58 */ { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  59 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  60 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  61 */ { 0x28, 0xA2, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  62 */ { 0x28, 0xB7, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  63 */ { 0x38, 0xA3, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  64 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  65 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x0A, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  66 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x00, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  67 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x00, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  68 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  69 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00 },
    /*  70 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  71 */ { 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  72 */ { 0x18, 0x01, 0x00, 0x00, 0x44, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  73 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x04, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  74 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 },
    /*  75 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x0A, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  76 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  77 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  78 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00 },
    /*  79 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  80 */ { 0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x80, 0x00, 0x00 },
    /*  81 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00 },
    /*  82 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x06, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  83 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  84 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x00, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  85 */ { 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x40, 0x40, 0x00 },
    /*  86 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x40, 0x00, 0x00 },
    /*  87 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  88 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  89 */ { 0x28, 0x02, 0x00, 0x00, 0xA8, 0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  90 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  91 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x0A, 0x20, 0xA3, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  92 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  93 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  94 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  95 */ { 0x28, 0x02, 0x00, 0x00, 0xA8, 0xA0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  96 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  97 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x04, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x11 },
    /*  98 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
    /*  99 */ { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 100 */ { 0x28, 0x02, 0x00, 0x00, 0xA8, 0xA0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 101 */ { 0x28, 0x02, 0x00, 0x00, 0x98, 0x10, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 102 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 103 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 104 */ { 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 105 */ { 0x28, 0x52, 0x15, 0x00, 0xA8, 0xA0, 0x80, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00 },
    /* 106 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x40, 0x00, 0x00 },
    /* 107 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

#define PAIR(a, b) (((a) << 8) + (b))

const int c1tri[] = {
    TRI(3, 3, 3),   TRI(3, 3, 10),   TRI(3, 3, 13),  TRI(3, 3, 24),  TRI(3, 3, 45),  TRI(3, 3, 46),
    TRI(3, 3, 50),  TRI(3, 50, 3),   TRI(5, 6, 3),   TRI(5, 6, 10),  TRI(5, 6, 13),  TRI(5, 6, 24),
    TRI(5, 6, 45),  TRI(5, 6, 46),   TRI(5, 6, 50),  TRI(6, 3, 3),   TRI(6, 3, 10),  TRI(6, 3, 13),
    TRI(6, 3, 24),  TRI(6, 3, 45),   TRI(6, 3, 46),  TRI(6, 3, 50),  TRI(6, 50, 3),  TRI(9, 3, 3),
    TRI(9, 3, 10),  TRI(9, 3, 13),   TRI(9, 3, 24),  TRI(9, 3, 45),  TRI(9, 3, 46),  TRI(9, 3, 50),
    TRI(9, 50, 3),  TRI(10, 3, 3),   TRI(10, 3, 10), TRI(10, 3, 13), TRI(10, 3, 24), TRI(10, 3, 45),
    TRI(10, 3, 46), TRI(10, 3, 50),  TRI(10, 50, 3), TRI(11, 3, 3),  TRI(11, 3, 10), TRI(11, 3, 13),
    TRI(11, 3, 24), TRI(11, 3, 45),  TRI(11, 3, 46), TRI(11, 3, 50), TRI(11, 50, 3), TRI(12, 3, 3),
    TRI(12, 3, 10), TRI(12, 3, 13),  TRI(12, 3, 24), TRI(12, 3, 45), TRI(12, 3, 46), TRI(12, 3, 50),
    TRI(12, 50, 3), TRI(13, 50, 3),  TRI(14, 3, 3),  TRI(14, 3, 10), TRI(14, 3, 13), TRI(14, 3, 24),
    TRI(14, 3, 45), TRI(14, 3, 46),  TRI(14, 3, 50), TRI(14, 50, 3), TRI(15, 50, 3), TRI(15, 62, 6),
    TRI(18, 3, 3),  TRI(18, 3, 10),  TRI(18, 3, 13), TRI(18, 3, 24), TRI(18, 3, 45), TRI(18, 3, 46),
    TRI(18, 3, 50), TRI(18, 50, 3),  TRI(20, 50, 3), TRI(20, 50, 5), TRI(20, 62, 4), TRI(20, 62, 6),
    TRI(20, 71, 1), TRI(22, 3, 3),   TRI(22, 3, 10), TRI(22, 3, 13), TRI(22, 3, 24), TRI(22, 3, 45),
    TRI(22, 3, 46), TRI(22, 3, 50),  TRI(22, 50, 3), TRI(23, 3, 3),  TRI(23, 3, 10), TRI(23, 3, 13),
    TRI(23, 3, 24), TRI(23, 3, 45),  TRI(23, 3, 46), TRI(23, 3, 50), TRI(23, 50, 3), TRI(24, 3, 3),
    TRI(24, 3, 10), TRI(24, 3, 13),  TRI(24, 3, 24), TRI(24, 3, 45), TRI(24, 3, 46), TRI(24, 3, 50),
    TRI(24, 50, 3), TRI(25, 3, 3),   TRI(25, 3, 10), TRI(25, 3, 13), TRI(25, 3, 24), TRI(25, 3, 45),
    TRI(25, 3, 46), TRI(25, 3, 50),  TRI(25, 50, 3), TRI(29, 3, 3),  TRI(29, 3, 10), TRI(29, 3, 13),
    TRI(29, 3, 24), TRI(29, 3, 45),  TRI(29, 3, 46), TRI(29, 3, 50), TRI(29, 50, 3), TRI(30, 50, 3),
    TRI(34, 3, 3),  TRI(34, 3, 10),  TRI(34, 3, 13), TRI(34, 3, 24), TRI(34, 3, 45), TRI(34, 3, 46),
    TRI(34, 3, 50), TRI(34, 50, 3),  TRI(37, 3, 3),  TRI(37, 3, 10), TRI(37, 3, 13), TRI(37, 3, 24),
    TRI(37, 3, 45), TRI(37, 3, 46),  TRI(37, 3, 50), TRI(37, 50, 3), TRI(40, 3, 3),  TRI(40, 3, 10),
    TRI(40, 3, 13), TRI(40, 3, 24),  TRI(40, 3, 45), TRI(40, 3, 46), TRI(40, 3, 50), TRI(40, 50, 3),
    TRI(45, 2, 33), TRI(45, 2, 35),  TRI(45, 2, 43), TRI(47, 45, 1), TRI(51, 3, 3),  TRI(51, 3, 10),
    TRI(51, 3, 13), TRI(51, 3, 24),  TRI(51, 3, 45), TRI(51, 3, 46), TRI(51, 3, 50), TRI(51, 50, 3),
    TRI(52, 3, 3),  TRI(52, 3, 10),  TRI(52, 3, 13), TRI(52, 3, 24), TRI(52, 3, 45), TRI(52, 3, 46),
    TRI(52, 3, 50), TRI(52, 50, 3),  TRI(56, 3, 3),  TRI(56, 3, 10), TRI(56, 3, 13), TRI(56, 3, 24),
    TRI(56, 3, 45), TRI(56, 3, 46),  TRI(56, 3, 50), TRI(56, 50, 3), TRI(60, 3, 3),  TRI(60, 3, 10),
    TRI(60, 3, 13), TRI(60, 3, 24),  TRI(60, 3, 45), TRI(60, 3, 46), TRI(60, 3, 50), TRI(60, 50, 3),
    TRI(64, 3, 3),  TRI(64, 3, 10),  TRI(64, 3, 13), TRI(64, 3, 24), TRI(64, 3, 45), TRI(64, 3, 46),
    TRI(64, 3, 50), TRI(64, 50, 3),  TRI(66, 50, 3), TRI(66, 50, 5), TRI(66, 62, 4), TRI(66, 62, 6),
    TRI(66, 71, 1), TRI(66, 91, 28), TRI(67, 50, 3), TRI(67, 50, 5), TRI(67, 62, 4), TRI(67, 62, 6),
    TRI(67, 71, 1), TRI(69, 63, 2),  TRI(69, 63, 4), TRI(70, 50, 3), TRI(70, 62, 4), TRI(70, 62, 6),
    TRI(73, 50, 3), TRI(73, 50, 5),  TRI(73, 62, 4), TRI(73, 62, 6), TRI(73, 71, 1), TRI(74, 45, 2),
    TRI(78, 63, 2), TRI(78, 63, 4),  TRI(81, 63, 2), TRI(81, 63, 4), TRI(82, 50, 3), TRI(82, 50, 5),
    TRI(82, 62, 4), TRI(82, 62, 6),  TRI(82, 71, 1), TRI(84, 50, 3), TRI(84, 50, 5), TRI(84, 62, 4),
    TRI(84, 62, 6), TRI(84, 71, 1),  TRI(85, 53, 1), TRI(86, 53, 1), TRI(87, 50, 2), TRI(87, 50, 4),
    TRI(90, 3, 3),  TRI(90, 3, 10),  TRI(90, 3, 13), TRI(90, 3, 24), TRI(90, 3, 45), TRI(90, 3, 46),
    TRI(90, 3, 50), TRI(90, 50, 3),  TRI(97, 50, 3), TRI(97, 50, 5), TRI(97, 62, 4), TRI(97, 62, 6),
    TRI(97, 71, 1), TRI(104, 4, 3),  TRI(104, 4, 50)
};

// clang-format off
const int prtb[] = {
    TRI(85, 86, 53), TRI(85, 47, 45), PAIR(85, 53),  PAIR(85, 86),   PAIR(15, 93),
    PAIR(15, 76),    PAIR(15, 79),    85,            15,             71,
    55,              103,             96,            83,             92,
    104,             26,              39,            41,             0,
    PAIR(69, 63),    PAIR(78, 63),    PAIR(87, 50),  PAIR(94, 80),   PAIR(81, 63),
    PAIR(3, 72),     PAIR(90, 72),    32,            106,            44,
    13,              50,              0,             0,              PAIR(87, 50),
    PAIR(69, 63),    PAIR(94, 80),    PAIR(78, 63),  PAIR(81, 63),   PAIR(90, 72),
    62,              50,              45,            7,              8,
    0,               0,               0,             7,              0,
    16,              0,               0,             0,              PAIR(14, 72),
    91,              0,               0,             0,              50,
    0,               0,               0,             57,             0,
    PAIR(50, 49),    0,               97,            21,             57,
    88,              0,               0,             TRI(74, 45, 2), 106,
    PAIR(105, 9),    PAIR(105, 10),   PAIR(105, 34), PAIR(105, 37),  10,
    0,               PAIR(84, 82),    97,            73,             PAIR(54, 11),
    PAIR(54, 12),    PAIR(54, 25),    0,             30,             13,
    0,               13,              0,             PAIR(66, 67),   82,
    73,              66,              0,             50,             70,
    TRI(51, 72, 52), PAIR(62, 60),    51,            56,             29,
    40,              97,              0,             98,             0,
    0,               PAIR(101, 18),   PAIR(101, 22), 0,              97,
    0,               24,              0,             0,              TRI(62, 60, 72),
    PAIR(89, 23),    0,               TRI(62, 5, 6), 0,              PAIR(104, 4),
    42,              PAIR(105, 64),   PAIR(0, 0)
};
// clang-format on
const uint8_t prdtb[] = { 38,  39,  36,  37,  25,  26,  27,  35,  24,  6,   7,   8,   9,   10,  11,
                          12,  13,  14,  15,  16,  61,  78,  41,  72,  114, 117, 121, 62,  70,  79,
                          118, 122, 42,  73,  43,  63,  74,  80,  119, 123, 84,  47,  48,  100, 101,
                          96,  83,  97,  99,  98,  54,  126, 127, 44,  21,  22,  55,  67,  69,  77,
                          128, 49,  68,  53,  125, 59,  124, 40,  45,  52,  76,  75,  120, 65,  64,
                          103, 104, 105, 106, 107, 102, 34,  46,  23,  109, 110, 111, 108, 51,  116,
                          115, 113, 112, 19,  3,   28,  18,  2,   60,  82,  31,  81,  30,  32,  33,
                          50,  20,  5,   66,  71,  1,   88,  89,  87,  17,  4,   93,  92,  58,  29,
                          91,  90,  86,  85,  57,  56,  95,  94 };

const uint8_t hdtb[]  = {
    84,  84,  84,  84,  73,  73,  73, 84, 73,  91, 91,  91,  91, 91, 91,  91, 91,  91,  91,
    91,  68,  77,  86,  106, 61,  61, 62, 69,  74, 78,  81,  90, 87, 94,  87, 69,  94,  78,
    81,  90,  70,  97,  97,  64,  64, 64, 60,  64, 64,  64,  57, 51, 52,  58, 66,  67,  57,
    53,  53,  88,  56,  96,  53,  92, 63, 102, 63, 85,  58,  92, 80, 80,  62, 98,  98,  105,
    105, 105, 105, 105, 105, 103, 58, 55, 54,  54, 54,  54,  83, 61, 61,  61, 61,  75,  82,
    73,  75,  82,  102, 71,  99,  71, 99, 76,  79, 96,  75,  65, 98, 106, 59, 101, 101, 101,
    91,  65,  100, 100, 102, 93,  89, 89, 72,  72, 104, 104, 95, 95
};

const uint8_t prlen[] = { 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 3,
                          3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2,
                          2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 3, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 3,
                          1, 2, 2, 2, 2, 1, 1, 4, 2, 3, 3, 3, 3, 2, 1, 3, 2, 2, 3, 3, 3, 1,
                          2, 2, 1, 2, 1, 3, 2, 2, 2, 1, 2, 2, 4, 3, 2, 2, 2, 2, 2, 1, 2, 1,
                          1, 3, 3, 1, 2, 1, 2, 1, 1, 4, 3, 1, 4, 1, 3, 2, 3, 1 };

const uint8_t contc[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const uint8_t leftc[] = { 105, 4, 42, 94, 85 };
// <ARITHMETIC-EXPRESSION>, ',', 'DECLARE', <IDENTIFIER LIST>, <PROCEDURE NAME>

const uint8_t lefti[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 5,
                          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

const uint8_t prind[] = { 0,   20,  27,  34,  41,  43,  47,  48,  50,  50,  50,  50,  50,  50,
                          50,  50,  50,  52,  52,  53,  53,  54,  54,  54,  54,  54,  54,  55,
                          56,  56,  56,  57,  57,  58,  58,  59,  60,  60,  61,  61,  62,  62,
                          62,  63,  63,  65,  67,  67,  68,  68,  73,  73,  73,  75,  81,  81,
                          81,  81,  84,  84,  84,  88,  91,  93,  93,  98,  98,  98,  99,  99,
                          99,  100, 106, 106, 106, 108, 108, 109, 109, 109, 110, 110, 111, 111,
                          111, 111, 111, 111, 111, 114, 114, 116, 116, 116, 116, 118, 118, 118,
                          119, 120, 122, 124, 126, 126, 126, 128, 128 };
