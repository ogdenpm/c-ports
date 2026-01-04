/****************************************************************************
 *  plm2a.c: part of the C port of Intel's ISIS-II plm80                  *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"
#include <stdlib.h>

/*
    indexed by cfCode
    a bbb ccc 0
*/
uint8_t fragControlBits[] = {
    0,    0,    0,    0,    0x26, 0x30, 0x30, 0x26, 0x30, 0x20, 0x30, 0x12, 0x12, 0x12, 0,    0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x60, 0,    0x26, 0x20, 0x20, 0,    0,    0,    0,    0,    0,
    0x10, 0x80, 0x80, 0x80, 0x90, 0x90, 0x40, 0xA0, 0xA0, 0xA0, 0x80, 0xB0, 0x90, 0x80, 0xB0, 0x90,
    0x80, 0xB0, 0x90, 0x80, 0xB0, 0x90, 0x80, 0xB0, 0x90, 0x20, 0x30, 0x30, 0x30, 0x10, 0x10, 0x70,
    0x70, 0x30, 0x30, 0x30, 0x30, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x20, 0x20, 0,    0,    0x20, 0,    0,    0x2C,
    0x40, 0,    0x10, 0x10, 0x60, 0x20, 0,    0,    0xA0, 0xA0, 0xA0, 0xA0, 0x32, 0x16, 0x10, 0x20,
    0,    0x10, 0x10, 0x10, 0x10, 0x10, 0x60, 0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0x70, 0x60, 0x60, 0x70, 0x50, 0x70, 0x60, 0x60, 0xE0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0,    0,    0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

uint8_t helperIndexMap[] = { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 0xA };

uint8_t fragmentHelperIndex[] = { 0,    2,    4,    6,    8,    0xA,  0xC, 0x10, 0x11, 0xE,  0x12, 0,   2,    4,
                 6,    8,    0xA,  0xC,  0x10, 0x11, 0xE, 0x12, 0,    2,    4,    6,   8,    0xA,
                 0xC,  0x10, 0x11, 0xE,  0x12, 0,    2,   4,    6,    8,    0xA,  0xC, 0x10, 0x11,
                 0xE,  0x12, 0,    2,    4,    6,    8,   0xA,  0xC,  0x10, 0x11, 0xE, 0x12, 0xC,
                 0x11, 0x12, 0xC,  0x11, 0x12, 2,    6,   0,    4,    6,    8,    0xA, 0xC,  0xE,
                 0x10, 0x11, 0x12, 0xD,  0xF,  7,    9,   0xB,  1,    5,    3 };

/*
    each entry is encoded as
    (module id << 2) + relative helper id
*/
uint8_t helperMap[][11] = {
    /* 11 uint8_t entries */
    { 94, 95, 96, 97, 98, 99, 100, 101, 103, 104, 102 },
    { 73, 74, 75, 76, 77, 78, 79, 80, 82, 83, 81 },
    { 0, 0, 0, 0, 0, 0, 69, 70, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 71, 72, 0, 0, 0 },
    { 0, 61, 0, 0, 62, 63, 59, 60, 0, 0, 0 },
    { 0, 66, 0, 0, 67, 68, 64, 65, 0, 0, 0 },
    { 0, 86, 0, 0, 87, 88, 84, 85, 0, 0, 0 },
    { 0, 91, 0, 0, 92, 93, 89, 90, 0, 0, 0 },
    { 0, 0, 0, 0, 11, 12, 13, 14, 15, 16, 17 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 },
    { 0, 34, 0, 35, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 29, 0, 30, 0, 0, 0, 0, 0, 0, 0 },
    { 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 },
    { 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58 },
    { 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116 },
    { 36, 38, 37, 39, 40, 41, 0, 0, 0, 0, 0 },
    { 42, 43, 44, 45, 46, 47, 0, 0, 0, 0, 0 },
    { 0, 31, 0, 0, 32, 33, 0, 0, 0, 0, 0 },
    { 105, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

uint8_t helperGroup[] = { 8, 0x1C, 0xC, 0, 0xB, 0x11, 1, 0xA, 0xF,  0x10, 0xD,
                       9, 2,    3,   4, 5,   6,    7, 0,   0x1E, 0x12, 0xE };

uint8_t noteTypeToHelperGroup[]       = { 3, 3, 3, 3, 3, 3, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0, // was also b457C in
                                                                            // plm4a.c
                       0, 0, 0, 0, 0, 0, 0, 0x12, 7, 4, 4, 2, 0xA, 0x15, 0, 0, 0, 0, 8, 9, 0, 0, 0, 0xB,
                       6, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 5, 1, 1, 1, 0x13, 0x13, 0x13, 1, 1, 1, 0x13,
                       0x13, 0x13, 0x14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0xC,
                       0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xD, 0xD, 0xD, 0xD, 0xD, 0xD, 0xD, 0xD, 0, 0xE,
                       0xE, 0xE, 0xE, 0xF, 0xF, 0xF, 0xF, 0, 0xF, 0xF, 0xF, 0xF, 0xE, 0xE, 0xE, 0xE, 0,
                       0x10, 0x10, 0x10, 0x10, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 };

uint8_t fragmentArgTypes[]       = {
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0x10, 0,    0,    0,    0,    0,    0,
    0,    0,    0x10, 0,    0x20, 0,    0,    0x13, 0,    0,    0,    0,    0,    0,    0,    0,
    0x50, 0,    0,    0,    0x10, 0x10, 0x20, 0x10, 0x10, 0x10, 0,    0x50, 0x50, 0,    0x50, 0x50,
    0,    0x50, 0x50, 0,    0x50, 0x50, 0,    0x50, 0x50, 0x40, 0,    0x10, 0x10, 0x20, 0x20, 0x60,
    0x60, 0x60, 0x60, 0x60, 0x60, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x40, 0x40, 0,    0,    0x10, 0,    0,    0x21,
    0x10, 0,    0x10, 0x10, 0x10, 0x40, 0,    0,    0x10, 0x10, 0x10, 0x10, 0x12, 0,    0,    0x40,
    0,    0,    0,    0,    0,    0,    0x10, 0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

// xxx nnnnn    nnnnn -> code sequence length
// indexed by curOp
uint8_t fragmentCodeLength[] = {
    0,    0x20, 0x40, 0x60, 0x81, 0x81, 0x84, 0x83, 0x83, 1,    0x83, 0x82, 0x83, 0x81, 0x81, 0x82,
    0x83, 0x83, 1,    0x83, 0x81, 0x82, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x21, 0x22, 0x23, 0x24,
    0x82, 0x81, 0x82, 0x83, 1,    2,    2,    0x21, 0x22, 0x23, 1,    2,    1,    2,    3,    2,
    4,    5,    4,    4,    5,    4,    3,    4,    3,    0x21, 0x85, 0x83, 0x83, 0x84, 0x83, 3,
    0x23, 0x88, 0x89, 0x8D, 0x91, 1,    2,    3,    4,    5,    4,    3,    2,    5,    4,    3,
    2,    4,    5,    6,    6,    5,    4,    3,    0x62, 0x41, 1,    2,    2,    2,    1,    0x8A,
    0x82, 0x83, 1,    1,    0x84, 0x61, 0xC,  0xE,  0x41, 0x42, 0x61, 0x62, 3,    2,    1,    0x62,
    0x61, 0,    0,    0,    0,    0,    0x84, 0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0x20, 0x20, 0x20, 0x20, 0x20, 0x22, 0x23, 0x23, 0x23, 0x23,
    6,    6,    6,    1,    1,    1,    2,    0x20, 0x60, 3,    0x20, 0x40, 0,    0,    0,    0,
    0,    0,    0x60, 0x60, 0x40, 0,    0x80, 0x84, 7,    7,    4,    5,    0,    0,    0x84, 0x84,
    0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    0x43, 0x43, 0x43, 0x63, 0x63, 0x63, 0x23, 0x23, 0x23, 0x23, 0x23,
    0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 3,    3,    0x23, 0x23, 0x23, 0x23, 0x23, 0x23
};

uint8_t fragmentOpcodeTable[] = {
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x50, 0x5C, 0x5D, 0x5E, 5,    0x50, 0x5C, 0x5D, 0xED, 0xCD,
    0xDE, 0xDC, 0xDE, 0xDC, 0xDE, 0xDC, 0x85, 0x85, 0x85, 5,    0x15, 0x25, 0x35, 0xC5, 0xD5, 0xE5,
    5,    0x15, 0x25, 0x35, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 0xFE, 0x85, 0x40, 0xB0, 0xB1, 0xB2,
    0xBC, 0xBD, 0x80, 0x81, 0x82, 0x8C, 0x8D, 0x5E, 0x15, 0x25, 0x35, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5,
    0xE5, 5,    5,    0xE5, 5,    0xE5, 5,    0xE5, 5,    0xE5, 5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    0xB1, 0xE1, 0xD1, 0xB1, 0xE1, 0xD1, 0x81, 1,
    0x81, 1,    0x81, 1,    0xB1, 0xE1, 0xD1, 5,    5,    0xC5, 0xD5, 0xE5, 0x95, 0xB5, 0xA5, 5,
    0xC5, 0xD5, 0xE5, 0x95, 0xA5, 0xB5, 0xC5, 0xD5, 0xE5, 0x95, 0xA5, 0xB5, 5,    0x15, 0x25, 0x35,
    5,    0x15, 0x25, 0x35, 0xC5, 0xD5, 0xE5, 0x65, 0x75, 0x85, 0x95, 0xA5, 0xB5, 0xC5, 0xD5, 0xE5,
    0x65, 0x75, 0x85, 0x95, 0xA5, 0xB5, 0xBC, 0xBD, 0x9E, 0xAE, 0x8C, 0x8D, 0x6E, 0x7E, 0x6A, 0x7B,
    0x8A, 0x86, 0x87, 0x68, 0x78, 0x89, 0x8A, 0x6B, 0x7B, 0xE6, 0xE7, 0xC8, 0xD8, 0xE9, 0xEA, 0xCB,
    0xDB, 0xA6, 0xB7, 0xA8, 0xA9, 0xBA, 0xAB, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5,
    0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 5,    0xC5, 0xD5, 0xE5, 0xE5, 0x55, 0xED,
    0xCE, 0xDE, 0xEC, 0xA0, 0xB0, 0xBD, 0xAE, 0xA,  0xB,  0xDB, 0xEA, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 8,    5,    1,    2,    3,    0xA9, 0xAB, 0xAC, 0xA0, 0xB0, 0xBD, 0xAE, 0xA,
    0xB,  0xDB, 0xEA, 0x80, 0x50, 0x10, 0x20, 0x30, 0x9A, 0xBA, 0xCA, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xE0, 0xED, 0xD,  0xCD, 0xE,  0xDE, 0xD0, 0xDC, 0xE,  0xDE, 0xD0, 0xDC, 8,
    5,    1,    2,    3,    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xA9, 0xAB, 0xAC, 0xB,  0xDB,
    0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 8,    5,    1,    2,    3,    8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x9A, 0xBA, 0xCA, 0xB0, 0xBD,
    0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 0x80, 0x50, 0x10, 0x20, 0x30, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 0xCD, 0x35, 0xE5, 0x15, 0x25,
    0x35, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 0xE5, 0x65, 0x75, 0x85, 0x95, 0xA5, 0xB5, 0x65, 0x75,
    0x85, 0x65, 0x75, 0x85
};

uint8_t fragmentOpcodeIndex[] = {
    0x18, 0x26, 0x18, 0x18, 0x18, 0x18, 0,    0x12, 0x12, 0x12, 0x2E, 0x27, 0x27, 0x27, 0x2D, 0x2A,
    0x2D, 0x2A, 0x2B, 0x28, 0x2C, 0x29, 0,    0,    0,    0x12, 7,    5,    2,    7,    5,    2,
    0x12, 7,    5,    2,    7,    5,    2,    7,    5,    2,    0,    0,    0,    1,    1,    1,
    1,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0x12, 0x12, 2,    0x12, 2,    0x12, 2,    0x12, 2,    0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0xA,  9,    0xB,  0xA,  9,    0xB,  0x17, 0x17,
    0x17, 0x17, 0x17, 0x17, 0x23, 0x22, 0x24, 0x12, 0x12, 0x1E, 0x1E, 0x1D, 0x1F, 0x20, 0x1C, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 2,    0x12, 0x12, 0x12, 0xD,  0x10, 0x13, 0x12, 7,    5,    2,
    0x12, 7,    5,    2,    7,    5,    2,    7,    5,    2,    7,    5,    2,    7,    5,    2,
    7,    5,    2,    7,    5,    2,    2,    2,    2,    2,    2,    2,    2,    2,    0x1C, 0x20,
    0x1C, 2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    0x1C, 0x20, 0x1C, 0x1C, 0x20, 0x1C, 7,    5,    2,    7,    5,    2,    7,    5,    2,
    7,    5,    2,    7,    5,    2,    7,    5,    2,    0,    0,    0,    0,    0,    0,    2,
    2,    2,    2,    0x1C, 0x20, 0x20, 0x1C, 0x1C, 0x20, 0x20, 0x1C, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x1A, 0x21, 0x1C, 0x12, 0x12, 0x12, 0x12, 0x12, 0x1A, 0x21, 0x1C, 0x1C, 0x20, 0x20, 0x1C, 0x1C,
    0x20, 0x20, 0x1C, 0x12, 0x12, 0x12, 0x12, 0x12, 0x1A, 0x21, 0x1C, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x1A, 0x21, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0x1B, 0x1D, 0x19, 0x1D, 0x1B, 0x1D, 0x19, 0x1D, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x1A, 0x21, 0x1C, 0x20, 0x20, 0x1C, 0x1C, 0x1A, 0x21, 0x1C, 0x20, 0x20,
    0x1C, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1A, 0x21, 0x1C, 0x20, 0x20, 0x1C, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0x1A, 0x21, 0x1C, 0x20, 0x20,
    0x1C, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1A, 0x21, 0x1C, 0x20, 0x20, 0x1C, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x25, 0x26, 0x26, 0,    0,
    0,    0,    0,    0,    7,    5,    2,    2,    7,    5,    2,    7,    5,    2,    7,    5,
    2,    7,    5,    2
};

uint16_t operatorPrecedence[] = { 0,      1,      0x1000, 0x1000, 0x2000, 0x2000, 0x2000, 0x4000, 0x4000, 0x5000,
                 0x6008, 0x6000, 0x5080, 0x8040, 0xD088, 0x3010, 0x8008, 0x9008, 0x8000, 0x8001,
                 0x9000, 0xA008, 0xD088, 0xC000, 0xF000, 0x5080, 0xD088, 0x3010, 0x9008, 0x9000,
                 0xA000, 0xB008, 0xA008, 0xD088, 0xD000, 0xE008, 0xE000, 0xE000, 0xF000, 0xF000,
                 0xB000, 0xB000, 0xB000, 0xE000, 0xE000, 0xE000, 0xC000 };

uint16_t operatorTypeRules[] = { 0x123,  0x123,  0x124,  0x126,  0x134,  0x143,  0x163,  0x423,  0x623,  0x624,
                 0x634,  0x634,  0x4026, 0x4123, 0x4126, 0x4106, 0x4123, 0x4126, 0x4123, 0x4123,
                 0x4126, 0x4136, 0x4326, 0x4623, 0x4666, 0x6024, 0x6124, 0x6104, 0x6124, 0x6124,
                 0x6134, 0x6114, 0x6134, 0x6324, 0x6624, 0x6634, 0x6634, 0x6663, 0x6664, 0x6666,
                 0x5146, 0x5164, 0x5154, 0x5346, 0x5364, 0x5354, 0x5623 };

uint8_t operatorResultType[] = { 0x10, 0x10, 0x20, 0x20, 0x10, 0x10, 2,    2, 5,    5,    5,    5,    1,    3,
                 1,    6,    3,    3,    3,    0x1C, 0x10, 4, 2,    2,    0x20, 0x20, 0x20, 0xE,
                 7,    4,    8,    7,    7,    6,    6,    6, 0x20, 0x10, 0x10, 0x10, 0x20, 0x20,
                 0x10, 0x10, 3,    7,    7,    3,    7,    7, 3,    1,    1,    1,    1,    1,
                 1,    1,    6,    3,    1,    0xE,  4,    2, 6,    0xD,  1,    1,    1,    1,
                 1,    4,    1,    1,    3,    3,    3,    1, 1,    1,    1,    1,    1,    1,
                 1,    1,    1,    1,    1,    1,    1,    1, 1,    4,    1,    1,    1,    1,
                 1,    1,    1,    1,    4,    1,    1,    1, 1,    1,    1,    1,    1,    1,
                 2,    2,    2,    2,    1,    1,    1,    1, 1,    1,    1,    1,    1,    1,
                 1,    3,    3,    3,    0x1C, 2,    1,    1 };

uint8_t opcodeEncodingMap[] = {
    0x3F, 0x40, 0x44, 0x43, 0x41, 0x42, 0x60, 0x6C, 0x6C, 0x6C, 0x61, 0x64, 0x5F, 0x5F, 0xEB, 0xEF,
    0xEB, 0xEF, 0xEB, 0xEF, 0xEB, 0xEF, 0x21, 0x22, 0x23, 0x24, 0x24, 0x24, 0x24, 0x27, 0x27, 0x27,
    0x25, 0x25, 0x25, 0x25, 0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x3B, 0x26, 0x3C, 0x3D, 0x3D, 0x3D,
    0x3E, 0x3E, 0x14, 0x14, 0x14, 0x14, 0x14, 0x16, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
    0x17, 0x4C, 0x45, 0x1C, 0x46, 0x1D, 0x47, 0x1E, 0x48, 0x1F, 0x49, 0x4A, 0x4B, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0xFE, 0xFB, 0xFA, 0xFE, 0xFB, 0xFA, 0xF7, 0xF8,
    0xF7, 0xF8, 0xF7, 0xF8, 0xFE, 0xFB, 0xFA, 0x5E, 0x5A, 0xF9, 0xFA, 0xFB, 0xFC, 0xFE, 0xFD, 0x5B,
    0x62, 0x62, 0x62, 9,    9,    0x12, 0x63, 0x63, 0x63, 0x5C, 0x5C, 0x5D, 0,    0,    0,    0,
    0,    0,    0,    0,    1,    1,    1,    2,    2,    2,    3,    3,    3,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    0x58, 0x58, 0x6F, 0x6F, 0x59, 0x59, 0x59, 0x59, 0xE5, 0xE6,
    0xE7, 0x59, 0x59, 0x59, 0x59, 0x65, 0x65, 0x65, 0x65, 0x59, 0x59, 0x59, 0x59, 0x65, 0x65, 0x65,
    0x65, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 2,    2,    2,    0x68, 0x68, 0x68, 0x69, 0x69, 0x69,
    3,    3,    3,    0x6A, 0x6A, 0x6A, 0x6B, 0x6B, 0x6B, 0x1B, 0x20, 0x20, 0x20, 0x1B, 0x1B, 0x39,
    0x39, 0x39, 0x39, 0xF3, 0xF4, 0xF5, 0xF6, 0xF3, 0xF4, 0xF5, 0xF6, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C,
    0xF0, 0xF1, 0xF2, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF3,
    0xF4, 0xF5, 0xF6, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C, 0xF0, 0xF1, 0xF2, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C,
    0xF0, 0xF1, 0xF2, 0xED, 0xEB, 0xEE, 0xEF, 0xED, 0xEB, 0xEE, 0xEF, 0xED, 0xEB, 0xEE, 0xEF, 0x2A,
    0x2B, 0x2C, 0x2C, 0x2C, 0xF0, 0xF1, 0xF2, 0xF4, 0xF5, 0xF3, 0xF6, 0xF0, 0xF1, 0xF2, 0xF4, 0xF5,
    0xF3, 0xF6, 0xED, 0xEB, 0xEE, 0xEF, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C,
    0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xAE, 0xAF, 0xB0, 0xB1, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C,
    0xF0, 0xF1, 0xF2, 0xF4, 0xF5, 0xF3, 0xF6, 0xED, 0xEB, 0xEE, 0xEF, 0x2D, 0x2E, 0x2F, 0x2F, 0x2F,
    0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xCF, 0xD0, 0xD1, 0xD2, 0x2D, 0x2E, 0x2F, 0x2F, 0x2F,
    0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xCF, 0xD0, 0xD1, 0xD2, 0x36, 0x37, 0x38, 0x38, 0x38,
    0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xDA, 0xDB, 0xDC, 0xDD, 0x36, 0x37, 0x38, 0x38, 0x38,
    0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xDA, 0xDB, 0xDC, 0xDD, 0x30, 0x31, 0x32, 0x32, 0x32,
    0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xB9, 0xBA, 0xBB, 0xBC, 0x30, 0x31, 0x32, 0x32, 0x32,
    0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xB9, 0xBA, 0xBB, 0xBC, 0x33, 0x34, 0x35, 0x35, 0x35,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xC4, 0xC5, 0xC6, 0xC7, 0x33, 0x34, 0x35, 0x35, 0x35,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xC4, 0xC5, 0xC6, 0xC7, 0x66, 0,    1,    0x17, 0x17,
    0x17, 0x17, 0x17, 0x17, 2,    2,    2,    0x70, 2,    2,    2,    3,    3,    3,    2,    2,
    2,    3,    3,    3
};

uint8_t loadOpAttributes[] = { 0x21, 0x42, 0x42, 0x43, 0x41, 0x24, 0x34, 0x43, 0x42, 0x42, 0x42, 0x43,
                 0x45, 0x45, 0x52, 0x53, 0x50, 0x50, 0,    0,    0x60, 0x10, 0x70, 0x70 };

uint8_t loadOpTargetReg[] = { 0x79, 1,    0x61, 0x61, 0x61, 0x61, 0x89, 0x88, 0x89, 1,    0x89, 0x89,
                 0x49, 0x89, 0x89, 0x89, 0x69, 0x89, 0x8F, 0x6F, 0x80, 0x60, 0x80, 0x80 };

uint8_t exprComplexity[] = { 0x63, 0,    0x37, 0x46, 4,    5,    6,   0x3B, 0x3C, 3,    0x3A, 3,    4,    4,
                 6,    0x3A, 0x3B, 0x3B, 5,    6,    3,   4,    5,    4,    5,    6,    5,    5,
                 7,    7,    8,    8,    9,    9,    0xB, 0xB,  0xC,  0x3B, 0x3C, 0x3C, 0x3C, 0x3D,
                 0x3E, 0x3E, 0x3F, 0x3F, 0x40, 0x40, 3,   2,    2,    0x39, 1,    2,    4,    4,
                 0x38, 0x39, 1,    0x38, 3,    4,    4,   5,    7,    8,    0x3B, 0x3C, 3,    3,
                 0x3A, 1,    0x38, 3,    5,    6,    7,   9,    0xA,  0x3C, 0x3D, 0x3E, 2,    0x39,
                 1,    2,    3,    4,    0x38, 0x39, 2,   2,    4,    4,    5,    0x39, 0x39, 1,
                 2,    4,    2,    4,    5,    6,    8,   9,    0x39, 0x3B, 0x3C, 0x3D, 0x38 };

uint8_t exprRegisterCost[] = { 0x63, 0,    1,    0x32, 1,    2,    2,    1,    2,    1,    1,    1,    0xB,  0xD,
                 0xD,  1,    0xB,  0xD,  1,    0xB,  1,    0xB,  0xD,  1,    0xB,  0xD,  0x11, 0x13,
                 0x11, 0x13, 0x11, 0x1B, 0x19, 0x1B, 0x19, 0x1B, 0x19, 1,    0xB,  0x11, 0x13, 0xD,
                 0x11, 0x13, 0x11, 0x1B, 0x19, 0x1B, 1,    2,    1,    1,    1,    2,    1,    2,
                 1,    2,    0x2F, 0x2F, 0x2F, 0x2D, 0x2F, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2F,
                 0x2D, 3,    3,    3,    0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 3,    3,
                 1,    1,    1,    1,    1,    1,    0x1D, 0x1F, 0x1D, 0x1F, 0x1D, 0x1D, 0x1F, 3,
                 3,    3,    0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 3 };

uint8_t typeConversionTable[][16] = {
    { 1, 0x47, 0x47, 0x47, 0, 0, 0, 0, 0, 0, 0, 0, 0x49, 0x49, 0x49, 0 },
    { 0x47, 1, 0x47, 0x47, 0, 0, 0, 0, 0, 0, 0, 0, 0x31, 0x49, 0x49, 0 },
    { 0x47, 0x47, 1, 0x61, 0, 0, 0, 0, 0, 0, 0, 0, 0x49, 0x31, 0x63, 0 },
    { 0x47, 0x47, 0x61, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0x49, 0x63, 0x31, 0 },
    { 9, 0xD, 0xC, 0xB, 1, 0, 0x30, 0x30, 0x30, 0, 0, 0, 0xE, 0x13, 0x12, 0 },
    { 0x32, 0x32, 0x32, 0x32, 0, 1, 0, 0, 0, 0, 0, 0, 0x30, 0x30, 0x30, 0 },
    { 0x34, 0x35, 0x35, 0x35, 0, 0, 1, 0x52, 0x52, 0, 0, 0, 0x36, 0x37, 0x37, 0 },
    { 0x34, 0x64, 0x64, 0x64, 0, 0, 0x52, 1, 0x61, 0, 0, 0, 0x65, 0x65, 0x65, 0 },
    { 0x3A, 0x3A, 0x3A, 0x3A, 0, 0, 0x52, 0x61, 1, 0, 0, 0, 0x3C, 0x3C, 0x3C, 0 },
    { 0x38, 0x39, 0x39, 0x39, 0, 0, 2, 0x53, 0x53, 1, 0x52, 0x52, 0x4A, 0x4A, 0x4B, 0 },
    { 0x38, 0x6A, 0x6A, 0x6A, 0, 0, 0x53, 2, 0x6E, 0x52, 1, 0x61, 0x65, 0x65, 0x66, 0 },
    { 0x3B, 0x3B, 0x3B, 0x3B, 0, 0, 0x53, 0x6E, 2, 0x52, 0x61, 1, 0x45, 0x45, 0x3E, 0 },
    { 0x48, 2, 0x48, 0x48, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0x52, 0x52, 0 },
    { 0x48, 0x48, 2, 0x6E, 0, 0, 0, 0, 0, 0, 0, 0, 0x52, 1, 0x61, 0 },
    { 0x48, 0x48, 0x6E, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0x52, 0x61, 1, 0 },
    { 0xA, 0x11, 0x10, 0xF, 0, 0, 0, 0, 0, 0x30, 0x30, 0x30, 0x16, 0x15, 0x14, 1 },
    { 0x33, 0x33, 0x33, 0x33, 0, 0, 0, 0, 0, 0, 0, 0, 0x30, 0x30, 0x30, 0 },
    { 0, 0, 0, 0, 0, 0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0, 0, 0, 0 },
    { 0x4B, 0x4C, 0x4C, 0x4C, 0, 0, 0x4A, 0x4A, 0x4B, 0, 0, 0, 0x4D, 0x4D, 0x4D, 0 },
    { 0x66, 0x67, 0x67, 0x67, 0, 0, 0x65, 0x65, 0x66, 0, 0, 0, 0x68, 0x68, 0x68, 0 },
    { 0x3D, 0x3F, 0x3F, 0x3F, 0, 0, 0x44, 0x44, 0x3D, 0, 0, 0, 0x40, 0x40, 0x40, 0 },
    { 0x50, 0x51, 0x51, 0x51, 0, 0, 0x4F, 0x4F, 0x50, 0x4A, 0x4A, 0x4B, 0x4D, 0x4D, 0x4E, 0 },
    { 0x6C, 0x6D, 0x6D, 0x6D, 0, 0, 0x6B, 0x6B, 0x6C, 0x65, 0x65, 0x66, 0x68, 0x68, 0x69, 0 },
    { 0x42, 0x43, 0x43, 0x43, 0, 0, 0x46, 0x46, 0x42, 0x44, 0x44, 0x3D, 0x40, 0x40, 0x41, 0 },
    { 0x47, 1, 0x47, 0x47, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0x52, 0x52, 0 },
    { 0x47, 0x47, 1, 0x61, 0, 0, 0, 0, 0, 0, 0, 0, 0x52, 1, 0x61, 0 },
    { 0x47, 0x47, 0x61, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0x52, 0x61, 1, 0 },
    { 8, 8, 8, 7, 0, 3, 0, 0, 0, 0, 0, 0, 6, 5, 4, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0x19, 0x18, 0x17, 0x19, 0x18, 0x17, 0, 0, 0, 0 },
    { 0x2D, 0x2F, 0x2E, 0x2E, 0, 0, 0x2B, 0x2A, 0x2C, 0x1D, 0x1C, 0x1E, 0x23, 0x22, 0x24, 0 },
    { 0x59, 0x60, 0x5F, 0x5F, 0, 0, 0x58, 0x58, 0x58, 0x54, 0x54, 0x54, 0x5D, 0x5C, 0x5E, 0 },
    { 0x28, 0x28, 0x27, 0x27, 0, 0, 0x29, 0x26, 0x25, 0x19, 0x18, 0x17, 0x1D, 0x1C, 0x1E, 0 },
    { 0x59, 0x58, 0x58, 0x58, 0, 0, 0, 0, 0, 0, 0, 0, 0x54, 0x54, 0x54, 0 },
    { 0x55, 0x54, 0x54, 0x54, 0, 0, 0, 0, 0, 0, 0, 0, 0x54, 0x54, 0x54, 0 },
    { 0x1F, 0x21, 0x20, 0x20, 0, 0, 0x1D, 0x1C, 0x1E, 0, 0, 0, 0x23, 0x22, 0x22, 0 },
    { 0x55, 0x5B, 0x5A, 0x5A, 0, 0, 0x54, 0x54, 0x54, 0, 0, 0, 0x5D, 0x5C, 0x5C, 0 },
    { 0x1B, 0x1B, 0x1A, 0x1A, 0, 0, 0x19, 0x18, 0x17, 0, 0, 0, 0x1D, 0x1C, 0x1C, 0 },
    { 0x55, 0x54, 0x54, 0x54, 0, 0, 0, 0, 0, 0, 0, 0, 0x56, 0x56, 0x56, 0 },
    { 0x54, 0x55, 0x55, 0x55, 0, 0, 0, 0, 0, 0, 0, 0, 0x57, 0x57, 0x57, 0 }
};

uint8_t nodeExprCategory[] = { 0x17, 0x16, 0x16, 0x16, 0,    0,    0,    0,    0,    1,    1,    2,    2,    2,
                 2,    2,    2,    2,    3,    3,    4,    4,    4,    5,    5,    5,    5,    5,
                 5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
                 5,    5,    5,    5,    5,    5,    6,    7,    8,    8,    9,    9,    9,    9,
                 9,    9,    0xA,  0xA,  0xB,  0xC,  0xC,  0xC,  0xC,  0xC,  0xC,  0xC,  0xD,  0xD,
                 0xD,  0xE,  0xE,  0xF,  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11,
                 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x14,
                 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 };

uint8_t loadOpFragment[] = { 6,    8,    0xA, 0x3A, 0xA, 6,   7, 0xF, 0x6D, 9, 0x12, 0x11,
                 0x10, 0x10, 0xD, 0xC,  0xB, 0xB, 4, 4,   0xE,  5, 0,    0 };

uint16_t commonConstants[] = { 0, 1, 2, 3, 4, 8, 9, 0xA, 0x10, 0xFD, 0xFE, 0xFF, 0xFFFD, 0xFFFE, 0xFFFF };

uint8_t peepholePatterns[] = { 0,    0x2A, 0,    0x2A, 0,    0x32, 0,    0x32, 0,   0x2A, 0x2A, 0,    0x2A,
                 0,    0x5A, 2,    0xA,  0,    2,    0,    9,    9,   0x11, 0x11, 0x19, 0x71,
                 0x59, 0x69, 0x51, 0x61, 1,    1,    1,    0,    1,   1,    0,    0xA,  0xA,
                 0x12, 0x12, 0x1A, 0x72, 0x5A, 0x6A, 0x52, 0x62, 2,   2,    2,    0,    0,
                 0x11, 0x21, 0x29, 0x39, 0x41, 9,    9,    0,    9,   9,    0,    8,    0,
                 0x59, 0x59, 0x71, 0x71, 0,    0,    0,    0,    1,   1,    1,    0x5B, 0x73,
                 0,    1,    1,    1,    0x59, 0x71, 0,    2,    0xA, 0x12, 2,    0xA,  0x12,
                 0,    1,    0,    1,    0,    1,    0,    1,    1,   0 };


uint8_t optimisationStepTable[] = {
         // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
 /* 00 */   0xFF, 0x22, 0xFF, 0x22, 0xFF, 0x22, 0xFF, 0x22, 0xFF, 0x22, 0x62, 0xFF, 0x22, 0xFF, 4,    4,
 /* 10 */   4,    0xFF, 0x50, 0xFF, 0x64, 0x85, 0x64, 0x85, 0x64, 0x64, 0xA5, 0x64, 0xA5, 0x64, 0x64, 5,
 /* 20 */   4,    0xFF, 0x64, 5,    0xFF, 0x64, 0x85, 0x64, 0x85, 0x64, 0x64, 0xA5, 0x64, 0xA5, 0x64, 0x64,
 /* 30 */   5,    4,    0xFF, 4,    4,    4,    4,    4,    4,    0x64, 4,    0xFF, 0x64, 4,    0xFF, 4,
 /* 40 */   0xFF, 0x25, 0x24, 0x64, 4,    4,    0xFF, 4,    0xFF, 0x64, 5,    4,    0x24, 4,    0xFF, 0x64,
 /* 50 */   5,    4,    0x25, 0x64, 0xFF, 0x14, 0x14, 0x14, 4,    4,    4,    0xFF, 4,    0xFF, 4,    0xFF,
 /* 50 */   4,    0xFF, 0x14, 4,    0xFF
};


// indexed by arith op
// DOUBLE, PLUSSIGN, MINUSSIGN, STAR, SLASH, MOD, AND, OR, XOR
// BASED, BYTEINDEX, WORDINDEX, MEMBER, UNARYMINUS, NOT, LOW, HIGH, ADDRESSOF
// xx yyy zzz
// xx left operand requirements (validation rules)
// yyy right operand requirements (validation rules)
// zzz result type determination rules

uint8_t constFoldRules[] = { 0x5A, 0x61, 0x69, 0x92, 0x92, 0x92, 0x91, 0x91, 0x91,
                 0x5D, 0xD3, 0xD3, 0xDC, 0x9B, 0x9B, 0x98, 0x98, 0x1A };

/* clang-format off */
// note  INCLUDE (162) item changed to have only 1 uint16_t
// indexed by T2 opcode
// xxxxxxnn
// nn -> length of t2 item in words
// other bits as follows
// 80: SETSTMTNO PROCEDURE LOCALLABEL CASELABEL LABELDEF INPUT GOTO JMP JNC JNZ SIGN ZERO PARITY CARRY DISABLE ENABLE
//     HALT STMTCNT LINEINFO MODULE SYNTAXERROR TOKENERROR EOF LIST NOLIST CODE NOCODE EJECT INCLUDE ERROR STACKPTR
//     SEMICOLON OPTBACKREF CASE ENDCASE ENDPROC BEGCALL
// 40: IDENTIFIER NUMBER BIGNUMBER 175 STACKPTR SEMICOLON OPTBACKREF CASE ENDCASE ENDPROC LENGTH LAST SIZE BEGCALL
// 20: meta node STMTCNT LINEINFO MODULE SYNTAXERROR TOKENERROR EOF LIST NOLIST CODE NOCODE EJECT INCLUDE ERROR
// 10: PROCEDURE LOCALLABEL CASELABEL LABELDEF HALT EOF ENDPROC
// 08: control flow boundary JMPFALSE RETURNBYTE RETURNWORD RETURN BEGMOVE CALL CALLVAR PROCEDURE GOTO JMP JNC JNZ ENDPROC
// 04: JMPFALSE CASEBLOCK
// 
// 
              // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
uint8_t nodeControlMap[] = {
                 2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    0xE,  1,    1,    1,    // 00
                 1,    1,    1,    2,    2,    2,    2,    2,    2,    2,    2,    2,    3,    3,    2,    1,    // 10
                 1,    1,    1,    1,    2,    2,    2,    2,    2,    2,    2,    2,    0,    0,    0,    0,    // 20
                 0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    1,    1,    1,    2,    2,    6,    // 30
                 1,    0,    0,    0,    0,    3,    0,    9,    9,    8,    1,    1,    1,    0,    0,    0,    // 40
                 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    // 50
                 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    // 60
                 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    // 70
                 1,    1,    2,    9,    0xB,  0xB,  0x81, 0x99, 0x91, 0x91, 0x91, 0x81, 0x89, 0x89, 0x89, 0x8A, // 80
                 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x90, 0xA1, 0xA3, 0xA0, 0xA1, 0xA2, 0xB0, 0xA0, 0xA0, 0xA0, // 90
                 0xA0, 0xA0, 0xA1, 0xA3, 0,    0,    0,    0,    0,    0,    0,    0,    0x41, 0x41, 0x41, 0x40, // a0
                 0,    0,    0,    0,    0,    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xD8, 0x41, 0x41, 0x41, 0xC0 };     // b0



                 // 0     1     2     3     4     5     6     7     8     9 
uint8_t optimisationStep2Map[] = { 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,      // LT  LE  NE  EQ GE GT ROL ROR SCL SCR
                    0,    0,    0,    0,    0,    0,    0,    0,    0x12, 0x14,   // SHL SHR JMPFALSE ..... DOUBLE PLUSSIGN
                    0x22, 0x33, 0x47, 0x47, 0x41, 0x49, 0x4F, 0,    0x5C, 0x5E,   // MINUSSIGN STAR SLASH MOD AND OR XOR BASED BYTEINDEX WORDINDEX
                    0x60 };                                                       // MEMBER

uint8_t optimsationStep1Map[] = { 0,    0,    0,    0,    0,    0,    1,    3,    5,    7,
                    9,    0xC,  0xE,  0,    0,    0,    0,    0,    0,    0x14,
                    0x25, 0x33, 0x3C, 0x3F, 0x41, 0x49, 0x4F, 0x55, 0,    0,
                    0x62 };

uint8_t step2ActionTable[] = { 0,    0x4D, 0,    0x55, 0,    0x5D, 0,    0x66, 0,    0x6F, 0x6F, 0,    0x77,
                 0,    0xB6, 0x8D, 0xB6, 0,    0x41, 0,    0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x30,
                 0x30, 0x31, 0x31, 0x32, 0x41, 0x41, 0x12, 0,    0x1F, 0x1F, 0,    0x30, 0x30,
                 0x31, 0x31, 0x32, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x41, 0x41, 0x41, 0,    0xAD,
                 0x42, 0x43, 0x44, 0x2C, 0x46, 0x41, 0x12, 0,    0x41, 0x12, 0,    0xAD, 0,
                 0x41, 0x12, 0x41, 0x12, 0xAD, 0,    0xAD, 0,    0x41, 0x41, 0x12, 0xAD, 0xAD,
                 0,    0x41, 0x41, 0x12, 0x20, 0x20, 0,    0x4A, 0x4B, 0x4C, 0x7F, 0x80, 0x81,
                 0,    0xD,  0,    0xE,  0,    0xF,  0,    0x10, 0x11, 0 };



uint8_t lookupResultAttr[] = { 0, 0, 0, 0, 0, 0, 2, 2,   2, 3, 3, 3, 1, 1, 1, 1, 1, 8, 4, 4,
                 4, 5, 5, 5, 6, 6, 6, 0xA, 9, 8, 3, 3, 1, 1, 6, 2, 2, 0, 0, 0xB };

uint8_t lookupResultLoc[] = { 0, 1, 2, 3, 4, 8, 1, 2, 3, 1,   2,   3, 1,   2, 3, 4,   8, 4,   1, 2,
                 3, 1, 2, 3, 1, 2, 3, 8, 8, 0xA, 0xA, 9, 0xA, 9, 9, 0xA, 9, 0xA, 9, 9 };

uint8_t attrLocLookupTable[][11] = { { 0, 1, 2, 3, 4, 0, 0, 0, 5, 0x26, 0x25 },
                     { 0, 0xC, 0xD, 0xE, 0xF, 0, 0, 0, 0x10, 0x21, 0x20 },
                     { 0, 6, 7, 8, 0, 0, 0, 0, 0, 0x24, 0x23 },
                     { 0, 9, 0xA, 0xB, 0, 0, 0, 0, 0, 0x1F, 0x1E },
                     { 0, 0x12, 0x13, 0x14, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0x15, 0x16, 0x17, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0x18, 0x19, 0x1A, 0, 0, 0, 0, 0, 0x22, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0x11, 0, 0, 0, 0, 0, 0x1D },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0x1C, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0x1B, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x27, 0 } };

// clang-format on

void WrFragData() {
    if (!(PRINT || OBJECT)) {
        if (fragment[0] == T2_SYNTAXERROR || fragment[0] == T2_TOKENERROR ||
            fragment[0] == T2_ERROR)
            programErrCnt++;
        return;
    }
    vfWbuf(&utf1, fragment, fragLen);
}

void PutTx1Byte(uint8_t val) {
    fragment[fragLen++] = val;
}

void PutTx1Word(uint16_t val) {
    PutTx1Byte(Low(val));
    PutTx1Byte(High(val));
}

static uint8_t EncodeFragmentArgument(uint8_t iArg, uint8_t start) {
    uint8_t argCode = (uint8_t)iCodeArgs[iArg++];

    if (iCodeArgs[0] <= 12) {
        if (fragLen == start)
            PutTx1Byte(argCode);
        else
            fragment[start] = (fragment[start] << 4) | argCode;   // merge the icode in high/low
    }

    if (argCode > 7) {
        if (argCode == 8 || argCode == 13 || argCode == 10)
            PutTx1Byte((uint8_t)iCodeArgs[iArg++]);
        else
            PutTx1Word(iCodeArgs[iArg++]);

        if (argCode >= 10 && argCode <= 12)
            PutTx1Word(iCodeArgs[iArg++]);
        if (argCode == 12)
            PutTx1Word(iCodeArgs[iArg++]);
    }
    return iArg;
} /* Sub_545D() */

static void EncodeFragArgs(uint8_t frag) {
    uint8_t iArg = 0;
    uint8_t start = fragLen;
    if (fragControlBits[frag] & (7 << 4)) {
        iArg = EncodeFragmentArgument(iArg, start);
        if (iCodeArgs[iArg] != 0 || iCodeArgs[0] <= 12)
            EncodeFragmentArgument(iArg, start);
    }
} /* Sub_5410() */

void EncodeFragData(uint8_t frag) {
    /* EncodeFragData() */
    fragLen = 0;
    PutTx1Byte(frag);
    if (fragControlBits[frag] & 0x80)
        PutTx1Byte(curNodeType);
    EncodeFragArgs(frag);
    memset(iCodeArgs, 0, sizeof(iCodeArgs));
    WrFragData();
}

void EmitTopItem() {
    fragLen = 0;
    if (!PRINT)
        if (tx2[tx2qp].nodeType == T2_LINEINFO || tx2[tx2qp].nodeType == T2_INCLUDE)
            return;
    PutTx1Byte(tx2[tx2qp].nodeType);
    if (fragControlBits[tx2[tx2qp].nodeType] & 0x80) {
        PutTx1Byte((uint8_t)tx2[tx2qp].right);
        PutTx1Word(tx2[tx2qp].left);
    } else
        switch (nodeControlMap[tx2[tx2qp].nodeType] & 3) {
        case 0:
            break;
        case 1:
            PutTx1Word(tx2[tx2qp].left);
            break;
        case 2:
            PutTx1Word(tx2[tx2qp].left);
            PutTx1Word(tx2[tx2qp].right);
            break;
        case 3:
            PutTx1Word(tx2[tx2qp].left);
            PutTx1Word(tx2[tx2qp].right);
            PutTx1Word(tx2[tx2qp].extra);
            break;
        }
    WrFragData();
}

void Tx2SyntaxError(uint8_t arg1b) {
    tx2[tx2qp].nodeType = T2_SYNTAXERROR;
    tx2[tx2qp].left     = arg1b;
}

uint8_t GetFragmentOperandType(uint8_t lrIdx) {
    return lrIdx == Left ? fragmentOpcodeTable[currentFragmentCode] >> 4 : fragmentOpcodeTable[currentFragmentCode] & 0xf;
}

void MoveTx2(uint8_t src, uint8_t dst) {
    tx2[dst] = tx2[src];
}

uint8_t IndirectAddr(uint8_t attr) {
    return attr == STRUCT_A ? 2 : attr + 2;
}

/**
 * AdjustStackOnReturn - Generate code to deallocate stack space on procedure return
 *
 * Generates 8080 assembly code to restore the stack pointer when returning from
 * a procedure, deallocating space used for local variables and temporary values.
 *
 * Two strategies based on stack offset size:
 *
 * 1. Large offsets (> 7 words from current SP):
 *    - Calculates target SP value
 *    - Uses SPHL instruction sequence:
 *      • Optional XCHG (if DE is in use)
 *      • Load HL with offset
 *      • DAD SP (add SP to HL)
 *      • SPHL (load SP from HL)
 *      • Optional XCHG (restore DE)
 *    - Code size: 5-7 bytes
 *
 * 2. Small offsets (≤ 7 words):
 *    - Optional INX SP (if odd uint8_t offset)
 *    - Loop of POP H or POP D instructions (2 bytes per uint16_t)
 *    - Code size: 1 + (offset/2) bytes
 *
 * Also updates currentStackDepth (current stack depth) for negative offsets.
 * Special handling: Reduces offset by 2 words for T2_RETURNWORD.
 *
 * @param stackOffset - Signed offset to adjust stack by (negative = deallocate)
 */
void AdjustStackOnReturn(uint16_t stackOffset) {
    uint16_t offset, q;

    offset = stackOffset + currentStackDepth * 2;
    q      = ((offset + 1) >> 1) + 2; // convert to words
    if (curNodeType == T2_RETURNWORD)
        q -= 2;
    if (q > 7) {
        // Large offset: use SPHL sequence
        if (exprLoc[0] == 3)
            EncodeFragData(CF_XCHG);
        iCodeArgs[0] = IR_SR;
        iCodeArgs[1] = offset;
        EncodeFragData(CF_SA2HL);
        EncodeFragData(CF_SPHL);
        if (exprLoc[0] == 3) {
            EncodeFragData(CF_XCHG);
            codeSize += 7;
        } else
            codeSize += 5;
    } else {
        // Small offset: use POP loop
        if (offset & 1) {
            EncodeFragData(CF_INXSP);
            codeSize++;
        }
        while (offset > 1) {
            if (exprLoc[0] == 3)
                iCodeArgs[0] = IR_D; /*  pop d */
            else
                iCodeArgs[0] = IR_H; /*  pop h */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_POP);
            codeSize++;
            offset -= 2;
        }
    }
    // Update stack depth for negative offsets
    if (stackOffset > 0xff00)
        currentStackDepth = (uint16_t)(-stackOffset) >> 1;
    else
        currentStackDepth = 0;
}

bool EnterBlk() {
    if (activeGrpCnt < 20) {
        activeGrpCnt++;
        return true;
    }
    if (blkOverCnt == 0) {
        Tx2SyntaxError(ERR204); /*  LIMIT EXCEEDED: NUMBER OF ACTIVE */
                                /*  PROCEDURES and do CASE GROUPS */
        EmitTopItem();
    }
    blkOverCnt++;
    return false;
}

bool ExitBlk() {
    if (blkOverCnt > 0) {
        blkOverCnt--;
        return false;
    } else if (activeGrpCnt > 0) {
        activeGrpCnt--;
        return true;
    } else {
        Tx2SyntaxError(ERR205); /*  ILLEGAL NESTING OF BLOCKS, ENDS not BALANCED */
        EmitTopItem();
        return false;
    }
}


void HandleFatalError(uint16_t err) {
    fatalCode = err;
    fragment[0]    = T2_SYNTAXERROR;
    fragment[1]    = (uint8_t)fatalCode;
    fragment[2]    = 0;
    fragLen        = 3;
    WrFragData();

    while (activeGrpCnt > 0) {
        if (ExitBlk()) {
            if (blkId > activeGrpCnt) {
                info             = blk[blkId].info;
                info->codeSize   = codeSize;
                info->stackUsage = stackUsage;
                blkId            = blk[blkId].next;
                codeSize         = blk[blkId].codeSize;
                stackUsage       = blk[blkId].stackSize;
            }
        }
    }
    longjmp(exception, -1);
}

void AnalyzeRegisterUsage() {
    uint8_t i, j, k, n;
    bool m;

    exprRegisterCount = 0;
    savedRegisterCount = 0;
    /* pass 1
    * loop over registers
    * •	Checks if register contains a valid TX2 node
      •	Determines if register is in current expression (registerInExpression[])
      •	Determines if register value needs to be preserved (registerNeedsSave[])
    */

    for (i = 0; i <= 3; i++) {
        registerInExpression[i] = false;
        registerNeedsSave[i] = false;
        registerWasSaved[i] = false;
        k         = registerContents[i];
        n         = nodeControlMap[tx2[k].nodeType] & 0xc0;
        if (k != 0) {
            registerHasValue[i] = true;
            if (exprLoc[0] == i || exprLoc[1] == i) {
                registerInExpression[i] = true;
                if (i > 0)
                    exprRegisterCount++;
            }
            if (n == 0 || n == 0x80) {
                if (tx2[k].extra == 0) {
                    if (tx2[k].exprAttr == registerDataType[i] || (tx2[k].exprAttr == BYTE_A && registerDataType[i] == 6))
                        if (tx2[k].cnt > 1 || registerInExpression[i] || (curExprLoc[0] != k && k != curExprLoc[1]))
                            registerNeedsSave[i] = true;
                }
            }
        } else
            registerHasValue[i] = registerIsDirect[i];
    }
    /* pass 2
    * loop over registers
    * •	Handles duplicate register contents
    * •	Counts registers that will actually be saved to stack
    */
    for (i = 0; i <= 3; i++) {
        if (registerNeedsSave[i]) {
            registerWasSaved[i] = true;
            m         = registerInExpression[i];
            j         = 3;
            while (j > i) {
                if (registerNeedsSave[j]) {
                    if (registerContents[j] == registerContents[i]) {
                        registerNeedsSave[j] = false;
                        if (i != 0) {
                            registerNeedsSave[i] = false;
                            m |= registerInExpression[j];
                        }
                    }
                }
                j--;
            }
            if (i != 0 && !m)
                savedRegisterCount++;
        }
    }
}

void CopyRegisterState(uint8_t src, uint8_t dst) {
    registerContents[dst]  = registerContents[src];
    registerDataType[dst]  = registerDataType[src];
    registerStackOffset[dst]  = registerStackOffset[src];
    registerOffset[dst]  = registerOffset[src];
    registerStorageClass[dst]  = registerStorageClass[src];
    registerIsDirect[dst] = registerIsDirect[src];
}

void SaveRegisterToStack(uint8_t irReg) {
    uint8_t i;
    AnalyzeRegisterUsage();
    i = stackNodeContents[currentStackDepth] = registerContents[irReg];
    if (registerNeedsSave[irReg])
        tx2[i].extra = currentStackDepth;

    if (irReg != 0)
        stackRegisterAttrs[currentStackDepth] = (registerDataType[irReg] << 4) | (registerOffset[irReg] & 0xf);
    else
        stackRegisterAttrs[currentStackDepth] = 0xB0;
}

/**
 * RestoreRegisterFromStack - Restore register contents from stack frame
 *
 * Retrieves a register's value and associated metadata from the stack frame,
 * updating the register tracking arrays to reflect the restored state. This is
 * the inverse operation of Sub_5C1D() which saves register state to stack.
 *
 * The function performs three main tasks:
 *
 * 1. Restore Register Association:
 *    - Retrieves the TX2 node index from stack: registerContents[irReg] = bC140[currentStackDepth]
 *    - Links register back to the expression node it was holding
 *    - If this stack level matches the node's saved position (tx2[i].extra == currentStackDepth):
 *      • Clears the node's stack reference: tx2[i].extra = 0
 *      • Indicates the value is now in register, not on stack
 *
 * 2. Clear Direct Register Flag:
 *    - Sets registerIsDirect[irReg] = 0
 *    - Indicates register now contains a direct value (not stack-relative)
 *    - Marks register as "clean" for code generation
 *
 * 3. Restore Register Attributes:
 *    - Extracts attribute from stack metadata: registerDataType[irReg] = bC0C3[currentStackDepth] >> 4
 *      • Upper 4 bits contain data type/addressing mode
 *      • Values: 0=uint8_t, 1=uint16_t, 2-8=various addressing modes
 *
 *    - Extracts offset from stack metadata: bC0A8[irReg] = bC0C3[currentStackDepth] & 0xf
 *      • Lower 4 bits contain register offset adjustment
 *      • Sign-extends if negative (bit 3 set):
 *        » If offset > 7: OR with 0xf0 to create sign-extended byte
 *        » Creates proper signed value for offset calculations
 *
 * Stack Metadata Format (bC0C3[currentStackDepth]):
 *
 *   Bits 7-4: Register attribute (registerDataType[])
 *   Bits 3-0: Register offset (bC0A8[], sign-extended)
 *
 *   Example: 0xB3 = attribute 0xB, offset -13 (0x3 → 0xF3)
 *            0x25 = attribute 0x2, offset +5
 *
 * Usage Context:
 *
 * This function is called when:
 * - Popping a register from stack (PopRegisterFromStack/Sub_6416)
 * - Restoring register state after spill (UpdateRegisterState case 0)
 * - Exchanging HL with stack top (UpdateRegisterState case 1)
 *
 * Relationship to Sub_5C1D:
 * - Sub_5C1D: Saves register → stack (PUSH operation)
 * - Sub_5C97: Restores stack → register (POP operation)
 *
 * Register State Arrays Updated:
 * - registerContents[]: TX2 node index (what value is in register)
 * - registerDataType[]: Data type/addressing mode attribute
 * - bC0A8[]: Register offset adjustment
 * - registerIsDirect[]: Direct value flag (cleared to 0)
 *
 * Stack Tracking Arrays Read:
 * - bC140[]: TX2 node index saved at stack level
 * - bC0C3[]: Packed attribute and offset metadata
 *
 * @param irReg - Register index to restore (0=B/BC, 2=D/DE, 3=H/HL)
 *
 * @global currentStackDepth - Current stack depth (stack level to restore from)
 * @global bC140[] - Stack-to-TX2 node mapping
 * @global bC0C3[] - Stack metadata (attribute and offset)
 * @global registerContents[] - Register-to-TX2 node mapping
 * @global registerDataType[] - Register attributes
 * @global bC0A8[] - Register offsets
 * @global registerIsDirect[] - Direct value flags
 * @global tx2[] - Expression tree nodes
 *
 * Side effects:
 * - Updates register tracking for irReg
 * - May clear tx2[].extra if register was saved at this stack level
 * - Does NOT modify currentStackDepth (caller must decrement stack depth)
 *
 * Called from:
 * - PopRegisterFromStack() - After POP instruction
 * - UpdateRegisterState() - During register state management
 * - ExchangeHLStack() - During XTHL operation
 *
 * Example:
 * ```
 * // Register H was saved to stack level 5 with attribute 0x6, offset -2
 * // bC140[5] = 42 (TX2 node), bC0C3[5] = 0x6E (attr=6, offset=14/-2)
 * // currentStackDepth = 5
 *
 * Sub_5C97(3);  // Restore register H (irReg=3)
 *
 * // Results:
 * // registerContents[3] = 42        (H now holds node 42)
 * // registerDataType[3] = 6         (attribute 6)
 * // bC0A8[3] = 0xFE      (offset -2, sign-extended from 0xE)
 * // registerIsDirect[3] = 0        (direct register)
 * // tx2[42].extra = 0    (value no longer on stack)
 * ```
 */
void RestoreRegisterFromStack(uint8_t irReg) {
    uint8_t i;

    // Step 1: Restore register-to-TX2 node association
    i = registerContents[irReg] = stackNodeContents[currentStackDepth];

    // If this stack level matches where the value was saved, clear stack reference
    if (currentStackDepth == tx2[i].extra)
        tx2[i].extra = 0;

    // Step 2: Mark register as containing direct value
    registerIsDirect[irReg] = 0;

    // Step 3: Restore register attribute and offset from packed stack metadata
    registerDataType[irReg] = stackRegisterAttrs[currentStackDepth] >> 4;  // Upper 4 bits: attribute
    registerOffset[irReg] = stackRegisterAttrs[currentStackDepth] & 0xf; // Lower 4 bits: offset

    // Sign-extend offset if negative (bit 3 set)
    if (registerOffset[irReg] > 7)
        registerOffset[irReg] = registerOffset[irReg] | 0xf0; // OR with 0xF0 to sign-extend
}

void PushRegisterToStack(uint8_t irReg) {
    if (stackUsage < ++currentStackDepth * 2)
        stackUsage = currentStackDepth * 2;
    SaveRegisterToStack(irReg);
    iCodeArgs[0] = irReg;
    iCodeArgs[1] = LOC_STACK;
    iCodeArgs[2] = currentStackDepth;
    EncodeFragData(CF_PUSH);
    codeSize++;
}

/**
 * SpillNonActiveRegisters - Save non-active registers to stack before invalidating operand
 *
 * Saves all registers that need preservation (registerNeedsSave[]) but are not currently in use
 * by the active expression (registerInExpression[]), then marks the specified operand as being on
 * the stack. This function is called before operations that will invalidate a register
 * containing a needed value.
 *
 * The function performs two main tasks:
 *
 * 1. Save Preserved Registers Not In Active Expression:
 *    - Loops through all registers (BC=0, DE=2, HL=3)
 *    - For each register where:
 *      • registerNeedsSave[i] = true (register needs to be preserved)
 *      • registerInExpression[i] = false (register NOT in current expression)
 *    - Calls PushRegisterToStack(i) to push register to stack
 *    - This prevents losing values needed later when we invalidate arg1b
 *
 * 2. Mark Operand As Stack-Based:
 *    - Determines which expression side (Left or Right) uses the register
 *    - Updates that side's location to 9 (stack-based)
 *    - If exprLoc[0] == arg1b: Set exprLoc[0] = 9
 *    - If exprLoc[1] == arg1b: Set exprLoc[1] = 9
 *    - This tells subsequent code that the operand is now on the stack
 *
 * Usage Scenarios:
 *
 * This function is called from Sub_5D6B() in two cases:
 *
 * Case 1: Register needs preservation AND is in active expression
 * - Before: registerNeedsSave[reg]=true, registerInExpression[reg]=true
 * - Action: Save other non-active registers, then save this one
 * - After: All preserved values are on stack, reg can be reused
 *
 * Case 2: Register is in active expression but doesn't need preservation
 * - But: No duplicate register found with same value
 * - Action: Save other non-active registers, mark as stack, then save
 * - After: All preserved values safe, reg marked as stack-based
 *
 * Example Flow (Sub_5D6B calling Sub_5E16):
 *
 * ```
 * // Setup: Register D holds operand for Left side
 * // exprLoc[0] = 2 (Left in D), exprLoc[1] = 3 (Right in H)
 * // registerNeedsSave[0]=true, registerInExpression[0]=false (B needs save, not active)
 * // registerNeedsSave[2]=false, registerInExpression[2]=true (D active, no save needed)
 * // registerNeedsSave[3]=true, registerInExpression[3]=true  (H active and needs save)
 *
 * Sub_5D6B(2);  // Called to save/handle register D
 *
 * // Sub_5D6B determines: registerInExpression[2]=true (D is in expression)
 * // But no duplicate register found
 * // So calls: Sub_5E16(2)
 *
 * Sub_5E16(2):
 *   // Loop i=0: registerNeedsSave[0]=true, registerInExpression[0]=false
 *   //   -> Save B to stack via PushRegisterToStack(0)
 *   // Loop i=2: Skip (arg=2)
 *   // Loop i=3: registerNeedsSave[3]=true, registerInExpression[3]=true
 *   //   -> Skip (in expression)
 *
 *   // exprLoc[0] == 2, so set exprLoc[0] = 9
 *
 * // Results:
 * // - Register B saved to stack
 * // - Left operand marked as stack-based (exprLoc[0]=9)
 * // - Register D can now be safely saved/reused
 * ```
 *
 * Register State Flags:
 * - registerNeedsSave[i]: Register needs preservation (value used later)
 * - registerInExpression[i]: Register in current active expression
 * - Combinations:
 *   • registerNeedsSave=false, registerInExpression=false: Empty/unused register
 *   • registerNeedsSave=false, registerInExpression=true:  Active operand, temporary value
 *   • registerNeedsSave=true,  registerInExpression=false: Saved value, not currently used -> SAVE IT
 *   • registerNeedsSave=true,  registerInExpression=true:  Active operand, needs preservation
 *
 * Why This Function Exists:
 *
 * When we need to save or invalidate a register that's in the active expression,
 * we must first ensure that OTHER registers holding values we'll need later are
 * safely preserved. This function identifies and saves those "bystander" registers
 * before we proceed with the operation on the target register.
 *
 * Without this step, we could lose important intermediate values when spilling
 * the active register to stack.
 *
 * @param arg1b - Register index being saved/invalidated (0=BC, 2=DE, 3=HL)
 *
 * @global registerNeedsSave[] - Register preservation flags
 * @global registerInExpression[] - Register in-expression flags
 * @global exprLoc[] - Expression location tracking (Left and Right)
 *
 * Side effects:
 * - Pushes non-active preserved registers to stack via PushRegisterToStack()
 * - Updates expression location for operand using arg1b
 * - Increases stack depth (currentStackDepth) for each saved register
 * - Generates PUSH instructions for saved registers
 * - Updates code size counter
 *
 * Called from:
 * - Sub_5D6B() - When saving a register in active expression
 *
 * Calls:
 * - PushRegisterToStack() - To push registers to stack
 *
 * Related Functions:
 * - Sub_5D6B(): Main register spilling logic (calls this)
 * - PushRegisterToStack(): Actually pushes register to stack
 * - AnalyzeRegisterUsage(): Sets registerNeedsSave[] and registerInExpression[] flags
 */
static void SpillNonActiveRegisters(uint8_t arg1b) {
    // Step 1: Save all preserved registers not in current expression
    for (uint8_t i = 0; i <= 3; i++)
        if (registerNeedsSave[i] && !registerInExpression[i])
            PushRegisterToStack(i); // Push register to stack

    // Step 2: Mark the specified operand as now being on the stack
    if (exprLoc[0] == arg1b)
        exprLoc[0] = 9; // Left operand now on stack
    else
        exprLoc[1] = 9; // Right operand now on stack
}



/**
 * SaveOrRedirectRegister - Handle register spilling or redirection for operand reuse
 *
 * Manages register conflicts when a register needs to be saved or when an operand
 * needs to be redirected to another register. This is a central function in the
 * register spilling mechanism that determines whether to save a register to stack
 * or simply redirect expression tracking to a duplicate register.
 *
 * The function implements a sophisticated strategy based on two key register flags:
 * - registerNeedsSave[reg]: Register value needs to be preserved (used later in code)
 * - registerInExpression[reg]: Register is currently in active expression (Left or Right operand)
 *
 * Decision Tree:
 *
 * 1. IF registerNeedsSave[arg1b] = true (Register needs preservation):
 *
 *    a) IF registerInExpression[arg1b] = true (AND register in active expression):
 *       • Call SpillNonActiveRegisters(arg1b)
 *         » Saves all OTHER preserved registers not in expression
 *         » Marks this operand as stack-based (exprLoc = 9)
 *       • Call PushRegisterToStack(arg1b)
 *         » Pushes this register to stack (PUSH instruction)
 *         » Updates stack tracking metadata
 *
 *       Example: Register holds operand AND needs to be saved for later
 *       → Save bystander registers, mark as stack, then PUSH
 *
 *    b) ELSE (register NOT in active expression):
 *       • Call PushRegisterToStack(arg1b) directly
 *         » Just push the register to stack
 *         » No need to save other registers (not in expression)
 *
 *       Example: Register not currently used but needs preservation
 *       → Simple PUSH operation
 *
 * 2. ELSE IF registerInExpression[arg1b] = true (Register in expression but doesn't need save):
 *
 *    First, attempt to find a duplicate register:
 *    • Loop through all registers (i = 0 to 3)
 *    • Look for register where:
 *      » registerContents[i] == registerContents[arg1b] (holds same TX2 node)
 *      » i != arg1b (different register)
 *      » registerDataType[i] == registerDataType[arg1b] (same attribute)
 *
 *    IF duplicate found:
 *       • Redirect expression to use the duplicate register
 *       • If exprLoc[0] == arg1b: exprLoc[0] = i
 *       • If exprLoc[1] == arg1b: exprLoc[1] = i
 *       • Return immediately (no save needed)
 *
 *       Example: Both D and H hold the same value
 *       → Redirect operand from D to H, freeing up D
 *
 *    IF no duplicate found:
 *       • Call SpillNonActiveRegisters(arg1b)
 *         » Saves other preserved registers
 *         » Marks operand as stack-based
 *       • Call PushRegisterToStack(arg1b)
 *         » Pushes register to stack
 *
 *       Example: Register in expression, no duplicate available
 *       → Must save to stack like case 1a
 *
 * 3. ELSE (Register neither needs preservation nor in expression):
 *    • Function does nothing
 *    • Register can be freely overwritten
 *
 * Register State Combinations:
 *
 * registerNeedsSave  registerInExpression  Action
 * ------  ------  ------
 * false   false   Nothing (register free to use)
 * false   true    Try duplicate redirect, else save (case 2)
 * true    false   Simple PUSH to stack (case 1b)
 * true    true    Spill bystanders + PUSH (case 1a)
 *
 * Usage Scenarios:
 *
 * Scenario 1: Commutative Operation Optimization
 * ```
 * // Expression: a + a (both operands same variable)
 * // Both D and H contain value of 'a'
 * // exprLoc[0] = 2 (Left in D), exprLoc[1] = 3 (Right in H)
 *
 * Sub_5D6B(2);  // Need to free up D
 *
 * // Finds: registerContents[3] == registerContents[2] (H has same value as D)
 * // Action: exprLoc[0] = 3 (redirect Left to H)
 * // Result: Both operands now in H, D freed without save
 * ```
 *
 * Scenario 2: Complex Expression with Dependencies
 * ```
 * // Expression: (b + c) * d
 * // B holds 'b', D holds 'c', H holds intermediate (b+c)
 * // Need to load 'd' into D, but 'c' in D is still needed
 * // registerNeedsSave[2]=true, registerInExpression[2]=false
 *
 * Sub_5D6B(2);  // Save register D
 *
 * // Case 1b: registerNeedsSave[2]=true, registerInExpression[2]=false
 * // Action: PushRegisterToStack(2) → PUSH D
 * // Result: D saved to stack, can be reused for 'd'
 * ```
 *
 * Scenario 3: Active Expression with No Duplicate
 * ```
 * // Expression: a + b
 * // D holds 'a' (Left), H holds 'b' (Right)
 * // Need to save D for later use, but it's in active expression
 * // registerNeedsSave[2]=true, registerInExpression[2]=true
 * // No other register holds 'a'
 *
 * Sub_5D6B(2);  // Save active register D
 *
 * // Case 1a: registerNeedsSave[2]=true, registerInExpression[2]=true
 * // Action 1: SpillNonActiveRegisters(2)
 * //   - Saves any other preserved registers (e.g., B if needed)
 * //   - Sets exprLoc[0] = 9 (mark Left as stack)
 * // Action 2: PushRegisterToStack(2) → PUSH D
 * // Result: All preserved values safe, Left operand on stack
 * ```
 *
 * Why Duplicate Detection Matters:
 *
 * When a register is in the active expression but doesn't need preservation,
 * checking for duplicates avoids unnecessary stack operations. This is common
 * in scenarios like:
 * - Commutative operations (a + a)
 * - Common subexpressions loaded into multiple registers
 * - Register allocation with multiple live copies
 *
 * By redirecting to a duplicate, we:
 * - Avoid PUSH/POP overhead
 * - Keep values in registers (faster access)
 * - Reduce stack usage
 * - Simplify code generation
 *
 * @param arg1b - Register index to save/redirect (0=BC, 2=DE, 3=HL)
 *
 * @global registerNeedsSave[] - Register preservation flags (needs save)
 * @global registerInExpression[] - Register in-expression flags (currently used)
 * @global registerContents[] - Register contents (TX2 node index)
 * @global registerDataType[] - Register attributes (data type/addressing mode)
 * @global exprLoc[] - Expression locations (Left and Right operands)
 *
 * Side effects:
 * - May push register(s) to stack via PushRegisterToStack()
 * - May save bystander registers via SpillNonActiveRegisters()
 * - May redirect expression tracking to duplicate register
 * - Updates stack depth (currentStackDepth) if saving
 * - Generates PUSH instructions if saving
 * - Updates code size counter if saving
 *
 * Called from:
 * - GenerateOperandCode() - Before loading operand
 * - SaveConflictingRegisters() - During register conflict resolution
 * - Other register allocation points
 *
 * Calls:
 * - SpillNonActiveRegisters() - Saves bystander registers
 * - PushRegisterToStack() - Pushes register to stack
 *
 * Related Functions:
 * - AnalyzeRegisterUsage() - Sets registerNeedsSave[] and registerInExpression[] flags
 * - SpillNonActiveRegisters() - Saves non-active preserved registers
 * - PushRegisterToStack() - Actually pushes register to stack with tracking
 * - AllocateRegister() - Main register allocation logic
 */
void SaveOrRedirectRegister(uint8_t arg1b) {

    if (registerNeedsSave[arg1b]) {
        // Case 1: Register needs preservation

        if (registerInExpression[arg1b]) {
            // Case 1a: Register in active expression
            // Save bystander registers first, then save this one
            SpillNonActiveRegisters(arg1b);
        }
        // Case 1b: Register not in expression, or case 1a continues
        // Push register to stack
        PushRegisterToStack(arg1b);

    } else if (registerInExpression[arg1b]) {
        // Case 2: Register in expression but doesn't need preservation

        // Try to find a duplicate register with same value
        for (uint8_t i = 0; i <= 3; i++) {
            if (registerContents[i] == registerContents[arg1b] && i != arg1b && registerDataType[i] == registerDataType[arg1b]) {
                // Found duplicate - redirect expression to use it instead
                if (exprLoc[0] == arg1b)
                    exprLoc[0] = i; // Redirect Left operand
                else
                    exprLoc[1] = i; // Redirect Right operand
                return;             // Done - no save needed
            }
        }

        // No duplicate found - must save to stack
        SpillNonActiveRegisters(arg1b);
        PushRegisterToStack(arg1b);
    }

    // Case 3: Register neither needs save nor in expression
    // Do nothing - register is free to overwrite
}

/**
 * InvalidateRegistersByMask - Selectively invalidate and save registers based on bitmask
 *
 * Invalidates a set of registers specified by a bitmask, optionally saving them to stack
 * if they contain values that need preservation. This function is used to clear registers
 * before operations that will overwrite them, while preserving the current expression
 * operand locations.
 *
 * The function performs a sophisticated register invalidation process:
 *
 * 1. Preserve Current Expression Context:
 *    - Saves curExprLoc[0] and curExprLoc[1] to local variables (j, k)
 *    - Clears curExprLoc[] to prevent interference during register analysis
 *    - Ensures expression operands aren't treated as "in use" during invalidation
 *
 * 2. Transform Bitmask for Processing:
 *    - Input arg1b is a bitmask where each bit represents a register
 *    - Performs rotation: arg1b = (arg1b >> 3) | (arg1b << 5)
 *    - This unusual transformation suggests the input bitmask uses a different
 *      bit ordering than the register loop (0-3)
 *    - Example: If bit pattern is xxxxx321, rotation produces 321xxxxx
 *      where bits 0-3 now align with register indices
 *
 * 3. Analyze Register State:
 *    - Calls AnalyzeRegisterUsage() to determine which registers need preservation
 *    - Sets registerNeedsSave[] flags (registers that need to be saved)
 *    - Since curExprLoc[] is cleared, no registers marked as "in expression"
 *
 * 4. Process Each Register (Loop i = 0 to 3):
 *    For each register position:
 *
 *    a) Check if register should be invalidated (arg1b & 1):
 *       IF bit is set:
 *
 *       i) Save if needed:
 *          • IF registerNeedsSave[i] = true (register needs preservation):
 *            » Call PushRegisterToStack(i) to push register to stack
 *            » Saves the value before invalidation
 *
 *       ii) Clear register tracking:
 *           • Set registerIsDirect[i] = 0 (clear direct value flag)
 *           • Set registerContents[i] = 0 (clear TX2 node association)
 *           • Marks register as empty/available
 *
 *       iii) Re-analyze:
 *            • Call AnalyzeRegisterUsage() again
 *            • Updates register state after invalidation
 *            • Ensures subsequent iterations see current state
 *
 *    b) Rotate bitmask for next iteration:
 *       • arg1b = (arg1b << 1) | (arg1b >> 7)
 *       • Shifts next bit into position 0
 *       • Circular rotation to test each bit
 *
 * 5. Restore Expression Context:
 *    - Restores curExprLoc[0] = j (original Left operand location)
 *    - Restores curExprLoc[1] = k (original Right operand location)
 *    - Expression operands remain unchanged by the invalidation
 *
 * Register Invalidation Behavior:
 *
 * - Bit set, registerNeedsSave=true:  Save register to stack, then invalidate
 * - Bit set, registerNeedsSave=false: Just invalidate (no save needed)
 * - Bit clear:             Leave register unchanged
 *
 * Bitmask Rotation Pattern:
 *
 * Initial:  arg1b = (arg1b >> 3) | (arg1b << 5)
 * Loop:     arg1b = (arg1b << 1) | (arg1b >> 7)
 *
 * This double-rotation pattern suggests:
 * - Input bitmask uses non-standard bit positions
 * - First rotation aligns bits with register indices
 * - Loop rotation tests each bit sequentially
 *
 * Usage Scenarios:
 *
 * Scenario 1: Clear Multiple Registers for Function Call
 * ```
 * // Before calling a function, need to free up B, D, H
 * // Current expression uses H (exprLoc[0] = 3)
 * // B contains a value needed later (registerNeedsSave[0] = true)
 *
 * // Bitmask: 0x0B (bits for B, D, H after rotation)
 * Sub_5E66(0x0B);
 *
 * // Process:
 * // - Save curExprLoc[0]=3, curExprLoc[1]=?
 * // - Clear curExprLoc[] temporarily
 * // - Rotate bitmask to align with registers
 * // - Loop i=0: Bit set, registerNeedsSave[0]=true → PUSH B, invalidate
 * // - Loop i=1: Skip (no register 1)
 * // - Loop i=2: Bit set, registerNeedsSave[2]=false → Just invalidate D
 * // - Loop i=3: Bit set, registerNeedsSave[3]=? → Save if needed, invalidate H
 * // - Restore curExprLoc[0]=3 (H still in expression, but invalidated)
 * ```
 *
 * Scenario 2: Clear Specific Register Set
 * ```
 * // Need to clear D and H, preserve B
 * // Bitmask: 0x0A (bits for D, H after rotation)
 *
 * Sub_5E66(0x0A);
 *
 * // - Saves/invalidates D if needed
 * // - Saves/invalidates H if needed
 * // - Leaves B unchanged
 * ```
 *
 * Why Clear curExprLoc[] During Processing:
 *
 * By temporarily clearing curExprLoc[], the function ensures that:
 * - AnalyzeRegisterUsage() doesn't mark registers as "in expression" (registerInExpression)
 * - registerNeedsSave[] flags are set purely based on reference counts and usage
 * - Register invalidation proceeds without expression interference
 * - Expression operands remain valid after invalidation
 *
 * This is crucial when invalidating registers that might contain
 * the active expression operands - we want to invalidate the register
 * state without losing track of where the operands are.
 *
 * Register State Arrays Modified:
 * - registerIsDirect[]: Direct value flags (cleared for invalidated registers)
 * - registerContents[]: Register contents (cleared for invalidated registers)
 * - Stack depth (currentStackDepth): Incremented for each saved register
 * - bC140[]: Stack-to-node mapping (if registers saved)
 * - bC0C3[]: Stack metadata (if registers saved)
 *
 * @param arg1b - Bitmask specifying which registers to invalidate
 *                (bit encoding is rotated before use)
 *
 * @global curExprLoc[] - Expression operand locations (saved/restored)
 * @global registerNeedsSave[] - Register preservation flags (set by AnalyzeRegisterUsage)
 * @global registerIsDirect[] - Direct value flags (cleared for invalidated registers)
 * @global registerContents[] - Register contents (cleared for invalidated registers)
 *
 * Side effects:
 * - Temporarily modifies curExprLoc[] (saved/restored)
 * - May push registers to stack via PushRegisterToStack()
 * - Clears register tracking for invalidated registers
 * - Calls AnalyzeRegisterUsage() multiple times
 * - Generates PUSH instructions for saved registers
 * - Updates stack depth and code size
 *
 * Called from:
 * - Code generation when multiple registers need clearing
 * - Before function calls or complex operations
 * - When register set must be reset to known state
 *
 * Calls:
 * - AnalyzeRegisterUsage() - To determine which registers need saving
 * - PushRegisterToStack() - To push registers to stack
 *
 * Related Functions:
 * - SaveOrRedirectRegister() - Handles single register save/redirect
 * - AnalyzeRegisterUsage() - Determines register preservation needs
 * - PushRegisterToStack() - Actually pushes register to stack
 */
void InvalidateRegistersByMask(uint8_t arg1b) {
    // Step 1: Save current expression operand locations
    uint8_t j = curExprLoc[0];
    uint8_t k = curExprLoc[1];

    // Step 2: Clear expression locations during processing
    // This prevents interference with register usage analysis
    curExprLoc[0] = 0;
    curExprLoc[1] = 0;

    // Step 3: Transform bitmask to align with register indices
    // Rotation pattern suggests non-standard input bit positions
    arg1b = (arg1b >> 3) | (arg1b << 5);

    // Step 4: Initial register usage analysis
    AnalyzeRegisterUsage();

    // Step 5: Process each register based on bitmask
    for (uint8_t i = 0; i <= 3; i++) {
        // Check if this register should be invalidated
        if (arg1b & 1) {
            // Save register if it needs preservation
            if (registerNeedsSave[i])
                PushRegisterToStack(i); // Push to stack

            // Clear register tracking - mark as empty
            registerIsDirect[i] = 0; // Clear direct value flag
            registerContents[i]  = 0; // Clear TX2 node association

            // Re-analyze register state after invalidation
            AnalyzeRegisterUsage();
        }

        // Rotate bitmask to test next register
        arg1b = (arg1b << 1) | (arg1b >> 7);
    }

    // Step 6: Restore original expression operand locations
    curExprLoc[0] = j;
    curExprLoc[1] = k;
}



void GenerateReturnSequence() {
    AdjustStackOnReturn(localVariableSize);
    info = blk[blkId].info;
    if (info && (info->flag & F_INTERRUPT)) {
        for (int i = IR_PSW; i <= IR_H; i++) {
            iCodeArgs[0] = i; /*  pop psw, pop b, pop d, pop h */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_POP);
        }
        EncodeFragData(CF_EI);
        codeSize += 5;
    }
}

void CreateConstantOrIdNode(uint16_t val, info_t *pInfo, uint8_t exprAttr, uint8_t exprLoc) {
    tx2[tx2qp].right    = val;
    tx2[tx2qp].left     = ToIdx(pInfo);
    tx2[tx2qp].exprAttr     = exprAttr;
    tx2[tx2qp].exprLoc     = exprLoc;
    tx2[tx2qp].nodeType = exprLoc == LOC_REG ? T2_NUMBER : T2_IDENTIFIER;
}

/*
Purpose
Retrieves the value and associated metadata from a TX2 node, determining both the numeric value and
information about how it's stored/accessed. Used during constant folding optimization to extract
operand values.
Parameters
•	Slot - Index of the TX2 node to examine
•	pAcc - Pointer to store the extracted value
•	pAccFlag - Pointer to store flags indicating the value's storage class and attributes
Functionality
The function examines the TX2 node type and extracts different information
based on whether it's an identifier or a number:
For T2_IDENTIFIER or T2_NUMBER nodes:
1.	Extracts the value:
  •	*pAcc = tx2[slot].right - The value is stored in the right field
2.	Determines the info pointer:
  •	info = FromIdx(tx2[slot].left) - Retrieves associated symbol information
3.	Sets the flag based on symbol attributes:
Condition	                                    Flag Value	            Meaning
No info OR F_MEMBER \| F_BASED \| F_ABSOLUTE	0	                    Absolute value / simple constant
F_AUTOMATIC	                                    0x100	                Automatic (stack) variable
F_EXTERNAL	                                    0x400 \| info->extId	External symbol with ID
F_MEMORY	                                    0x800	                Memory-mapped variable
type == PROC_T	                                0x1000 \| info->procId	Procedure with ID
F_DATA	                                        0x200	                Initialized data
Otherwise	                                    0x2000	                Uninitialized data
For other node types:
•	*pAcc = 0
•	*pAccFlag = slot ? 0x4000 : 0 - Uses slot value to distinguish between left/right operands
Usage Context
Called from:
•	FoldConstantExpr() - During constant folding optimization to get operand values
•	Other optimization passes that need to determine if an expression can be evaluated at compile time
The flag values encode both the storage class and additional metadata (like external ID or
procedure ID) in a single uint16_t, allowing the constant folding logic to determine:
•	Whether the value can be folded (flag == 0 for simple constants)
•	How to generate code if folding isn't possible
•	What fixup information is needed for external/procedure references
*/
void GetOperandValue(uint8_t slot, wpointer pAcc, wpointer pAccFlag) {
    if (tx2[slot].nodeType == T2_IDENTIFIER || tx2[slot].nodeType == T2_NUMBER) {
        *pAcc = tx2[slot].right;
        info  = FromIdx(tx2[slot].left);
        if (!info || (info->flag & (F_MEMBER | F_BASED | F_ABSOLUTE)))
            *pAccFlag = 0;
        else if ((info->flag & F_AUTOMATIC))
            *pAccFlag = 0x100;
        else if ((info->flag & F_EXTERNAL))
            *pAccFlag = 0x400 | info->extId;
        else if ((info->flag & F_MEMORY))
            *pAccFlag = 0x800;
        else if (info->type == PROC_T)
            *pAccFlag = 0x1000 | info->procId;
        else if ((info->flag & F_DATA))
            *pAccFlag = 0x200;
        else
            *pAccFlag = 0x2000;
    } else {
        *pAcc     = 0;
        *pAccFlag = slot ? 0x4000 : 0;
    }
}

void DecrementExprRefs() {
    for (uint8_t i = 0; i <= 1; i++) {
        uint8_t j = curExprLoc[i];
        if (j && --tx2[j].cnt == 0) {
            for (uint8_t k = 0; k <= 3; k++) {
                if (registerContents[k] == j)
                    registerContents[k] = 0;
            }
            stackNodeContents[tx2[j].extra] = 0;
        }
    }
}

void UpdateExpressionLookup(uint8_t arg1b) {
    iCodeArgsIndex[arg1b] = attrLocLookupTable[exprAttr[arg1b]][exprLoc[arg1b]];
}


/**
 * EncodeOperandInfo - Encode operand information into intermediate code arguments
 *
 * Analyzes a TX2 node and encodes its operand representation into the iCodeArgs[] array.
 * The encoding format depends on the operand type:
 *
 * 1. Stack-based operand (nodeControlMap & 0xC0 == 0):
 *    - Code: 0xA (stack reference)
 *    - Args: stack level, uint8_t offset from current stack pointer
 *
 * 2. Variable with symbol info:
 *    - Code: 0xC (automatic) or 0xB (non-automatic)
 *    - Args: offset from base, symbol index
 *    - For automatic: additional stack-relative offset
 *
 * 3. Immediate constant:
 *    - Code: 8 (uint8_t) or 9 (uint16_t)
 *    - Args: immediate value
 *
 * The iCodeArgsIndex  index is advanced as arguments are added, preparing data for
 * subsequent code generation via EncodeFragData().
 *
 * @param arg1b - Index of TX2 node to encode
 */


void EncodeOperandInfo(uint8_t arg1b) {
    if ((nodeControlMap[tx2[arg1b].nodeType] & 0xc0) == 0) {
        // Stack-based operand
        iCodeArgs[registersToSaveCount++] = 0xa;
        iCodeArgs[registersToSaveCount++] = tx2[arg1b].extra;
        iCodeArgs[registersToSaveCount++] = (currentStackDepth - tx2[arg1b].extra) * 2;
    } else if (tx2[arg1b].left) {
        // Variable with symbol
        info           = FromIdx(tx2[arg1b].left);
        iCodeArgs[registersToSaveCount++] = info->flag & F_AUTOMATIC ? 0xc : 0xb;
        iCodeArgs[registersToSaveCount++] = tx2[arg1b].right - info->linkVal;
        iCodeArgs[registersToSaveCount++] = ToIdx(info);
        if ((info->flag & F_AUTOMATIC))
            iCodeArgs[registersToSaveCount++] = tx2[arg1b].right + currentStackDepth * 2;
    } else {
        // Immediate constant
        iCodeArgs[registersToSaveCount++] = tx2[arg1b].right < 0x100 ? 8 : 9;
        iCodeArgs[registersToSaveCount++] = tx2[arg1b].right;
    }
}

void EncodeExpressionOperand(uint8_t arg1b) {
    if (exprLoc[arg1b] <= 3)
        iCodeArgs[registersToSaveCount++] = exprLoc[arg1b];
    else
        EncodeOperandInfo(curExprLoc[arg1b]);
}

void AdjustRegisterOffset(uint8_t irReg) {
    if (irReg > IR_H)
        return;
    if (registerDataType[irReg] <= 6) {
        while (registerOffset[irReg] != 0) {
            if (registerOffset[iCodeArgs[0] = irReg] > 0x7f) {
                registerOffset[irReg]++;
                EncodeFragData(CF_INX);
            } else {
                registerOffset[irReg]--;
                EncodeFragData(CF_DCX);
            }
            codeSize++;
        }
    }
}

void PopRegisterFromStack(uint8_t irReg) {
    iCodeArgs[0] = irReg;
    iCodeArgs[1] = LOC_STACK;
    iCodeArgs[2] = currentStackDepth;
    EncodeFragData(CF_POP);
    codeSize++;
    RestoreRegisterFromStack(irReg);
    currentStackDepth--;
}

void GetTx2Item() {
    tx2[tx2qp].nodeType = Rd2Byte();

    switch (3 & nodeControlMap[tx2[tx2qp].nodeType]) {
    case 0:
        if (tx2[tx2qp].nodeType == T2_EOF)
            eofSeen = true;
        break;
    case 1:
        tx2[tx2qp].left = Rd2Word();
        break;
    case 2:
        tx2[tx2qp].left  = Rd2Word();
        tx2[tx2qp].right = Rd2Word();
        break;
    case 3:
        tx2[tx2qp].left  = Rd2Word();
        tx2[tx2qp].right = Rd2Word();
        tx2[tx2qp].extra = Rd2Word();
        break;
    }
}

void MergeRedundantNodes() {
    /* T2_MODULE not needed if previous was public level label */
    // remove redundant module markers after local labels
    if (curNodeType == T2_MODULE) {
        if (tx2[tx2qp - 1].nodeType == T2_LABELDEF &&
            !(FromIdx(tx2[tx2qp - 1].left)->flag & (F_MODGOTO | F_PUBLIC)))
            tx2qp--; // ignore this item
    } else
        // merge consecutive T2_LINEINFO which have no statements
        if (curNodeType == T2_LINEINFO && tx2[tx2qp].right == 0 &&
            tx2[tx2qp - 1].nodeType == T2_LINEINFO && tx2[tx2qp - 1].right == 0) {
            tx2[tx2qp - 1].extra = tx2[tx2qp].left; // set first blkCnt to new lineCnt
            tx2qp--;                                // and ignore this item
        }
}

void FillTx2Queue() {

    tx2qp = 4; // leave fixed at bottom
    // remove any used items from the queue, moving down the others
    if (tx2qEnd > tx2qNxt) {
        uint8_t k = tx2qEnd - tx2qNxt;
        memmove(&tx2[tx2qp], &tx2[tx2qNxt], k * sizeof(tx2_t));
        tx2qp += k;
        // tx2qNxt += k; // removed as updated before used
    }
    bool exceeded = false;  // statement size error already reported
    bool endblk   = false; // basic block terminator seen
    while (tx2qp < 255 && !eofSeen) {
        GetTx2Item();
        curNodeType = tx2[tx2qp].nodeType;
        MergeRedundantNodes();
        if (tx2qp == 4) { //  first node of statement
            if (curNodeType == T2_STMTCNT || curNodeType == T2_LOCALLABEL || curNodeType == T2_EOF)
                tx2qp++;
            else if ((nodeControlMap[curNodeType] & 0x20)) // meta node so emit
                EmitTopItem();
            else if (!exceeded) {
                exceeded = true;
                Tx2SyntaxError(ERR200); /*  LIMIT EXCEEDED: STATEMENT SIZE */
                EmitTopItem();
            }
        } else
            tx2qp++;
        // T2_BEGMOVE, T2_CALL, T2_CALLVAR, T2_PROCEDURE, T2_GOTO, T2_JMP, T2_JNC, T2_JNZ, T2_ENDPROC
        if (nodeControlMap[curNodeType] & 8)
            endblk = true;
        if (endblk &&
            (curNodeType == T2_STMTCNT || curNodeType == T2_EOF || curNodeType == T2_LOCALLABEL))
            break;
    }
    tx2qEnd = tx2qp; // new end of queue
}

static void SkipStmt(uint8_t startIdx, int8_t direction) {
    tx2qNxt = startIdx + direction;
    while (tx2[tx2qNxt].nodeType != T2_STMTCNT && tx2[tx2qNxt].nodeType != T2_EOF &&
           tx2[tx2qNxt].nodeType != T2_LOCALLABEL)
        tx2qNxt += direction;
}

void SetFirstStatementEnd() {
    SkipStmt(tx2qEnd, -1);      // find the start of the last stmt in the queue (may be partial)
    if (tx2qNxt == 4 && !eofSeen)   // start seen so assume its after tx2qEnd 
        tx2qNxt = tx2qEnd;
    else {
        //
        for (uint8_t i = tx2qp = 4; tx2qp < tx2qNxt; tx2qp++) {
            nodeControlFlags = nodeControlMap[tx2[tx2qp].nodeType];
            if ((nodeControlFlags & 0x10)) {    // label or halt
                // PROCEDURE LOCALLABEL CASELABEL LABELDEF HALT EOF ENDPROC
                if (i < tx2qp) {    // we have items that generate code
                    if (tx2[tx2qp].nodeType == T2_LOCALLABEL)
                        tx2qNxt = tx2qp;
                    else
                        SkipStmt(tx2qp, -1);    // backup
                    return;
                }
                i++;
            }
            if ((nodeControlFlags & 8)) {   // Basic block end
                // JMPFALSE RETURNBYTE RETURNWORD RETURN BEGMOVE CALL CALLVAR PROCEDURE GOTO JMP JNC JNZ ENDPROC
                SkipStmt(tx2qp, 1); // skip to next statement
                return;
            }
            if ((nodeControlFlags & 0x20))  // none code item
                // STMTCNT LINEINFO MODULE SYNTAXERROR TOKENERROR EOF LIST NOLIST CODE NOCODE EJECT INCLUDE ERROR
                i++;
        }
    }
}
