/****************************************************************************
 *  plm80: C port of Intel's ISIS-II PLM80 v4.0                             *
 *  Copyright (C) 2020 Mark Ogden <mark.pm.ogden@btinternet.com>            *
 *                                                                          *
 *  This program is free software; you can redistribute it and/or           *
 *  modify it under the terms of the GNU General Public License             *
 *  as published by the Free Software Foundation; either version 2          *
 *  of the License, or (at your option) any later version.                  *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program; if not, write to the Free Software             *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,              *
 *  MA  02110-1301, USA.                                                    *
 *                                                                          *
 ****************************************************************************/


#include "plm.h"

byte b42A8[] = {    // was also b4789 in plm3a.c
    2, 2, 3, 4, 3, 4, 2, 2, 
    3, 4, 2, 3, 2, 3, 3, 3, 
    3, 2, 2, 3, 4, 2, 3, 2, 
    3, 2, 2, 2, 2, 3, 2, 2, 
    2, 3, 2, 3, 2, 2, 3, 2, 
    2, 1, 2, 2, 3, 4};


byte b42D6[] = {        // was also b47B7 in plm3a.c minus duplicate of b4304 entries
      0,  2,  4,  7,0xB,0xE,0x12,0x14,
    0x16,0x19,0x1D,0x1F,0x22,0x24,0x27,0x2A,
    0x2D,0x30,0x32,0x34,0x37,0x3B,0x3D,0x40,
    0x42,0x45,0x47,0x49,0x4B,0x4D,0x50,0x52,
    0x54,0x56,0x59,0x5B,0x5E,0x60,0x62,0x65,
    0x67,0x69,0x6A,0x6C,0x6E,0x71};






byte b4602[] = {
    0xA7,0xA7,0xA7,0xA7,0xA7,0xA7,0xA7,0xA7,
    0xA7,0xA7,0xA7,0xA8,0xA8,0xA8,0xA8,0xA8,
    0xA8,0xA8,0xA8,0xA8,0xA8,0xA8,0xA9,0xA9,
    0xA9,0xA9,0xA9,0xA9,0xA9,0xA9,0xA9,0xA9,
    0xA9,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
    0xAA,0xAA,0xAA,0xAA,0xAB,0xAB,0xAB,0xAB,
    0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,0xA6,
    0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,
    0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,
    0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,
    0xA6};

byte opcodes[] = 
    "\x3SUB" "\x3SBB" "\x3RLC" "\x3RRC"
    "\x3RAL" "\x3RAR" "\x3Shl" "\x3Shr"
    "\x3" "ADD" "\x3" "ADC" "\x3MUL" "\x3" "DIV"
    "\x3" "ANA" "\x3ORA" "\x3XRA" "\x3NEG"
    "\x3" "not" "\x3M10" "\x3" "CPI" "\x3" "CMP"
    "\x3SUI" "\x3SBI" "\x3" "ADI" "\x3" "ACI"
    "\x3" "ANI" "\x3ORI" "\x3XRI" "\x3INR"
    "\x3INX" "\x3" "DCR" "\x3" "DCX" "\x5" "ADD\tA"
    "\x3PSW" "\x1" "A" "\x1" "B" "\x1" "C"
    "\x1" "D" "\x1" "E" "\x1H" "\x1L"
    "\x1M" "\x2" "AH" "\x2" "DH" "\x2" "DA"
    "\x2" "DB" "\x3" "DBP" "\x3" "DHP" "\x3" "PDB"
    "\x3" "APH" "\x3" "DPH" "\x3" "PDA" "\x3" "PDH"
    "\x2" "PB" "\x2" "PD" "\x2" "PH" "\x3" "BPH"
    "\x3" "BBA";

byte regNo[] = {7, 0, 2, 4, 7, 1, 3, 5, 6};
                    /* A B D H A C E L M */
byte regIdx[] = {0x86,0x88,0x8C,0x90,0x86,0x8A,0x8E,0x92,0x94};
byte stkRegNo[] = {3, 0, 1, 2};
byte stkRegIdx[] = {0x82, 0x88, 0x8C, 0x90};  /* psw b d h */

byte b473D[] = {
    0x90, 0x98, 7, 0xF, 0x17, 0x1F, 0, 0, 
    0x80, 0x88, 0, 0, 0xA0, 0xB0, 0xA8, 0, 
    0, 0, 0, 0xFE, 0xB8, 0xD6, 0xDE, 0xC6, 
    0xCE, 0xE6, 0xF6, 0xEE, 4, 3, 5, 0xB, 
    0x87};

byte b475E[] = {
    9, 0x1D, 0xC, 1, 0, 0, 1, 0, 
    0, 0, 0xD, 9, 2, 3, 4, 5, 
    0x20, 0, 1, 0x1F, 0, 0xE};

byte b4774[] = {
    0x17, 0, 0x19, 0x13, 0, 0, 0x16, 0, 0, 0, 0x1A, 0x18, 3, 
    2, 5, 4, 0, 0, 0x15, 0, 0, 0x1B};

byte b478A[] = {
    8, 0, 0xC, 0x14, 0, 0, 1, 0, 
    0, 0, 0xD, 9, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0xE};

byte b47A0[] = {
    0,4,8,0xC,0x10,0x14,0x18,0x1C,
    0x20,0x24,0x28,0x2C,0x30,0x34,0x38,0x3C,
    0x40,0x44,0xCF,0x48,0x4C,0x50,0x54,0x58,
    0x5C,0x60,0x64,0x68,0x6C,0x70,0x74,0x78,
    0x7C};

word w47C1[] = {
    0,0,0,0,0x1000,0x1001,0x2002,0x1004,
    0x1005,0x1006,0x1007,0x2008,0x200A,0x100C,0x100D,0x100E,
    0x300F,0x2012,0x1014,0x3015,0x1018,0x1019,0x101A,0x101B,
    0x101C,0x101D,0x101E,0x101F,0x1022,0x2022,0x3022,0x4022,
    0x2020,0x1026,0x2026,0x3026,0x1029,0x2029,0x120F,0x102C,
    0x202C,0x302C,0x102F,0x1030,0x1031,0x2032,0x2034,0x2036,
    0x3038,0x303B,0x303E,0x3041,0x3044,0x3047,0x304A,0x304D,
    0x3050,0x1053,0x2054,0x1056,0x1057,0x3058,0x305B,0x105E,
    0x105E,0x405F,0x7063,0x906A,0xD073,0x1083,0x2083,0x3083,
    0x4083,0x5083,0x4080,0x3081,0x2082,0x4088,0x308C,0x208F,
    0x2091,0x3093,0x4096,0x509A,0x509F,0x40A4,0x30A8,0x20AB,
    0x20AE,0x10AD,0x10B0,0x20B1,0x20B3,0x20B5,0x10B7,0x80B8,
    0x10C0,0x10FC,0x10C1,0x10C2,0x20C3,0x10C5,0x80C6,0xA0CE,
    0x102C,0x202C,0x102C,0x202C,0x220D,0x1210,0x1211,0x2212,
    0x1022,0x1214,0x1215,0x1216,0x1217,0x1218,0x2219,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0x10D8,
    0,0,0,0x10D9,0x10DA,0x10DA,0x10DB,0x10DC,
    0x30DD,0x30E0,0x30E3,0x10E6,0x10E7,0x10E8,0x20E9,0,
    0,0x10EB,0,0,0,0,0,0,
    0,0,0,0,0,0,0x10EC,0x20ED,
    0x40EF,0x40F3,0x20F7,0x30F9};

word w4919[] = {
    0x134,0x137,0x13F,0x142,0x14A,0x14D,0x151,0x15A,
    0x15C,0x15F,0x161,0x14A,0x14D,0x151,0x15A,0x15C,
    0x15F,0x161,0x134,0x137,0x13F,0x142,0x14A,0x14D,
    0x151,0x15A,0x15C,0x15F,0x161,0x11A,0x11D,0xFD,
    0x101,0x103,0x10B,0x10E,0x1F6,0x1F9,0x1FB,0x1E9,
    0x1EC,0x1EE,0x1DB,0x1DE,0x1E0,0x1CE,0x1D1,0x1D3,
    0x134,0x137,0x13F,0x142,0x14A,0x14D,0x151,0x15A,
    0x15C,0x15F,0x161,0x183,0x185,0x1AE,0x1B2,0x1B4,
    0x183,0x185,0x1BE,0x1C2,0x1C4,0x183,0x185,0x183,
    0x185,0x134,0x137,0x13F,0x142,0x14A,0x14D,0x151,
    0x177,0x17A,0x16A,0x16D,0x183,0x185,0x192,0x196,
    0x198,0x18A,0x18C,0x19D,0x1A1,0x1A3,0x134,0x137,
    0x13F,0x142,0x14A,0x14D,0x151,0x177,0x17A,0x16A,
    0x16D,0x205,0x134,0x137,0x13F,0x142,0x14A,0x14D,
    0x151,0x15A,0x15C,0x15F,0x161};

byte b4A03[] = {
    0xB, 8, 0xB, 8, 0x10, 0xD, 9, 0x10, 
    0xE, 0xB, 9, 0x10, 0xD, 9, 0x10, 0xE, 
    0xB, 9, 0xB, 8, 0xB, 8, 0x10, 0xD, 
    9, 0x10, 0xE, 0xB, 9, 0x1A, 0x17, 0xE, 
    0xA, 8, 0xF, 0xC, 0xF, 0xC, 0xA, 0xD, 
    0xA, 8, 0xE, 0xB, 9, 0xD, 0xA, 8, 
    0xB, 8, 0xB, 8, 0x10, 0xD, 9, 0x10, 
    0xE, 0xB, 9, 7, 5, 0x10, 0xC, 0xA, 
    7, 5, 0x10, 0xC, 0xA, 7, 5, 7, 
    5, 0xB, 8, 0xB, 8, 0x10, 0xD, 9, 
    0xC, 9, 0xD, 0xA, 7, 5, 0xB, 7, 
    5, 8, 6, 0x11, 0xD, 0xB, 0xB, 8, 
    0xB, 8, 0x10, 0xD, 9, 0xC, 9, 0xD, 
    0xA, 8, 0xB, 8, 0xB, 8, 0x10, 0xD, 
    9, 0x10, 0xE, 0xB, 9};

byte b4A78[] = {
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
/* 05EE */ "\xC3" "\tJMP\t" "\x87\x7\x80"};

word w506F[] = {
/*  A4-1 */ 0x44c,
/*  A5-1 */ 0x4f9,
/*  A6-2 */ 0x219, 0xb0,
/*  A7-1 */ 0x25c,
/*  A8-1 */ 0x1ff,
/*  A9-1 */ 0x207,
/*  A10-1 */ 0x210,
/*  A11-2 */ 0x34d, 0x357,
/*  A12-2 */ 0x357, 0x3c1,
/*  A13-1 */ 0x357,
/*  A14-1 */ 0x4f2,
/*  A15-1 */ 0x3c1,
/*  A16-3 */ 0x343, 0x17f, 0x32f,
/*  A17-2 */ 0x343, 0x3c1,
/*  A18-1 */ 0x343,
/*  A19-3 */ 0x361, 0xd1, 0x36b,
/*  A20-1 */ 0x36b,
/*  A21-1 */ 0xde,
/*  A22-1 */ 0x4ba,
/*  A23-1 */ 0x469,
/*  A24-1 */ 0x503,
/*  A25-1 */ 0x50b,
/*  A26-1 */ 0xd1,
/*  A27-1 */ 0x48b,
/*  A32-2 */ 0x2a3, 0x48b,
/*  A28-1 A29-2 A30-3 A31-4 A112-1 */ 0xa0, 0xa0, 0xa0, 0xa0,
/*  A33-1 A34-2 A35-3 */ 0x415, 0x415, 0x415,
/*  A36-1 A37-2 */ 0x5ac, 0x5ac, 0x5ac,
/*  A39-1 A40-2 A41-3 A104-1 A105-2 A106-1 A107-2 */ 0x3e7, 0x3e7, 0x3e7,
/*  A42-1 */ 0x3fd,
/*  A43-1 */ 0x3f7,
/*  A44-1 */ 0x5b2,
/*  A45-2 */ 0x3fd, 0x491,
/*  A46-2 */ 0x3f7, 0x491,
/*  A47-2 */ 0x5b2, 0x491,
/*  A48-3 */ 0x3fd, 0x4e9, 0x491,
/*  A49-3 */ 0x3f7, 0x4e9, 0x491,
/*  A50-3 */ 0x5b2, 0x4e9, 0x491,
/*  A51-3 */ 0x3fd, 8, 0x491,
/*  A52-3 */ 0x3f7, 8, 0x491,
/*  A53-3 */ 0x5b2, 8, 0x491,
/*  A54-3 */ 0x3fd, 0x491, 0x8c,
/*  A55-3 */ 0x3f7, 0x491, 0x8c,
/*  A56-3 */ 0x5b2, 0x491, 0x8c,
/*  A57-1 */ 0xa8,
/*  A58-2 */ 0x210, 0x3ac,
/*  A59-1 */ 0x4b1,
/*  A60-1 */ 0x4c1,
/*  A61-3 */ 0x36b, 0x17f, 0x3cc,
/*  A62-3 */ 0x36b, 0x17f, 0x361,
/*  A63-1 A64-1 */ 0x6b,
/*  A65-4 */ 0x23b, 0x460, 0x210, 0x433,
/*  A66-7 */ 0x246, 0x4f9, 0x2ad, 0x17f, 0x31b, 0x325, 0x433,
/*  A67-9 */ 0x225, 0x460, 0x219, 0xb0, 0x2ad, 0x17f, 0x31b, 0x325, 0x433,
/*  A68-13 */ 0x230, 0x460, 0x219, 0xb0, 0x2ad, 0x17f, 0x31b, 0x325, 0x2ad, 0x17f, 0x31b, 0x325, 0x433,
/*  A74-4 */ 0x3f3,
/*  A75-3 */ 0x3f3,
/*  A76-2 */ 0x3f3,
/*  A69-1 A70-2 A71-3 A72-4 A73-5 */ 0x3d7, 0x3d7, 0x3d7, 0x3d7, 0x3d7,
/*  A77-4 */ 0x23, 0x485, 0x485, 0x485,
/*  A78-3 */ 0x1a, 0x485, 0x485,
/*  A79-2 */ 0x11, 0x485,
/*  A80-2 */ 0x41b, 0x485,
/*  A81-3 */ 0x59, 0x485, 0x485,
/*  A82-4 */ 0x50, 0x485, 0x485, 0x485,
/*  A83-5 */ 0x47, 0x485, 0x485, 0x485, 0x485,
/*  A84-5 */ 0x62, 0x47f, 0x47f, 0x47f, 0x47f,
/*  A85-4 */ 0x3e, 0x47f, 0x47f, 0x47f,
/*  A86-3 */ 0x35, 0x47f, 0x47f,
/*  A87-2 */ 0x2c, 0x47f,
/*  A89-1 */ 0xa8,
/*  A88-2 */ 0xa8, 0xa8,
/*  A90-1 */ 0x8c,
/*  A91-2 */ 0x8c, 0x16f,
/*  A92-2 */ 0x503, 0x207,
/*  A93-2 */ 0x17f, 0x2ad,
/*  A94-1 */ 0x92,
/*  A95-8 */ 0x586, 0xa8, 0xa8, 0x307, 0x17f, 0x2e9, 0x4f2, 0x433,
/*  A96-1 */ 0x42b,
/*  A98-1 */ 0x2a3,
/*  A99-1 */ 0x299,
/*  A100-2 */ 0x485, 0x19e,
/*  A101-1 */ 0xa8,
/*  A102-8 */ 0x5e6, 0x5ee, 0x544, 0x54d, 0x590, 0x177, 0x5b8, 0x556,
/*  A103-10 */ 0x17f, 0x5ee, 0x544, 0x54d, 0x590, 0x177, 0xd1, 0x285, 0x423, 0x1d3,
/*  A135-1 */ 0x43a,
/*  A139-1 */ 0x168,
/*  A140-1 A141-1 */ 0x196,
/*  A142-1 */ 0x19e,
/*  A143-1 */ 0x187,
/*  A144-3 */ 0x380, 0x18e, 0x4c9,
/*  A145-3 */ 0x380, 0x1f7, 0x4c9,
/*  A146-3 */ 0x380, 0x1ee, 0x4c9,
/*  A147-1 */ 0x491,
/*  A148-1 */ 0xd9,
/*  A149-1 */ 0xe5,
/*  A150-2 */ 0xe5, 0x162,
/*  A153-1 */ 0x266,
/*  A166-1 */ 0x74,
/*  A167-2 */ 0x74, 0x423,
/*  A168-4 */ 0x74, 0x423, 0x4e9, 0x491,
/*  A169-4 */ 0x74, 0x423, 8, 0x491,
/*  A170-2 */ 0x74, 0x491,
/*  A171-3 */ 0x74, 0x491, 0x8c,
/*  A97-1 */ 0x7e,
/*  B31-14 */ 0x15c, 0x307, 0x17f, 0x2e9,
/*  B32-10 */ 0x10a, 0x4f2,
/*  B33-8 */ 0x136, 0xa0, 0x460, 0xa0, 0xa0, 0x444, 0x98, 0x48b,
/*  B34-15 */ 0x122, 0x2b7, 0x2cb,
/*  B35-12 */ 0x115, 0x251, 0x375, 0xa0, 0x4f2, 0xa0, 0x4f2, 0x1a6, 0x98, 0xb9, 0x1e5, 0x48b,
/*  B29-26 */ 0x122, 0x2b7, 0x2cb,
/*  B30-23 */ 0x115, 0x251, 0x375, 0x474, 0xa0, 0x4f2, 0x4c9, 0xa0, 0x4f2, 0, 0x4d1, 0x325,
          0x285, 0x499, 0x311, 0x177, 0x1af, 0x98, 0xc9, 0x456, 0xb9, 0x1c1, 0x48b,
/*  B0 B18 B48 B73 B94 B106 -11 */ 0xea, 0x2f3, 0x3a1,
/*  B1 B19 B49 B74 B95 B107 -8 */ 0x122, 0x27b, 0x409, 0x325, 0x271, 0x3e1, 0x311, 0x48b,
/*  B2 B20 B50 B75 B96 B108 -11 */ 0x10f, 0x2c1, 0x396,
/*  B3 B21 B51 B76 B97 B109 -8 */ 0x115, 0x27b, 0x403, 0x325, 0x271, 0x3db, 0x311, 0x48b,
/*  B4 B11 B22 B52 B77 B98 B110 -16 */ 0x11b, 0x513, 0x51d,
/*  B5 B12 B23 B53 B78 B99 B111 -13 */ 0x128, 0x527, 0x17f, 0x531,
/*  B6 B13 B24 B54 B79 B100 B112 -9 */ 0x14e, 0x53b, 0x403, 0x325, 0x177, 0x53b, 0x3db, 0x311, 0x48b,
/*  B7 B14 B25 B55 B113 -16 */ 0x147, 0x4f2,
/*  B8 B15 B26 B56 B114 -14 */ 0xf0, 0x2f3, 0x3a1,
/*  B9 B16 B27 B57 B115 -11 */ 0x12f, 0x4f2,
/*  B10 B17 B28 B58 B116 -9 */ 0x155, 0x53b, 0x409, 0x325, 0x177, 0x53b, 0x3e1, 0x311, 0x48b,
/*  B82 B103 -13 */ 0xf0, 0x2f3, 0x3a1,
/*  B83 B104 -10 */ 0x12f, 0x27b, 0x40f, 0x2f3, 0x271, 0x17f, 0x3ed, 0x2d5, 0x4f2, 0x48b,
/*  B80 B101 -12 */ 0x147, 0x325, 0x3ac,
/*  B81 B102 -9*/ 0x155, 0x53b, 0x409, 0x325, 0x177, 0x53b, 0x3e1, 0x311, 0x48b,
/*  B59 B64 B69 B71 B84 -7 */ 0x103, 0x2ad,
/*  B60 B65 B70 B72 B85 -5 */ 0xfc, 0x3d7, 0xc1, 0x1b8, 0x48b,
/*  B89-8 */ 0x103, 0x2ad,
/*  B90-6 */ 0xfc, 0x41b, 0x485, 0xc1, 0x1ca, 0x48b,
/*  B86-11 */ 0x15c, 0x307, 0x17f, 0x2e9,
/*  B87-7 */ 0x10a, 0x4f2,
/*  B88-5 */ 0x136, 0xa0, 0xc1, 0x1b8, 0x48b,
/*  B91-17 */ 0x15c, 0x307, 0x17f, 0x2e9,
/*  B92-13 */ 0x10a, 0x4f2,
/*  B93-11 */ 0x136, 0x285, 0x41b, 0x485, 0x311, 0x28f, 0x485, 0x325, 0xc1, 0x1dc, 0x48b,
/*  B61-16 */ 0x15c, 0x307, 0x17f, 0x2e9,
/*  B62-12 */ 0x10a, 0x4f2,
/*  B63-10 */ 0x136, 0x28f, 0x47f, 0x325, 0x285, 0x47f, 0x311, 0xc1, 0x1d3, 0x48b,
/*  B66-16 */ 0x15c, 0x307, 0x17f, 0x2e9,
/*  B67-12 */ 0x10a, 0x4f2,
/*  B68-10 */ 0x136, 0x285, 0x485, 0x311, 0x28f, 0x485, 0x325, 0xc1, 0x1d3, 0x48b,
/*  B45-13 */ 0xf7, 0x2fd, 0x2df,
/*  B46-10 */ 0x10a, 0x4f2,
/*  B47-8 */ 0x136, 0x28f, 0x8c, 0x325, 0x285, 0x8c, 0x311, 0x48b,
/*  B42-14 */ 0x13b, 0x513, 0x51d,
/*  B43-11 */ 0x15c, 0x4f2,
/*  B44-9 */ 0x141, 0x53b, 0x8c, 0x325, 0x177, 0x53b, 0x8c, 0x311, 0x48b,
/*  B39-13 */ 0xf7, 0x2fd, 0x2df,
/*  B40-10 */ 0x10a, 0x4f2,
/*  B41-8 */ 0x136, 0x4c9, 0x4d9, 0x325, 0x38b, 0x4a1, 0x311, 0x48b,
/*  B36-15 */ 0x13b, 0x2fd, 0x2df,
/*  B37-12 */ 0x141, 0x4f2,
/*  B38-10 */ 0x15c, 0x4c9, 0x4e1, 0x2f3, 0x38b, 0x17f, 0x4a9, 0x2d5, 0x4f2, 0x48b,
/*  B105-8 */ 0x5c0, 0x55f, 0x56a, 0xc1, 0x574, 0xb9, 0x57d, 0x48b,
/*  A108-2 */ 0x598, 0x42b,
/*  A38-1 */ 0x5a2,
/*  A109-1 */ 0x3b7,
/*  A110-1 */ 0x339,
/*  A111-2 */ 0xa0, 0xa8,
/*  A113-1 */ 0x5c3,
/*  A114-1 */ 0x32f,
/*  A115-1 */ 0x361,
/*  A116-1 */ 0x5cd,
/*  A117-1 */ 0x5d6,
/*  A118-2 */ 0x485, 0x5df
};

byte stackOrigin[] = "\xE@STACK$ORIGIN ";



static void EmitSource()
{
    word p[3], s, t;

    Fread(&tx1File, (pointer)p, 6);
    if (p[1] > 0 || p[2] == 0)
        s = p[0];
    else {
        s = p[2];
        p[2] = 0;
    }
    for (t = p[0]; t <= s; t++) {
        EmitLinePrefix();
        w812F = t;
        stmtCnt = p[1];
        blkCnt = p[2];
        GetSourceLine();
    }
}



static void AddrCheck(word arg1w)
{
    if (arg1w != baseAddr)
        FatalError(0xD8);
}

static void NewStatementNo()
{
    Fread(&tx1File, (pointer)&stmtNo, 2);
    if (stmtNo == 0)
        return;
    if (DEBUG) { 
        if (((rec_t *)rec8)->len + 4 >= 1020)
            WriteRec(rec8, 1);
        RecAddWord(rec8, 1, baseAddr);
        RecAddWord(rec8, 1, stmtNo);
    }
    if (codeOn)
        EmitStatementNo();
}




static void EmitLocalLabel()
{
    Fread(&tx1File, (pointer)&w96D7, 2);
    locLabStr[1] = '@';
    locLabStr[0] = Num2Asc(w96D7, 0, 10, &locLabStr[2]) + 1;
    AddrCheck(WordP(localLabelsP)[w96D7]);
    EmitLabel();
}

static void EmitSymLabel()
{
    Fread(&tx1File, (pointer)&curInfoP, 2);
    curInfoP = curInfoP + botInfo;
    curSymbolP = GetSymbol();
    locLabStr[0] = SymbolP(curSymbolP)->name[0];
    memmove(&locLabStr[1], &SymbolP(curSymbolP)->name[1], locLabStr[0]);
    AddrCheck(GetLinkVal());
    EmitLabel();
}

static void EmitSimpleError()
{
    Fread(&tx1File, (pointer)&errData, 2);
    errData.info = 0;
    errData.stmt = stmtNo;
    EmitError();
}


static void EmitNearError()
{
    Fread(&tx1File, (pointer)&errData, 4);
    errData.stmt = stmtNo;
    EmitError();
}

static void EmitFullError()
{
    Fread(&tx1File, (pointer)&errData, 6);
    EmitError();
}



static void SetNewAddr()
{
    Fread(&tx1File, (pointer)&curInfoP, 2);
    Fread(&tx1File, (pointer)&baseAddr, 2);
    curInfoP = curInfoP + botInfo;
    baseAddr += GetLinkVal();
    FlushRecs();
}

void MiscCntrl()
{
    byte name[19];

    switch (cfCode) {
    case T2_LIST: listOff = false; break;
    case T2_NOLIST: listOff = true; break;
    case T2_CODE: codeOn = PRINT; break;
    case T2_NOCODE: codeOn = false; break;
    case T2_EJECT:
            if (listing )
                NewPageNextChLst();
            break;
    case T2_INCLUDE:
            EmitLinePrefix();
            TellF(&srcFil, (loc_t *)&srcFileTable[srcFileIdx + 8]);
            Backup((loc_t *)&srcFileTable[srcFileIdx + 8], offLastCh - offCurCh);
            srcFileIdx = srcFileIdx + 10;
            Fread(&tx1File, &name[13], 6);      /* Read() in name of include file */
            Fread(&tx1File, &name[6], 7);
            Fread(&tx1File, &name[0], 7);       /* overwrites the type byte */
            memmove((pointer)&srcFileTable[srcFileIdx], name, 16);
            CloseF(&srcFil);
            InitF(&srcFil, "SOURCE", &name[1]);
            OpenF(&srcFil, 1);
            offCurCh = offLastCh;       /* force Read() next Time       */
            break;
    }
}

void Sub_54BA()
{
    Fread(&tx1File, &cfCode, 1);
    if (cfCode == T2_LINEINFO)
        EmitSource();
    else if (cfCode == 0x86)
        Fread(&tx1File, (pointer)&stmtNo, 2);
    else if (cfCode == T2_STMTCNT)
        NewStatementNo();
    else if (cfCode == T2_LOCALLABEL || cfCode == T2_CASELABEL)
        EmitLocalLabel();
    else if (cfCode == T2_LABELDEF)
        EmitSymLabel();
    else if (cfCode == 0xA4)
        SetNewAddr();
    else if (cfCode == T2_SYNTAXERROR)
        EmitSimpleError();
    else if (cfCode == T2_TOKENERROR)
        EmitNearError();
    else if (T2_LIST <= cfCode && cfCode <= T2_INCLUDE)
        MiscCntrl();
    else if (cfCode == T2_EOF)
        bo812B = false;
    else if (cfCode == T2_ERROR)
        EmitFullError();
    else
        Sub_668B();
}

