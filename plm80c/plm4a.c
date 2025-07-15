/****************************************************************************
 *  plm4a.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

byte modHelperIdCnt[] = { // was also b4789 in plm3a.c
    2, 2, 3, 4, 3, 4, 2, 2, 3, 4, 2, 3, 2, 3, 3, 3, 3, 2, 2, 3, 4, 2, 3,
    2, 3, 2, 2, 2, 2, 3, 2, 2, 2, 3, 2, 3, 2, 2, 3, 2, 2, 1, 2, 2, 3, 4
};

byte modHelperId[] = {
    0,    2,    4,    7,    0xB,  0xE,  0x12, 0x14, 0x16, 0x19, 0x1D, 0x1F, 0x22, 0x24, 0x27, 0x2A,
    0x2D, 0x30, 0x32, 0x34, 0x37, 0x3B, 0x3D, 0x40, 0x42, 0x45, 0x47, 0x49, 0x4B, 0x4D, 0x50, 0x52,
    0x54, 0x56, 0x59, 0x5B, 0x5E, 0x60, 0x62, 0x65, 0x67, 0x69, 0x6A, 0x6C, 0x6E, 0x71
};

byte b4602[] = { 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA8, 0xA8, 0xA8,
                 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9,
                 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                 0xAA, 0xAA, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xA6,
                 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6,
                 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6 };
// clang-format off
// note UCF reflects the original spurious references to start offset 0xcf which
// was beyond the array.
char const *opcodes[] = { "SUB", "SBB", "RLC", "RRC", "RAL", "RAR", "SHL", "SHR",
                   "ADD", "ADC", "MUL", "DIV", "ANA", "ORA", "XRA", "NEG",
                   "NOT", "M10", "UCF", "CPI", "CMP", "SUI", "SBI", "ADI",
                   "ACI", "ANI", "ORI", "XRI", "INR", "INX", "DCR", "DCX",
                   "ADD\tA"};  

/* A B D H A C E L M */
byte regNo[]   = { 7, 0, 2, 4, 7, 1, 3, 5, 6 };
char const *regStr[]     = { "\x1" "A", "\x1" "B", "\x1" "D", "\x1H",
                             "\x1" "A", "\x1" "C", "\x1" "E", "\x1L",
                             "\x11M" };

byte regPairNo[]  = { 3, 0, 1, 2 };
char const *regPairStr[] = { "\x3PSW",  "\x1" "B", "\x1" "D", "\x1H" };

byte b473D[]      = { 0x90, 0x98, 7,    0xF,  0x17, 0x1F, 0, 0, 0x80, 0x88, 0,
                      0,    0xA0, 0xB0, 0xA8, 0,    0,    0, 0, 0xFE, 0xB8, 0xD6,
                      0xDE, 0xC6, 0xCE, 0xE6, 0xF6, 0xEE, 4, 3, 5,    0xB,  0x87 };

byte b475E[] = { 9, 0x1D, 0xC, 1, 0, 0, 1, 0, 0, 0, 0xD, 9, 2, 3, 4, 5, 0x20, 0, 1, 0x1F, 0, 0xE };

byte b4774[] = { 0x17, 0, 0x19, 0x13, 0, 0, 0x16, 0,    0, 0, 0x1A,
                 0x18, 3, 2,    5,    4, 0, 0,    0x15, 0, 0, 0x1B };

byte b478A[] = { 8, 0, 0xC, 0x14, 0, 0, 1, 0, 0, 0, 0xD, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xE };



/*
    indexed by cfCode
    low 12 bits are start of codeSeq
    top 4 bits are the length of the codeSeq
*/
word instuctionSeq[] = {
    0,      0,      0,      0,      0x1000, 0x1001, 0x2002, 0x1004, 0x1005, 0x1006, 0x1007, 0x2008,
    0x200A, 0x100C, 0x100D, 0x100E, 0x300F, 0x2012, 0x1014, 0x3015, 0x1018, 0x1019, 0x101A, 0x101B,
    0x101C, 0x101D, 0x101E, 0x101F, 0x1022, 0x2022, 0x3022, 0x4022, 0x2020, 0x1026, 0x2026, 0x3026,
    0x1029, 0x2029, 0x120F, 0x102C, 0x202C, 0x302C, 0x102F, 0x1030, 0x1031, 0x2032, 0x2034, 0x2036,
    0x3038, 0x303B, 0x303E, 0x3041, 0x3044, 0x3047, 0x304A, 0x304D, 0x3050, 0x1053, 0x2054, 0x1056,
    0x1057, 0x3058, 0x305B, 0x105E, 0x105E, 0x405F, 0x7063, 0x906A, 0xD073, 0x1083, 0x2083, 0x3083,
    0x4083, 0x5083, 0x4080, 0x3081, 0x2082, 0x4088, 0x308C, 0x208F, 0x2091, 0x3093, 0x4096, 0x509A,
    0x509F, 0x40A4, 0x30A8, 0x20AB, 0x20AE, 0x10AD, 0x10B0, 0x20B1, 0x20B3, 0x20B5, 0x10B7, 0x80B8,
    0x10C0, 0x10FC, 0x10C1, 0x10C2, 0x20C3, 0x10C5, 0x80C6, 0xA0CE, 0x102C, 0x202C, 0x102C, 0x202C,
    0x220D, 0x1210, 0x1211, 0x2212, 0x1022, 0x1214, 0x1215, 0x1216, 0x1217, 0x1218, 0x2219, 0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0x10D8, 0,      0,      0,      0x10D9, 0x10DA, 0x10DA, 0x10DB, 0x10DC,
    0x30DD, 0x30E0, 0x30E3, 0x10E6, 0x10E7, 0x10E8, 0x20E9, 0,      0,      0x10EB, 0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0x10EC, 0x20ED,
    0x40EF, 0x40F3, 0x20F7, 0x30F9
};

word helperStart[] = { 0x134, 0x137, 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x15A, 0x15C, 0x15F, 0x161, 0x14A,
                 0x14D, 0x151, 0x15A, 0x15C, 0x15F, 0x161, 0x134, 0x137, 0x13F, 0x142, 0x14A, 0x14D,
                 0x151, 0x15A, 0x15C, 0x15F, 0x161, 0x11A, 0x11D, 0xFD,  0x101, 0x103, 0x10B, 0x10E,
                 0x1F6, 0x1F9, 0x1FB, 0x1E9, 0x1EC, 0x1EE, 0x1DB, 0x1DE, 0x1E0, 0x1CE, 0x1D1, 0x1D3,
                 0x134, 0x137, 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x15A, 0x15C, 0x15F, 0x161, 0x183,
                 0x185, 0x1AE, 0x1B2, 0x1B4, 0x183, 0x185, 0x1BE, 0x1C2, 0x1C4, 0x183, 0x185, 0x183,
                 0x185, 0x134, 0x137, 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x177, 0x17A, 0x16A, 0x16D,
                 0x183, 0x185, 0x192, 0x196, 0x198, 0x18A, 0x18C, 0x19D, 0x1A1, 0x1A3, 0x134, 0x137,
                 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x177, 0x17A, 0x16A, 0x16D, 0x205, 0x134, 0x137,
                 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x15A, 0x15C, 0x15F, 0x161 };

byte helperLen[] = { 0xB,  8,    0xB, 8,    0x10, 0xD, 9,    0x10, 0xE, 0xB,  9,   0x10, 0xD,
                 9,    0x10, 0xE, 0xB,  9,    0xB, 8,    0xB,  8,   0x10, 0xD, 9,    0x10,
                 0xE,  0xB,  9,   0x1A, 0x17, 0xE, 0xA,  8,    0xF, 0xC,  0xF, 0xC,  0xA,
                 0xD,  0xA,  8,   0xE,  0xB,  9,   0xD,  0xA,  8,   0xB,  8,   0xB,  8,
                 0x10, 0xD,  9,   0x10, 0xE,  0xB, 9,    7,    5,   0x10, 0xC, 0xA,  7,
                 5,    0x10, 0xC, 0xA,  7,    5,   7,    5,    0xB, 8,    0xB, 8,    0x10,
                 0xD,  9,    0xC, 9,    0xD,  0xA, 7,    5,    0xB, 7,    5,   8,    6,
                 0x11, 0xD,  0xB, 0xB,  8,    0xB, 8,    0x10, 0xD, 9,    0xC, 9,    0xD,
                 0xA,  8,    0xB, 8,    0xB,  8,   0x10, 0xD,  9,   0x10, 0xE, 0xB,  9 };

// clang-format off
// code table is formatted as
// first byte: base value of opcode if not 0
// byte >= 0xc0 - modifies the first byte of the opcode
// byte > 0x80 - adds additional bytes
// byte == 0x80 - terminates sequence
// other bytes passed through as chars
byte codeTable[] = {
    /* 0000 */ "\x8D" "\tADC\tL" "\x80"
    /* 0008 */ "\xC6" "\tADI\t" "\x84\xFF\x80"
    /* 0011 */ "\xE6" "\tANI\t" "\x84\x1\x80"
    /* 001A */ "\xE6" "\tANI\t" "\x84\x3\x80"
    /* 0023 */ "\xE6" "\tANI\t" "\x84\x7\x80"
    /* 002C */ "\xE6" "\tANI\t" "\x84\x80"
    /* 0034 */ "\x80"
    /* 0035 */ "\xE6" "\tANI\t" "\x84\xc0\x80"
    /* 003E */ "\xE6" "\tANI\t" "\x84\xE0\x80"
    /* 0047 */ "\xE6" "\tANI\t" "\x84\xF8\x80"
    /* 0050 */ "\xE6" "\tANI\t" "\x84\xFC\x80"
    /* 0059 */ "\xE6" "\tANI\t" "\x84\xFE\x80"
    /* 0062 */ "\xE6" "\tANI\t" "\x84\xF0\x80"
    /* 006B */ "\xCD" "\tCALL\t" "\x82\x80"
    /* 0074 */ "\xCD" "\tCALL\t" "\x83 \x80"
    /* 007E */ "\xCD" "\tCALL\t" "\x93     \x80"
    /* 008C */ "\x2F" "\tCMA" "\x80"
    /* 0092 */ "\x27" "\tDAA" "\x80"
    /* 0098 */ "\x9" "\tDAD\tB" "\x80"
    /* 00A0 */ "\x29" "\tDAD\tH" "\x80"
    /* 00A8 */ "\x9" "\tDAD\t" "\xC0\x80"
    /* 00B0 */ "\x39" "\tDAD\tSP" "\x80"
    /* 00B9 */ "\x3D" "\tDCR\tA" "\x80"
    /* 00C1 */ "\xD" "\tDCR\tC" "\x80"
    /* 00C9 */ "\x1B" "\tDCX\tD" "\x80"
    /* 00D1 */ "\x2B" "\tDCX\tH" "\x80"
    /* 00D9 */ "\xF3" "\tDI" "\x80"
    /* 00DE */ "\0" "\tDW\t" "\x82\x80"
    /* 00E5 */ "\xFB" "\tEI" "\x80"
    /* 00EA */ "\0\x89" "AH:" "\x80"
    /* 00F0 */ "\0\x89" "APH:" "\x80"
    /* 00F7 */ "\0\x89" "B:" "\x80"
    /* 00FC */ "\0\x89" "BBA:" "\x80"
    /* 0103 */ "\0\x89" "BPH:" "\x80"
    /* 010A */ "\0\x89" "D:" "\x80"
    /* 010F */ "\0\x89" "DA:" "\x80"
    /* 0115 */ "\0\x89" "DB:" "\x80"
    /* 011B */ "\0\x89" "DBP:" "\x80"
    /* 0122 */ "\0\x89" "DH:" "\x80"
    /* 0128 */ "\0\x89" "DHP:" "\x80"
    /* 012F */ "\0\x89" "DPH:" "\x80"
    /* 0136 */ "\0\x89H:" "\x80"
    /* 013B */ "\0\x89PB:" "\x80"
    /* 0141 */ "\0\x89PD:" "\x80"
    /* 0147 */ "\0\x89PDA:" "\x80"
    /* 014E */ "\0\x89PDB:" "\x80"
    /* 0155 */ "\0\x89PDH:" "\x80"
    /* 015C */ "\0\x89PH:" "\x80"
    /* 0162 */ "\x76" "\tHLT" "\x80"
    /* 0168 */ "\xDB" "\tIN\t" "\x86\x80"
    /* 016F */ "\x3C" "\tINR\tA" "\x80"
    /* 0177 */ "\x13" "\tINX\tD" "\x80"
    /* 017F */ "\x23" "\tINX\tH" "\x80"
    /* 0187 */ "\xC2" "\tJ\x88\t" "\x82\x80"
    /* 018E */ "\xFA" "\tJM\t" "\x87\x4\x80"
    /* 0196 */ "\xC3" "\tJMP\t" "\x82\x80"
    /* 019E */ "\xD2" "\tJNC\t" "\x82\x80"
    /* 01A6 */ "\xD2" "\tJNC\t" "\x87\x4\x80"
    /* 01AF */ "\xD2" "\tJNC\t" "\x87\x5\x80"
    /* 01B8 */ "\xC2" "\tJNZ\t" "\x87\xFE\x80"
    /* 01C1 */ "\xC2" "\tJNZ\t" "\x87\xEC\x80"
    /* 01CA */ "\xC2" "\tJNZ\t" "\x87\xFD\x80"
    /* 01D3 */ "\xC2" "\tJNZ\t" "\x87\xF9\x80"
    /* 01DC */ "\xC2" "\tJNZ\t" "\x87\xF8\x80"
    /* 01E5 */ "\xC2" "\tJNZ\t" "\x87\xF7\x80"
    /* 01EE */ "\xEA" "\tJPE\t" "\x87\x4\x80"
    /* 01F7 */ "\xCA" "\tJZ\t" "\x87\x4\x80"
    /* 01FF */ "\x3A" "\tLDA\t" "\x82\x80"
    /* 0207 */ "\xA" "\tLDAX\t" "\xC0\x80"
    /* 0210 */ "\x2A" "\tLHLD\t" "\x82\x80"
    /* 0219 */ "\x21" "\tLXI\tH," "\x82\t\x8A\x80"
    /* 0225 */ "\x21" "\tLXI\tH," "\x87\xD\x80"
    /* 0230 */ "\x21" "\tLXI\tH," "\x87\x11\x80"
    /* 023B */ "\x21" "\tLXI\tH," "\x87\x8\x80"
    /* 0246 */ "\x21" "\tLXI\tH," "\x87\t" "\x80"
    /* 0251 */ "\x21" "\tLXI\tH," "\x94\0\x80"
    /* 025C */ "\x1" "\tLXI\t" "\xC0,\x92\x80"
    /* 0266 */ "\x31" "\tLXI\tSP," "\x85\x80"
    /* 0271 */ "\x7A" "\tMOV\tA,D" "\x80"
    /* 027B */ "\x7B" "\tMOV\tA,E" "\x80"
    /* 0285 */ "\x7C" "\tMOV\tA,H" "\x80"
    /* 028F */ "\x7D" "\tMOV\tA,L" "\x80"
    /* 0299 */ "\x78" "\tMOV\tA," "\xE0\x80"
    /* 02A3 */ "\x78" "\tMOV\tA," "\xE2\x80"
    /* 02AD */ "\x7E" "\tMOV\tA,M" "\x80"
    /* 02B7 */ "\x44" "\tMOV\tB,H" "\x80"
    /* 02C1 */ "\x4F" "\tMOV\tC,A" "\x80"
    /* 02CB */ "\x4D" "\tMOV\tC,L" "\x80"
    /* 02D5 */ "\x57" "\tMOV\tD,A" "\x80"
    /* 02DF */ "\x50" "\tMOV\tD,B" "\x80"
    /* 02E9 */ "\x56" "\tMOV\tD,M" "\x80"
    /* 02F3 */ "\x5F" "\tMOV\tE,A" "\x80"
    /* 02FD */ "\x59" "\tMOV\tE,C" "\x80"
    /* 0307 */ "\x5E" "\tMOV\tE,M" "\x80"
    /* 0311 */ "\x67" "\tMOV\tH,A" "\x80"
    /* 031B */ "\x66" "\tMOV\tH,M" "\x80"
    /* 0325 */ "\x6F" "\tMOV\tL,A" "\x80"
    /* 032F */ "\x46" "\tMOV\t" "\xD0,M\x80"
    /* 0339 */ "\x40" "\tMOV\t" "\xD2,\xe0\x80"
    /* 0343 */ "\x46" "\tMOV\t" "\xD2,M\x80"
    /* 034D */ "\x40" "\tMOV\t" "\xD0,\xe1\x80"
    /* 0357 */ "\x40" "\tMOV\t" "\xD2,\xe3\x80"
    /* 0361 */ "\x70" "\tMOV\tM," "\xE0\x80"
    /* 036B */ "\x70" "\tMOV\tM," "\xE2\x80"
    /* 0375 */ "\x3E" "\tMVI\tA," "\x84\x10\x80"
    /* 0380 */ "\x3E" "\tMVI\tA," "\x84\xFF\x80"
    /* 038B */ "\x3E" "\tMVI\tA," "\x84\0\x80"
    /* 0396 */ "\x6" "\tMVI\tB," "\x84\0\x80"
    /* 03A1 */ "\x16" "\tMVI\tD," "\x84\0\x80"
    /* 03AC */ "\x26" "\tMVI\tH," "\x84\0\x80"
    /* 03B7 */ "\x6" "\tMVI\t" "\xD2,\x96\x80"
    /* 03C1 */ "\x6" "\tMVI\t" "\xD0,\x84\0\x80"
    /* 03CC */ "\x36" "\tMVI\tM," "\x84\0\x80"
    /* 03D7 */ "\0" "\t" "\x9B\x80"
    /* 03DB */ "\0" "\t" "\x9B\t\xE5\x80"
    /* 03E1 */ "\0" "\t" "\x9B\t\xE7\x80"
    /* 03E7 */ "\0" "\t" "\x9B\t\xC0\x80"
    /* 03ED */ "\0" "\t" "\x9B\t\xEC\x80"
    /* 03F3 */ "\0" "\t" "\xAB\x80"
    /* 03F7 */ "\0" "\t" "\xAB\t\x86\x80"
    /* 03FD */ "\0" "\t" "\xBB\t\xEC\x80"
    /* 0403 */ "\0" "\t" "\x8B\t\xE9\x80"
    /* 0409 */ "\0" "\t" "\x8B\t\xEB\x80"
    /* 040F */ "\0" "\t" "\x8B\t\xEC\x80"
    /* 0415 */ "\0" "\t" "\x8B\t\xDC\x80"
    /* 041B */ "\xB7" "\tORA\tA" "\x80"
    /* 0423 */ "\xB5" "\tORA\tL" "\x80"
    /* 042B */ "\xD3" "\tOUT\t" "\x86\x80"
    /* 0433 */ "\xE9" "\tPCHL" "\x80"
    /* 043A */ "\0; PROC" "\t" "\x81\x80"
    /* 0444 */ "\xC1" "\tPOP\tB" "\x80"
    /* 044C */ "\xC1" "\tPOP\t" "\xC0\t\x8A\x80"
    /* 0456 */ "\xF1" "\tPOP\tPSW" "\x80"
    /* 0460 */ "\xE5" "\tPUSH\tH" "\x80"
    /* 0469 */ "\xC5" "\tPUSH\t" "\xC0\t\x8A\x80"
    /* 0474 */ "\xF5" "\tPUSH\tPSW" "\x80"
    /* 047F */ "\x17" "\tRAL" "\x80"
    /* 0485 */ "\x1F" "\tRAR" "\x80"
    /* 048B */ "\xC9" "\tRET" "\x80"
    /* 0491 */ "\x9F" "\tSBB\tA" "\x80"
    /* 0499 */ "\x98" "\tSBB\tB" "\x80"
    /* 04A1 */ "\x9C" "\tSBB\tH" "\x80"
    /* 04A9 */ "\x9E" "\tSBB\tM" "\x80"
    /* 04B1 */ "\x22" "\tSHLD\t" "\x82\x80"
    /* 04BA */ "\xF9" "\tSPHL" "\x80"
    /* 04C1 */ "\x32" "\tSTA\t" "\x82\x80"
    /* 04C9 */ "\x97" "\tSUB\tA" "\x80"
    /* 04D1 */ "\x91" "\tSUB\tC" "\x80"
    /* 04D9 */ "\x95" "\tSUB\tL" "\x80"
    /* 04E1 */ "\x96" "\tSUB\tM" "\x80"
    /* 04E9 */ "\xD6" "\tSUI\t" "\x84\x1\x80"
    /* 04F2 */ "\xEB" "\tXCHG" "\x80"
    /* 04F9 */ "\xE3" "\tXTHL\t" "\t\x8A\x80"
    /* 0503 */ "\x3" "\tINX\t" "\xC0\x80"
    /* 050B */ "\xB" "\tDCX\t" "\xC0\x80"
    /* 0513 */ "\x69" "\tMOV\tL,C" "\x80"
    /* 051D */ "\x60" "\tMOV\tH,B" "\x80"
    /* 0527 */ "\x4E" "\tMOV\tC,M" "\x80"
    /* 0531 */ "\x46" "\tMOV\tB,M" "\x80"
    /* 053B */ "\x1A" "\tLDAX\tD" "\x80"
    /* 0544 */ "\xA" "\tLDAX\tB" "\x80"
    /* 054D */ "\x12" "\tSTAX\tD" "\x80"
    /* 0556 */ "\xC2" "\tJNZ\t" "\x87\xFB\x80"
    /* 055F */ "\x6" "\tMVI\tB," "\x84\xC\x80"
    /* 056A */ "\x48" "\tMOV\tC,B" "\x80"
    /* 0574 */ "\xC2" "\tJNZ\t" "\x87\xFF\x80"
    /* 057D */ "\xC2" "\tJNZ\t" "\x87\xFA\x80"
    /* 0586 */ "\x21" "\tLXI\tH," "\x92\x80"
    /* 0590 */ "\x3" "\tINX\tB" "\x80"
    /* 0598 */ "\x78" "\tMOV\tA," "\xE3\x80"
    /* 05A2 */ "\x36" "\tMVI\tM," "\x86\x80"
    /* 05AC */ "\0" "\t" "\x8B\t\xD2\x80"
    /* 05B2 */ "\0" "\t" "\xBB\t\xE2\x80"
    /* 05B8 */ "\x2D" "\tDCR\tL" "\x80"
    /* 05C0 */ "\0" "\x89\x80"
    /* 05C3 */ "\x40" "\tMOV\t" "\xD0,\xe2\x80"
    /* 05CD */ "\x33" "\tINX\tSP" "\x80"
    /* 05D6 */ "\x3B" "\tDCX\tSP" "\x80"
    /* 05DF */ "\xDA" "\tJC\t" "\x82\x80"
    /* 05E6 */ "\x2C" "\tINR\tL" "\x80"
    /* 05EE */ "\xC3" "\tJMP\t" "\x87\x7\x80"
};

word codeSeq[] = {

/* 0x000 C: 4 */
   0x44c,
/* 0x001 C: 5 */
   0x4f9,
/* 0x002 C: 6 */
   0x219, 0xb0,
/* 0x004 C: 7 */
   0x25c,
/* 0x005 C: 8 */
   0x1ff,
/* 0x006 C: 9 */
   0x207,
/* 0x007 C: 10 */
   0x210,
/* 0x008 C: 11 */
   0x34d, 0x357,
/* 0x00a C: 12 */
   0x357, 0x3c1,
/* 0x00c C: 13 */
   0x357,
/* 0x00d C: 14 */
   0x4f2,
/* 0x00e C: 15 */
   0x3c1,
/* 0x00f C: 16 */
   0x343, 0x17f, 0x32f,
/* 0x012 C: 17 */
   0x343, 0x3c1,
/* 0x014 C: 18 */
   0x343,
/* 0x015 C: 19 */
   0x361, 0xd1, 0x36b,
/* 0x018 C: 20 */
   0x36b,
/* 0x019 C: 21 */
   0xde,
/* 0x01a C: 22 */
   0x4ba,
/* 0x01b C: 23 */
   0x469,
/* 0x01c C: 24 */
   0x503,
/* 0x01d C: 25 */
   0x50b,
/* 0x01e C: 26 */
   0xd1,
/* 0x01f C: 27 */
   0x48b,
/* 0x020 C: 32 */
   0x2a3, 0x48b,
/* 0x022 C: 28,29,30,31,112 */
   0xa0,  //End C: 112
   0xa0,  //End C: 29
   0xa0,  //End C: 30
   0xa0,
/* 0x026 C: 33,34,35 */
   0x415,  //End C: 33
   0x415,  //End C: 34
   0x415,
/* 0x029 C: 36,37 */
   0x5ac,  //End C: 36
   0x5ac,  //End C: 37
   0x5ac,
/* 0x02c > C: 39,40,41,104,105,106,107 */
   0x3e7,  //End C: 106
   0x3e7,  //End C: 107
   0x3e7,
/* 0x02f C: 42 */
   0x3fd,
/* 0x030 C: 43 */
   0x3f7,
/* 0x031 C: 44 */
   0x5b2,
/* 0x032 C: 45 */
   0x3fd, 0x491,
/* 0x034 C: 46 */
   0x3f7, 0x491,
/* 0x036 C: 47 */
   0x5b2, 0x491,
/* 0x038 C: 48 */
   0x3fd, 0x4e9, 0x491,
/* 0x03b C: 49 */
   0x3f7, 0x4e9, 0x491,
/* 0x03e C: 50 */
   0x5b2, 0x4e9, 0x491,
/* 0x041 C: 51 */
   0x3fd, 0x8, 0x491,
/* 0x044 C: 52 */
   0x3f7, 0x8, 0x491,
/* 0x047 C: 53 */
   0x5b2, 0x8, 0x491,
/* 0x04a C: 54 */
   0x3fd, 0x491, 0x8c,
/* 0x04d C: 55 */
   0x3f7, 0x491, 0x8c,
/* 0x050 C: 56 */
   0x5b2, 0x491, 0x8c,
/* 0x053 C: 57 */
   0xa8,
/* 0x054 C: 58 */
   0x210, 0x3ac,
/* 0x056 C: 59 */
   0x4b1,
/* 0x057 C: 60 */
   0x4c1,
/* 0x058 C: 61 */
   0x36b, 0x17f, 0x3cc,
/* 0x05b C: 62 */
   0x36b, 0x17f, 0x361,
/* 0x05e C: 63,64 */
   0x6b,
/* 0x05f C: 65 */
   0x23b, 0x460, 0x210, 0x433,
/* 0x063 C: 66 */
   0x246, 0x4f9, 0x2ad, 0x17f, 0x31b, 0x325, 0x433,
/* 0x06a C: 67 */
   0x225, 0x460, 0x219, 0xb0, 0x2ad, 0x17f, 0x31b, 0x325, 0x433,
/* 0x073 C: 68 */
   0x230, 0x460, 0x219, 0xb0, 0x2ad, 0x17f, 0x31b, 0x325, 0x2ad, 0x17f, 0x31b, 0x325, 0x433,
/* 0x080 C: 74 */
   0x3f3,
/* 0x081 > C: 75 */
   0x3f3,
/* 0x082 > C: 76 */
   0x3f3,
/* 0x083 > C: 69,70,71,72,73 */
   0x3d7,  //End C: 76
   0x3d7,  //End C: 70
   0x3d7,  //End C: 71
   0x3d7,  //End C: 72
   0x3d7,
/* 0x088 C: 77 */
   0x23, 0x485, 0x485, 0x485,
/* 0x08c C: 78 */
   0x1a, 0x485, 0x485,
/* 0x08f C: 79 */
   0x11, 0x485,
/* 0x091 C: 80 */
   0x41b, 0x485,
/* 0x093 C: 81 */
   0x59, 0x485, 0x485,
/* 0x096 C: 82 */
   0x50, 0x485, 0x485, 0x485,
/* 0x09a C: 83 */
   0x47, 0x485, 0x485, 0x485, 0x485,
/* 0x09f C: 84 */
   0x62, 0x47f, 0x47f, 0x47f, 0x47f,
/* 0x0a4 C: 85 */
   0x3e, 0x47f, 0x47f, 0x47f,
/* 0x0a8 C: 86 */
   0x35, 0x47f, 0x47f,
/* 0x0ab C: 87 */
   0x2c, 0x47f,
/* 0x0ad C: 89 */
   0xa8,
/* 0x0ae C: 88 */
   0xa8, 0xa8,
/* 0x0b0 C: 90 */
   0x8c,
/* 0x0b1 C: 91 */
   0x8c, 0x16f,
/* 0x0b3 C: 92 */
   0x503, 0x207,
/* 0x0b5 C: 93 */
   0x17f, 0x2ad,
/* 0x0b7 C: 94 */
   0x92,
/* 0x0b8 C: 95 */
   0x586, 0xa8, 0xa8, 0x307, 0x17f, 0x2e9, 0x4f2, 0x433,
/* 0x0c0 C: 96 */
   0x42b,
/* 0x0c1 C: 98 */
   0x2a3,
/* 0x0c2 C: 99 */
   0x299,
/* 0x0c3 C: 100 */
   0x485, 0x19e,
/* 0x0c5 C: 101 */
   0xa8,
/* 0x0c6 C: 102 */
   0x5e6, 0x5ee, 0x544, 0x54d, 0x590, 0x177, 0x5b8, 0x556,
/* 0x0ce C: 103 */
   0x17f, 0x5ee, 0x544, 0x54d, 0x590, 0x177, 0xd1, 0x285, 0x423, 0x1d3,
/* 0x0d8 C: 135 */
   0x43a,
/* 0x0d9 C: 139 */
   0x168,
/* 0x0da C: 140,141 */
   0x196,
/* 0x0db C: 142 */
   0x19e,
/* 0x0dc C: 143 */
   0x187,
/* 0x0dd C: 144 */
   0x380, 0x18e, 0x4c9,
/* 0x0e0 C: 145 */
   0x380, 0x1f7, 0x4c9,
/* 0x0e3 C: 146 */
   0x380, 0x1ee, 0x4c9,
/* 0x0e6 C: 147 */
   0x491,
/* 0x0e7 C: 148 */
   0xd9,
/* 0x0e8 C: 149 */
   0xe5,
/* 0x0e9 C: 150 */
   0xe5, 0x162,
/* 0x0eb C: 153 */
   0x266,
/* 0x0ec C: 166 */
   0x74,
/* 0x0ed C: 167 */
   0x74, 0x423,
/* 0x0ef C: 168 */
   0x74, 0x423, 0x4e9, 0x491,
/* 0x0f3 C: 169 */
   0x74, 0x423, 0x8, 0x491,
/* 0x0f7 C: 170 */
   0x74, 0x491,
/* 0x0f9 C: 171 */
   0x74, 0x491, 0x8c,
/* 0x0fc C: 97 */
   0x7e,
/* 0x0fd H: 31 */
   0x15c, 0x307, 0x17f, 0x2e9,
/* 0x101 > H: 32 */
   0x10a, 0x4f2,
/* 0x103 > H: 33 */
   0x136, 0xa0, 0x460, 0xa0, 0xa0, 0x444, 0x98, 0x48b,
/* 0x10b H: 34 */
   0x122, 0x2b7, 0x2cb,
/* 0x10e > H: 35 */
   0x115, 0x251, 0x375, 0xa0, 0x4f2, 0xa0, 0x4f2, 0x1a6, 0x98, 0xb9, 0x1e5, 0x48b,
/* 0x11a H: 29 */
   0x122, 0x2b7, 0x2cb,
/* 0x11d > H: 30 */
   0x115, 0x251, 0x375, 0x474, 0xa0, 0x4f2, 0x4c9, 0xa0, 0x4f2, 0x0, 0x4d1, 0x325, 0x285, 0x499, 0x311, 0x177, 0x1af, 0x98, 0xc9, 0x456, 0xb9, 0x1c1, 0x48b,
/* 0x134 H: 0,18,48,73,94,106 */
   0xea, 0x2f3, 0x3a1,
/* 0x137 > H: 1,19,49,74,95,107 */
   0x122, 0x27b, 0x409, 0x325, 0x271, 0x3e1, 0x311, 0x48b,
/* 0x13f H: 2,20,50,75,96,108 */
   0x10f, 0x2c1, 0x396,
/* 0x142 > H: 3,21,51,76,97,109 */
   0x115, 0x27b, 0x403, 0x325, 0x271, 0x3db, 0x311, 0x48b,
/* 0x14a H: 4,11,22,52,77,98,110 */
   0x11b, 0x513, 0x51d,
/* 0x14d > H: 5,12,23,53,78,99,111 */
   0x128, 0x527, 0x17f, 0x531,
/* 0x151 > H: 6,13,24,54,79,100,112 */
   0x14e, 0x53b, 0x403, 0x325, 0x177, 0x53b, 0x3db, 0x311, 0x48b,
/* 0x15a H: 7,14,25,55,113 */
   0x147, 0x4f2,
/* 0x15c > H: 8,15,26,56,114 */
   0xf0, 0x2f3, 0x3a1,
/* 0x15f > H: 9,16,27,57,115 */
   0x12f, 0x4f2,
/* 0x161 > H: 10,17,28,58,116 */
   0x155, 0x53b, 0x409, 0x325, 0x177, 0x53b, 0x3e1, 0x311, 0x48b,
/* 0x16a H: 82,103 */
   0xf0, 0x2f3, 0x3a1,
/* 0x16d > H: 83,104 */
   0x12f, 0x27b, 0x40f, 0x2f3, 0x271, 0x17f, 0x3ed, 0x2d5, 0x4f2, 0x48b,
/* 0x177 H: 80,101 */
   0x147, 0x325, 0x3ac,
/* 0x17a > H: 81,102 */
   0x155, 0x53b, 0x409, 0x325, 0x177, 0x53b, 0x3e1, 0x311, 0x48b,
/* 0x183 H: 59,64,69,71,84 */
   0x103, 0x2ad,
/* 0x185 > H: 60,65,70,72,85 */
   0xfc, 0x3d7, 0xc1, 0x1b8, 0x48b,
/* 0x18a H: 89 */
   0x103, 0x2ad,
/* 0x18c > H: 90 */
   0xfc, 0x41b, 0x485, 0xc1, 0x1ca, 0x48b,
/* 0x192 H: 86 */
   0x15c, 0x307, 0x17f, 0x2e9,
/* 0x196 > H: 87 */
   0x10a, 0x4f2,
/* 0x198 > H: 88 */
   0x136, 0xa0, 0xc1, 0x1b8, 0x48b,
/* 0x19d H: 91 */
   0x15c, 0x307, 0x17f, 0x2e9,
/* 0x1a1 > H: 92 */
   0x10a, 0x4f2,
/* 0x1a3 > H: 93 */
   0x136, 0x285, 0x41b, 0x485, 0x311, 0x28f, 0x485, 0x325, 0xc1, 0x1dc, 0x48b,
/* 0x1ae H: 61 */
   0x15c, 0x307, 0x17f, 0x2e9,
/* 0x1b2 > H: 62 */
   0x10a, 0x4f2,
/* 0x1b4 > H: 63 */
   0x136, 0x28f, 0x47f, 0x325, 0x285, 0x47f, 0x311, 0xc1, 0x1d3, 0x48b,
/* 0x1be H: 66 */
   0x15c, 0x307, 0x17f, 0x2e9,
/* 0x1c2 > H: 67 */
   0x10a, 0x4f2,
/* 0x1c4 > H: 68 */
   0x136, 0x285, 0x485, 0x311, 0x28f, 0x485, 0x325, 0xc1, 0x1d3, 0x48b,
/* 0x1ce H: 45 */
   0xf7, 0x2fd, 0x2df,
/* 0x1d1 > H: 46 */
   0x10a, 0x4f2,
/* 0x1d3 > H: 47 */
   0x136, 0x28f, 0x8c, 0x325, 0x285, 0x8c, 0x311, 0x48b,
/* 0x1db H: 42 */
   0x13b, 0x513, 0x51d,
/* 0x1de > H: 43 */
   0x15c, 0x4f2,
/* 0x1e0 > H: 44 */
   0x141, 0x53b, 0x8c, 0x325, 0x177, 0x53b, 0x8c, 0x311, 0x48b,
/* 0x1e9 H: 39 */
   0xf7, 0x2fd, 0x2df,
/* 0x1ec > H: 40 */
   0x10a, 0x4f2,
/* 0x1ee > H: 41 */
   0x136, 0x4c9, 0x4d9, 0x325, 0x38b, 0x4a1, 0x311, 0x48b,
/* 0x1f6 H: 36 */
   0x13b, 0x2fd, 0x2df,
/* 0x1f9 > H: 37 */
   0x141, 0x4f2,
/* 0x1fb > H: 38 */
   0x15c, 0x4c9, 0x4e1, 0x2f3, 0x38b, 0x17f, 0x4a9, 0x2d5, 0x4f2, 0x48b,
/* 0x205 H: 105 */
   0x5c0, 0x55f, 0x56a, 0xc1, 0x574, 0xb9, 0x57d, 0x48b,
/* 0x20d C: 108 */
   0x598, 0x42b,
/* 0x20f C: 38 */
   0x5a2,
/* 0x210 C: 109 */
   0x3b7,
/* 0x211 C: 110 */
   0x339,
/* 0x212 C: 111 */
   0xa0, 0xa8,
/* 0x214 C: 113 */
   0x5c3,
/* 0x215 C: 114 */
   0x32f,
/* 0x216 C: 115 */
   0x361,
/* 0x217 C: 116 */
   0x5cd,
/* 0x218 C: 117 */
   0x5d6,
/* 0x219 C: 118 */
   0x485, 0x5df,

};
// clang-formats on
byte stackOrigin[] = "\xE@STACK$ORIGIN ";

static void EmitSource() {
    word endLine;

    word line = Rd1Word();
    word stmt = Rd1Word();
    word blk  = Rd1Word();
    if (stmt > 0 || blk == 0)
        endLine = line;
    else {
        endLine = blk;
        blk     = 0;
    }
    for (; line <= endLine; line++) {
        EmitLinePrefix();
        lineNo  = line;
        stmtCnt = stmt;
        blkCnt  = blk;
        GetSourceLine();
    }
}

static void AddrCheck(word arg1w) {
    if (arg1w != baseAddr)
        fatal_ov46(0xD8);
}

static void NewStatementNo() {
    stmtNo = Rd1Word();
    if (stmtNo == 0)
        return;
    if (DEBUG) {
        if (getWord(&recLineNum[REC_LEN]) + 4 >= 1020)
            WriteRec(recLineNum, 1);
        RecAddWord(recLineNum, 1, baseAddr);
        RecAddWord(recLineNum, 1, stmtNo);
    }
    if (codeOn)
        EmitStatementNo();
}

static void EmitLocalLabel() {
    locLabelNum = Rd1Word();
    AddrCheck(localLabels[locLabelNum]);
    sprintf(locPStr, "@%d", locLabelNum);
    EmitLabel(locPStr);
}

static void EmitSymLabel() {
    info = FromIdx(Rd1Word());
    curSym = info->sym;
    AddrCheck(info->linkVal);
    EmitLabel(curSym->name->str);
}

static void EmitSimpleError() {
    errData.num  = Rd1Word();
    errData.info = 0;
    errData.stmt = stmtNo;
    EmitError();
}

static void EmitNearError() {
    errData.num  = Rd1Word();
    errData.info = Rd1Word();
    errData.stmt = stmtNo;
    EmitError();
}

static void EmitFullError() {
    errData.num  = Rd1Word();
    errData.info = Rd1Word();
    errData.stmt = Rd1Word();
    EmitError();
}

static void SetNewAddr() {
    info = FromIdx(Rd1Word());
    baseAddr = Rd1Word();
    baseAddr += info->linkVal;
    FlushRecs();
}

void Sub_54BA() {
    cfCode = Rd1Byte();
    if (cfCode == T2_LINEINFO)
        EmitSource();
    else if (cfCode == T2_SETSTMTNO)
        stmtNo = Rd1Word();
    else if (cfCode == T2_STMTCNT)
        NewStatementNo();
    else if (cfCode == T2_LOCALLABEL || cfCode == T2_CASELABEL)
        EmitLocalLabel();
    else if (cfCode == T2_LABELDEF)
        EmitSymLabel();
    else if (cfCode == T2_SETADDR)
        SetNewAddr();
    else if (cfCode == T2_SYNTAXERROR)
        EmitSimpleError();
    else if (cfCode == T2_TOKENERROR)
        EmitNearError();
    else if (T2_LIST <= cfCode && cfCode <= T2_INCLUDE)
        MiscControl(&utf1);
    else if (cfCode == T2_EOF)
        morePass4 = false;
    else if (cfCode == T2_ERROR)
        EmitFullError();
    else
        Sub_668B();
}
