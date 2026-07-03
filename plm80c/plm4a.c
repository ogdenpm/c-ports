/****************************************************************************
 *  plm4a.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

/**
 * @file plm4a.c
 * @brief Pass-4 decode/emit tables and T2 record dispatcher for PL/M-80 C port.
 *
 * This module provides:
 * - Static lookup tables for fragment/opcode expansion.
 * - Encoded instruction text/object templates (`codeTable`, `codeSeq`, related maps).
 * - Runtime handlers that consume pass-2/3 stream records from `utf1` and emit:
 *   - listing lines,
 *   - object/fixup records,
 *   - diagnostics.
 *
 * High-level flow:
 * 1) Read one control/fragment byte (`cfCode`) from `utf1`.
 * 2) Dispatch special control records (line info, stmt no, labels, set addr, errors).
 * 3) For normal fragment codes, emit current fragment via template decode.
 */

/* ------------------------------------------------------------------------- */
/*  Table notes                                                              */
/* ------------------------------------------------------------------------- */

/**
 * Maps helper variant slots to helper IDs used by pass-4 helper call emission.
 * 
 * This table converts a helper index (0-45) to the actual helper ID used in
 * code generation. Helper functions implement complex operations that don't
 * map directly to single 8080 instructions.
 *
 * Examples:
 *   modHelperId[0] = 0x00  → First helper
 *   modHelperId[1] = 0x02  → Second helper
 *   modHelperId[45] = 0x75 → Last helper (PMO extension)
 */
uint8_t modHelperId[] = {
    0,    2,    4,    7,    0xB,  0xE,  0x12, 0x14, 0x16, 0x19, 0x1D, 0x1F, 0x22, 0x24, 0x27, 0x2A,
    0x2D, 0x30, 0x32, 0x34, 0x37, 0x3B, 0x3D, 0x40, 0x42, 0x45, 0x47, 0x49, 0x4B, 0x4D, 0x50, 0x52,
    0x54, 0x56, 0x59, 0x5B, 0x5E, 0x60, 0x62, 0x65, 0x67, 0x69, 0x6A, 0x6C, 0x6E, 0x71, /* PMO */ 0x75
};

// b4602
/**
 * Alias map used when `cfCode >= CF_174`.
 * Remaps extended fragment aliases to canonical fragment control codes.
 *
 * Purpose:
 *   - Extended fragment codes (174+) need to be mapped back to base codes
 *   - Allows more fragment variants without expanding core tables
 *   - Provides aliasing mechanism for similar operations
 *
 * Mapping ranges:
 *   - CF_174 to CF_184 (11 codes) → 0xA7
 *   - CF_185 to CF_195 (11 codes) → 0xA8
 *   - CF_196 to CF_206 (11 codes) → 0xA9
 *   - CF_207 to CF_217 (11 codes) → 0xAA
 *   - CF_218 to CF_228 (11 codes) → 0xAB
 *   - CF_229 to CF_254 (26 codes) → 0xA6
 */
uint8_t cfCodeAliasMap[] = { 
    0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, // 174-184 → 0xA7
    0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, // 185-195 → 0xA8
    0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, // 196-206 → 0xA9
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, // 207-217 → 0xAA
    0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, // 218-228 → 0xAB
    0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, // 229-239 → 0xA6
    0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, // 240-250 → 0xA6
    0xA6, 0xA6, 0xA6, 0xA6                                            // 251-254 → 0xA6
};

// clang-format off
/**
 * Mnemonic strings indexed by helper opcode variant index.
 * Used for listing text generation.
 */
char const *opcodes[] = { "SUB", "SBB", "RLC", "RRC", "RAL", "RAR", "SHL", "SHR",
                   "ADD", "ADC", "MUL", "DIV", "ANA", "ORA", "XRA", "NEG",
                   "NOT", "M10", "UCF", "CPI", "CMP", "SUI", "SBI", "ADI",
                   "ACI", "ANI", "ORI", "XRI", "INR", "INX", "DCR", "DCX",
                   "ADD\tA"};  

/* A B D H A C E L M */
/**
 * Register encoding lookup for operand insertion into opcode bits.
 * Order follows helper/template operand conventions.
 */
uint8_t regNo[]   = { 7, 0, 2, 4, 7, 1, 3, 5, 6 };
char const *regStr[]     = { "\x1" "A", "\x1" "B", "\x1" "D", "\x1H",
                             "\x1" "A", "\x1" "C", "\x1" "E", "\x1L",
                             "\x1" "M" };

uint8_t regPairNo[]  = { 3, 0, 1, 2 };
char const *regPairStr[] = { "\x3PSW",  "\x1" "B", "\x1" "D", "\x1H" };

/**
 * Base opcode-byte lookup indexed by helper opcode variant index.
 * Paired with `opcodes[]` for object+listing consistency.
 */
uint8_t helperOpcodeBaseByteMap[]      = { 0x90, 0x98, 7,    0xF,  0x17, 0x1F, 0, 0, 0x80, 0x88, 0,
                      0,    0xA0, 0xB0, 0xA8, 0,    0,    0, 0, 0xFE, 0xB8, 0xD6,
                      0xDE, 0xC6, 0xCE, 0xE6, 0xF6, 0xEE, 4, 3, 5,    0xB,  0x87 };

/**
 * Fragment -> sequence descriptor:
 *   low 12 bits: start index in `codeSeq`
 *   high 4 bits: sequence length
 */
uint16_t instructionSeq[] = {
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

/**
 * Helper fragment start offsets into `codeSeq`.
 */
uint16_t helperStart[] = { 0x134, 0x137, 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x15A, 0x15C, 0x15F, 0x161, 0x14A,
                 0x14D, 0x151, 0x15A, 0x15C, 0x15F, 0x161, 0x134, 0x137, 0x13F, 0x142, 0x14A, 0x14D,
                 0x151, 0x15A, 0x15C, 0x15F, 0x161, 0x11A, 0x11D, 0xFD,  0x101, 0x103, 0x10B, 0x10E,
                 0x1F6, 0x1F9, 0x1FB, 0x1E9, 0x1EC, 0x1EE, 0x1DB, 0x1DE, 0x1E0, 0x1CE, 0x1D1, 0x1D3,
                 0x134, 0x137, 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x15A, 0x15C, 0x15F, 0x161, 0x183,
                 0x185, 0x1AE, 0x1B2, 0x1B4, 0x183, 0x185, 0x1BE, 0x1C2, 0x1C4, 0x183, 0x185, 0x183,
                 0x185, 0x134, 0x137, 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x177, 0x17A, 0x16A, 0x16D,
                 0x183, 0x185, 0x192, 0x196, 0x198, 0x18A, 0x18C, 0x19D, 0x1A1, 0x1A3, 0x134, 0x137,
                 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x177, 0x17A, 0x16A, 0x16D, 0x205, 0x134, 0x137,
                 0x13F, 0x142, 0x14A, 0x14D, 0x151, 0x15A, 0x15C, 0x15F, 0x161 };

/**
 * Helper fragment lengths corresponding to `helperStart[]`.
 */
uint8_t helperLen[] = { 0xB,  8,    0xB, 8,    0x10, 0xD, 9,    0x10, 0xE, 0xB,  9,   0x10, 0xD,
                 9,    0x10, 0xE, 0xB,  9,    0xB, 8,    0xB,  8,   0x10, 0xD, 9,    0x10,
                 0xE,  0xB,  9,   0x1A, 0x17, 0xE, 0xA,  8,    0xF, 0xC,  0xF, 0xC,  0xA,
                 0xD,  0xA,  8,   0xE,  0xB,  9,   0xD,  0xA,  8,   0xB,  8,   0xB,  8,
                 0x10, 0xD,  9,   0x10, 0xE,  0xB, 9,    7,    5,   0x10, 0xC, 0xA,  7,
                 5,    0x10, 0xC, 0xA,  7,    5,   7,    5,    0xB, 8,    0xB, 8,    0x10,
                 0xD,  9,    0xC, 9,    0xD,  0xA, 7,    5,    0xB, 7,    5,   8,    6,
                 0x11, 0xD,  0xB, 0xB,  8,    0xB, 8,    0x10, 0xD, 9,    0xC, 9,    0xD,
                 0xA,  8,    0xB, 8,    0xB,  8,   0x10, 0xD,  9,   0x10, 0xE, 0xB,  9 };

// clang-format off
/**
 * Encoded template bytecode for instruction/listing emission.
 *
 * Format:
 * - first byte: base opcode (if non-zero)
 * - byte >= 0xC0: modifies opcode byte
 * - byte >  0x80: appends extra object bytes / operand directives
 * - byte == 0x80: end of template
 * - otherwise: literal listing text bytes
 */
/*
Purpose:
This array contains encoded template bytecode that defines how to generate both:
1.	Intel 8080 machine code (object bytes)
2.	Assembly listing text (for human-readable output)
How it works (per the comment on lines 142-150):
The codeTable is structured as a sequence of templates where each byte has special meaning:
•	First byte: Base opcode (if non-zero) - the actual Intel 8080 instruction opcode
•	Bytes >= 0xC0: Modifiers that change the opcode byte
•	Bytes > 0x80: Special directives for:
•	Appending extra object bytes
•	Operand handling instructions (e.g., \x84 means append operand byte)
•	Byte == 0x80: Template terminator
•	Other bytes: Literal text for listing output (e.g., "\tADC\tL")
Examples from the table:
•	Line 152: "\x8D" "\tADC\tL" "\x80" - Generates "ADC L" instruction (opcode 0x8D)
•	Line 153: "\xC6" "\tADI\t" "\x84\xFF\x80" - Generates "ADI" instruction with immediate operand
This is essentially a domain-specific bytecode interpreter that the compiler uses to emit both machine code and human-readable assembly listings from compact encoded templates.

*/
uint8_t codeTable[] = {
    /* 0000 */ "\x8D" "\tADC\tL" "\x80"
    // Template 0: ADC L instruction
    // Object: 8D (ADC L opcode)
    // Listing: "    ADC    L"
    // Single byte, no operands

    /* 0008 */ "\xC6" "\tADI\t" "\x84\xFF\x80"
    // Template 1: ADI (Add Immediate) with 0xFF mask
    // Object: C6 [immediate byte]
    // Listing: "    ADI    [value]"
    // 0x84 = append byte operand
    // 0xFF = operand byte to append (255)

    /* 0011 */ "\xE6" "\tANI\t" "\x84\x1\x80"
    // Template 2: ANI (AND Immediate) with 0x01 mask
    // Object: E6 01
    // Listing: "    ANI    01H"
    // Masks to bit 0

    /* 001A */ "\xE6" "\tANI\t" "\x84\x3\x80"
    // Template 3: ANI with 0x03 mask
    // Masks to bits 0-1

    /* 0023 */ "\xE6" "\tANI\t" "\x84\x7\x80"
    // Template 4: ANI with 0x07 mask
    // Masks to bits 0-2

    /* 002C */ "\xE6" "\tANI\t" "\x84\x80"
    // Template 5: ANI with variable operand
    // Object: E6 [operand byte]
    // Operand follows directive

    /* 0034 */ "\x80"
    // Template 6: Empty/NOP template
    // No object code, no listing
    // Used as placeholder

    /* 0035 */ "\xE6" "\tANI\t" "\x84\xc0\x80"
    // Template 7: ANI with 0xC0 mask
    // Masks to bits 6-7

    /* 003E */ "\xE6" "\tANI\t" "\x84\xE0\x80"
    // Template 8: ANI with 0xE0 mask
    // Masks to bits 5-7

    /* 0047 */ "\xE6" "\tANI\t" "\x84\xF8\x80"
    // Template 9: ANI with 0xF8 mask
    // Masks to bits 3-7

    /* 0050 */ "\xE6" "\tANI\t" "\x84\xFC\x80"
    // Template 10: ANI with 0xFC mask
    // Masks to bits 2-7

    /* 0059 */ "\xE6" "\tANI\t" "\x84\xFE\x80"
    // Template 11: ANI with 0xFE mask
    // Masks to bits 1-7

    /* 0062 */ "\xE6" "\tANI\t" "\x84\xF0\x80"
    // Template 12: ANI with 0xF0 mask
    // Masks to bits 4-7

    /* 006B */ "\xCD" "\tCALL\t" "\x82\x80"
    // Template 13: CALL instruction
    // Object: CD [low addr] [high addr]
    // Listing: "    CALL   [address]"
    // 0x82 = append 16-bit address operand
    // Used for direct procedure calls

    /* 0074 */ "\xCD" "\tCALL\t" "\x83 \x80"
    // Template 14: CALL with extended encoding
    // 0x83 = append special formatted byte

    /* 007E */ "\xCD" "\tCALL\t" "\x93     \x80"
    // Template 15: CALL with address label
    // 0x93 = append address label operand

    /* 008C */ "\x2F" "\tCMA" "\x80"
    // Template 16: CMA (Complement Accumulator)
    // Object: 2F
    // No operands

    /* 0092 */ "\x27" "\tDAA" "\x80"
    // Template 17: DAA (Decimal Adjust Accumulator)
    // Object: 27
    // No operands

    /* 0098 */ "\x9" "\tDAD\tB" "\x80"
    // Template 18: DAD B (Double Add Registers B)
    // Object: 09
    // No operands

    /* 00A0 */ "\x29" "\tDAD\tH" "\x80"
    // Template 19: DAD H (Double Add Registers H)
    // Object: 29
    // No operands

    /* 00A8 */ "\x9" "\tDAD\t" "\xC0\x80"
    // Template 20: DAD with variable operand
    // Object: 09 C0
    // Operand follows directive

    /* 00B0 */ "\x39" "\tDAD\tSP" "\x80"
    // Template 21: DAD SP (Double Add Stack Pointer)
    // Object: 39
    // No operands

    /* 00B9 */ "\x3D" "\tDCR\tA" "\x80"
    // Template 22: DCR A (Decrement Register A)
    // Object: 3D
    // No operands

    /* 00C1 */ "\xD" "\tDCR\tC" "\x80"
    // Template 23: DCR C (Decrement Register C)
    // Object: 0D
    // No operands

    /* 00C9 */ "\x1B" "\tDCX\tD" "\x80"
    // Template 24: DCX D (Decrement Register Pair DE)
    // Object: 1B
    // No operands

    /* 00D1 */ "\x2B" "\tDCX\tH" "\x80"
    // Template 25: DCX H (Decrement Register Pair HL)
    // Object: 2B
    // No operands

    /* 00D9 */ "\xF3" "\tDI" "\x80"
    // Template 26: DI (Disable Interrupts)
    // Object: F3
    // No operands

    /* 00DE */ "\0" "\tDW\t" "\x82\x80"
    // Template 27: DW (Define Word)
    // 0x82 = append word operand

    /* 00E5 */ "\xFB" "\tEI" "\x80"
    // Template 28: EI (Enable Interrupts)
    // Object: FB
    // No operands

    /* 00EA */ "\0\x89" "AH:" "\x80"
    // Template 29: Absolute label definition
    // Object: 89 41 48
    // "AH:" is the label

    /* 00F0 */ "\0\x89" "APH:" "\x80"
    // Template 30: Absolute label with procedure hint
    // Object: 89 50 48
    // "APH:" is the label

    /* 00F7 */ "\0\x89" "B:" "\x80"
    // Template 31: Absolute label for B register
    // Object: 89 42 42
    // "B:" is the label

    /* 00FC */ "\0\x89" "BBA:" "\x80"
    // Template 32: Absolute label for B register (byte access)
    // Object: 89 42 42 41
    // "BBA:" is the label

    /* 0103 */ "\0\x89" "BPH:" "\x80"
    // Template 33: Absolute label for B register (pointed)
    // Object: 89 42 50
    // "BPH:" is the label

    /* 010A */ "\0\x89" "D:" "\x80"
    // Template 34: Absolute label for D register
    // Object: 89 42 44
    // "D:" is the label

    /* 010F */ "\0\x89" "DA:" "\x80"
    // Template 35: Absolute label for D register (direct)
    // Object: 89 42 44 41
    // "DA:" is the label

    /* 0115 */ "\0\x89" "DB:" "\x80"
    // Template 36: Absolute label for B register (direct byte)
    // Object: 89 42 42 41
    // "DB:" is the label

    /* 011B */ "\0\x89" "DBP:" "\x80"
    // Template 37: Absolute label for B register (direct pointed)
    // Object: 89 42 42 50
    // "DBP:" is the label

    /* 0122 */ "\0\x89" "DH:" "\x80"
    // Template 38: Absolute label for H register
    // Object: 89 42 48
    // "DH:" is the label

    /* 0128 */ "\0\x89" "DHP:" "\x80"
    // Template 39: Absolute label for H register (pointed)
    // Object: 89 42 58
    // "DHP:" is the label

    /* 012F */ "\0\x89" "DPH:" "\x80"
    // Template 40: Absolute label for H register (direct)
    // Object: 89 42 50
    // "DPH:" is the label

    /* 0136 */ "\0\x89H:" "\x80"
    // Template 41: Absolute label for H register (H-L direct)
    // Object: 89 48 48
    // "H:" is the label

    /* 013B */ "\0\x89PB:" "\x80"
    // Template 42: Absolute label for PB register
    // Object: 89 50 42
    // "PB:" is the label

    /* 0141 */ "\0\x89PD:" "\x80"
    // Template 43: Absolute label for PD register
    // Object: 89 50 44
    // "PD:" is the label

    /* 0147 */ "\0\x89PDA:" "\x80"
    // Template 44: Absolute label for PDA register
    // Object: 89 50 44 41
    // "PDA:" is the label

    /* 014E */ "\0\x89PDB:" "\x80"
    // Template 45: Absolute label for PDB register
    // Object: 89 50 42 41
    // "PDB:" is the label

    /* 0155 */ "\0\x89PDH:" "\x80"
    // Template 46: Absolute label for PDH register
    // Object: 89 50 48
    // "PDH:" is the label

    /* 015C */ "\0\x89PH:" "\x80"
    // Template 47: Absolute label for PH register
    // Object: 89 50 48
    // "PH:" is the label

    /* 0162 */ "\x76" "\tHLT" "\x80"
    // Template 48: HLT (Halt)
    // Object: 76
    // No operands

    /* 0168 */ "\xDB" "\tIN\t" "\x86\x80"
    // Template 49: IN (Input from Port)
    // Object: DB [port]
    // 0x86 = append byte operand

    /* 016F */ "\x3C" "\tINR\tA" "\x80"
    // Template 50: INR A (Increment Register A)
    // Object: 3C
    // No operands

    /* 0177 */ "\x13" "\tINX\tD" "\x80"
    // Template 51: INX D (Increment Register Pair DE)
    // Object: 13
    // No operands

    /* 017F */ "\x23" "\tINX\tH" "\x80"
    // Template 52: INX H (Increment Register Pair HL)
    // Object: 23
    // No operands

    /* 0187 */ "\xC2" "\tJ\x88\t" "\x82\x80"
    // Template 53: Jumps with condition
    // Object: C2 [byte1] [byte2]
    // 0x82 = append word operand

    /* 018E */ "\xFA" "\tJM\t" "\x87\x4\x80"
    // Template 54: JM (Jump Minus)
    // Object: FA [addr]
    // 0x87 = append address operand

    /* 0196 */ "\xC3" "\tJMP\t" "\x82\x80"
    // Template 55: JMP (Jump)
    // Object: C3 [low addr] [high addr]
    // 0x82 = append 16-bit address operand

    /* 019E */ "\xD2" "\tJNC\t" "\x82\x80"
    // Template 56: JNC (Jump if No Carry)
    // Object: D2 [addr]
    // 0x82 = append word operand

    /* 01A6 */ "\xD2" "\tJNC\t" "\x87\x4\x80"
    // Template 57: JNC with extended encoding
    // 0x87 = append address operand

    /* 01AF */ "\xD2" "\tJNC\t" "\x87\x5\x80"
    // Template 58: JNC with extended encoding (variant)
    // 0x87 = append address operand

    /* 01B8 */ "\xC2" "\tJNZ\t" "\x87\xFE\x80"
    // Template 59: JNZ (Jump if Not Zero)
    // Object: C2 [addr]
    // 0x87 = append address operand

    /* 01C1 */ "\xC2" "\tJNZ\t" "\x87\xEC\x80"
    // Template 60: JNZ with extended encoding
    // 0x87 = append address operand

    /* 01CA */ "\xC2" "\tJNZ\t" "\x87\xFD\x80"
    // Template 61: JNZ with extended encoding (variant)
    // 0x87 = append address operand

    /* 01D3 */ "\xC2" "\tJNZ\t" "\x87\xF9\x80"
    // Template 62: JNZ with extended encoding (variant)
    // 0x87 = append address operand

    /* 01DC */ "\xC2" "\tJNZ\t" "\x87\xF8\x80"
    // Template 63: JNZ with extended encoding (variant)
    // 0x87 = append address operand

    /* 01E5 */ "\xC2" "\tJNZ\t" "\x87\xF7\x80"
    // Template 64: JNZ with extended encoding (variant)
    // 0x87 = append address operand

    /* 01EE */ "\xEA" "\tJPE\t" "\x87\x4\x80"
    // Template 65: JPE (Jump if Parity Even)
    // Object: EA [addr]
    // 0x87 = append address operand

    /* 01F7 */ "\xCA" "\tJZ\t" "\x87\x4\x80"
    // Template 66: JZ (Jump if Zero)
    // Object: CA [addr]
    // 0x87 = append address operand

    /* 01FF */ "\x3A" "\tLDA\t" "\x82\x80"
    // Template 67: LDA (Load Accumulator Direct)
    // Object: 3A [low byte] [high byte]
    // 0x82 = append word operand

    /* 0207 */ "\xA" "\tLDAX\t" "\xC0\x80"
    // Template 68: LDAX (Load Register Pair Direct)
    // Object: 0A C0
    // Operand follows directive

    /* 0210 */ "\x2A" "\tLHLD\t" "\x82\x80"
    // Template 69: LHLD (Load HL Direct)
    // Object: 2A [low addr] [high addr]
    // 0x82 = append 16-bit address operand

    /* 0219 */ "\x21" "\tLXI\tH," "\x82\t\x8A\x80"
    // Template 70: LXI (Load Register Pair Immediate)
    // Object: 21 [low addr] [high addr]
    // 0x82 = append 16-bit address operand

    /* 0225 */ "\x21" "\tLXI\tH," "\x87\xD\x80"
    // Template 71: LXI with extended encoding
    // 0x87 = append address operand

    /* 0230 */ "\x21" "\tLXI\tH," "\x87\x11\x80"
    // Template 72: LXI with extended encoding (variant)
    // 0x87 = append address operand

    /* 023B */ "\x21" "\tLXI\tH," "\x87\x8\x80"
    // Template 73: LXI with extended encoding (variant)
    // 0x87 = append address operand

    /* 0246 */ "\x21" "\tLXI\tH," "\x87\t" "\x80"
    // Template 74: LXI with byte-sized immediate
    // Object: 21 [low addr] [high addr] 87 09
    // Followed by immediate byte

    /* 0251 */ "\x21" "\tLXI\tH," "\x94\0\x80"
    // Template 75: LXI with word-sized immediate
    // Object: 21 [low addr] [high addr] 94 00
    // Followed by immediate word

    /* 025C */ "\x1" "\tLXI\t" "\xC0,\x92\x80"
    // Template 76: LXI with direct addressing
    // Object: 01 C0 92
    // "LXI H,ADDR" → load HL with address

    /* 0266 */ "\x31" "\tLXI\tSP," "\x85\x80"
    // Template 77: LXI SP (Load Stack Pointer Immediate)
    // Object: 31 [low byte] [high byte]
    // 0x85 = append word operand

    /* 0271 */ "\x7A" "\tMOV\tA,D" "\x80"
    // Template 78: MOV A,D (Move Data Reg D to A)
    // Object: 7A
    // No operands

    /* 027B */ "\x7B" "\tMOV\tA,E" "\x80"
    // Template 79: MOV A,E (Move Data Reg E to A)
    // Object: 7B
    // No operands

    /* 0285 */ "\x7C" "\tMOV\tA,H" "\x80"
    // Template 80: MOV A,H (Move Data Reg H to A)
    // Object: 7C
    // No operands

    /* 028F */ "\x7D" "\tMOV\tA,L" "\x80"
    // Template 81: MOV A,L (Move Data Reg L to A)
    // Object: 7D
    // No operands

    /* 0299 */ "\x78" "\tMOV\tA," "\xE0\x80"
    // Template 82: MOV A,Direct (Move Direct to A)
    // Object: 78 E0
    // Operand follows directive

    /* 02A3 */ "\x78" "\tMOV\tA," "\xE2\x80"
    // Template 83: MOV A,Direct (Move Direct to A, extended)
    // Object: 78 E2
    // Operand follows directive

    /* 02AD */ "\x7E" "\tMOV\tA,M" "\x80"
    // Template 84: MOV A,M (Move Memory to A)
    // Object: 7E
    // No operands

    /* 02B7 */ "\x44" "\tMOV\tB,H" "\x80"
    // Template 85: MOV B,H (Move Data Reg H to B)
    // Object: 44
    // No operands

    /* 02C1 */ "\x4F" "\tMOV\tC,A" "\x80"
    // Template 86: MOV C,A (Move Accumulator to C)
    // Object: 4F
    // No operands

    /* 02CB */ "\x4D" "\tMOV\tC,L" "\x80"
    // Template 87: MOV C,L (Move Data Reg L to C)
    // Object: 4D
    // No operands

    /* 02D5 */ "\x57" "\tMOV\tD,A" "\x80"
    // Template 88: MOV D,A (Move Accumulator to D)
    // Object: 57
    // No operands

    /* 02DF */ "\x50" "\tMOV\tD,B" "\x80"
    // Template 89: MOV D,B (Move Data Reg B to D)
    // Object: 50
    // No operands

    /* 02E9 */ "\x56" "\tMOV\tD,M" "\x80"
    // Template 90: MOV D,M (Move Memory to D)
    // Object: 56
    // No operands

    /* 02F3 */ "\x5F" "\tMOV\tE,A" "\x80"
    // Template 91: MOV E,A (Move Accumulator to E)
    // Object: 5F
    // No operands

    /* 02FD */ "\x59" "\tMOV\tE,C" "\x80"
    // Template 92: MOV E,C (Move Data Reg C to E)
    // Object: 59
    // No operands

    /* 0307 */ "\x5E" "\tMOV\tE,M" "\x80"
    // Template 93: MOV E,M (Move Memory to E)
    // Object: 5E
    // No operands

    /* 0311 */ "\x67" "\tMOV\tH,A" "\x80"
    // Template 94: MOV H,A (Move Accumulator to H)
    // Object: 67
    // No operands

    /* 031B */ "\x66" "\tMOV\tH,M" "\x80"
    // Template 95: MOV H,M (Move Memory to H)
    // Object: 66
    // No operands

    /* 0325 */ "\x6F" "\tMOV\tL,A" "\x80"
    // Template 96: MOV L,A (Move Accumulator to L)
    // Object: 6F
    // No operands

    /* 032F */ "\x46" "\tMOV\t" "\xD0,M\x80"
    // Template 97: MOV indirect (D0) ← Register
    // Object: 46 D0
    // Followed by register to move

    /* 0339 */ "\x40" "\tMOV\t" "\xD2,\xe0\x80"
    // Template 98: MOV indirect (D2) ← Immediate
    // Object: 40 D2
    // Followed by immediate byte

    /* 0343 */ "\x46" "\tMOV\t" "\xD2,M\x80"
    // Template 99: MOV indirect (D2) ← Register
    // Object: 46 D2
    // Followed by register to move

    /* 034D */ "\x40" "\tMOV\t" "\xD0,\xe1\x80"
    // Template 100: MOV indirect (D0) ← Immediate (extended)
    // Object: 40 D0
    // Followed by immediate word

    /* 0357 */ "\x40" "\tMOV\t" "\xD2,\xe3\x80"
    // Template 101: MOV indirect (D2) ← Immediate (extended)
    // Object: 40 D2
    // Followed by immediate word

    /* 0361 */ "\x70" "\tMOV\tM," "\xE0\x80"
    // Template 102: MOV Memory ← Accumulator
    // Object: 70 E0
    // Followed by effective address

    /* 036B */ "\x70" "\tMOV\tM," "\xE2\x80"
    // Template 103: MOV Memory ← Accumulator (extended)
    // Object: 70 E2
    // Followed by effective address

    /* 0375 */ "\x3E" "\tMVI\tA," "\x84\x10\x80"
    // Template 104: MVI A,Immediate (Move Immediate to A)
    // Object: 3E 10
    // Followed by immediate byte

    /* 0380 */ "\x3E" "\tMVI\tA," "\x84\xFF\x80"
    // Template 105: MVI A,Immediate (Move Immediate to A, 255)
    // Object: 3E FF
    // Followed by immediate byte

    /* 038B */ "\x3E" "\tMVI\tA," "\x84\0\x80"
    // Template 106: MVI A,Immediate (Move Immediate to A, 0)
    // Object: 3E 00
    // Followed by immediate byte

    /* 0396 */ "\x6" "\tMVI\tB," "\x84\0\x80"
    // Template 107: MVI B,Immediate (Move Immediate to B)
    // Object: 06 00
    // Followed by immediate byte

    /* 03A1 */ "\x16" "\tMVI\tD," "\x84\0\x80"
    // Template 108: MVI D,Immediate (Move Immediate to D)
    // Object: 16 00
    // Followed by immediate byte

    /* 03AC */ "\x26" "\tMVI\tH," "\x84\0\x80"
    // Template 109: MVI H,Immediate (Move Immediate to H)
    // Object: 26 00
    // Followed by immediate byte

    /* 03B7 */ "\x6" "\tMVI\t" "\xD2,\x96\x80"
    // Template 110: MVI indirect (D2) ← Immediate
    // Object: 06 D2
    // Followed by immediate byte

    /* 03C1 */ "\x6" "\tMVI\t" "\xD0,\x84\0\x80"
    // Template 111: MVI indirect (D0) ← Immediate (with byte)
    // Object: 06 D0
    // Followed by immediate byte

    /* 03CC */ "\x36" "\tMVI\tM," "\x84\0\x80"
    // Template 112: MVI Memory ← Immediate
    // Object: 36 00
    // Followed by immediate byte

    /* 03D7 */ "\0" "\t" "\x9B\x80"
    // Template 113: Align/template padding
    // Ignored by code generator

    /* 03DB */ "\0" "\t" "\x9B\t\xE5\x80"
    // Template 114: Align/template padding with E5
    // Ignored by code generator

    /* 03E1 */ "\0" "\t" "\x9B\t\xE7\x80"
    // Template 115: Align/template padding with E7
    // Ignored by code generator

    /* 03E7 */ "\0" "\t" "\x9B\t\xC0\x80"
    // Template 116: Align/template padding with C0
    // Ignored by code generator

    /* 03ED */ "\0" "\t" "\x9B\t\xEC\x80"
    // Template 117: Align/template padding with EC
    // Ignored by code generator

    /* 03F3 */ "\0" "\t" "\xAB\x80"
    // Template 118: Conditional byte/word addition
    // Object: AB
    // No operands

    /* 03F7 */ "\0" "\t" "\xAB\t\x86\x80"
    // Template 119: Conditional byte/word addition (extended)
    // Object: AB 86
    // Followed by condition byte

    /* 03FD */ "\0" "\t" "\xBB\t\xEC\x80"
    // Template 120: Conditional byte/word addition (different condition)
    // Object: BB EC
    // Followed by condition byte

    /* 0403 */ "\0" "\t" "\x8B\t\xE9\x80"
    // Template 121: MOV PSW ← Immediate
    // Object: 8B E9
    // Followed by immediate byte

    /* 0409 */ "\0" "\t" "\x8B\t\xEB\x80"
    // Template 122: MOV PSW ← Immediate (E)
    // Object: 8B EB
    // Followed by immediate byte

    /* 040F */ "\0" "\t" "\x8B\t\xEC\x80"
    // Template 123: MOV PSW ← Immediate (C)
    // Object: 8B EC
    // Followed by immediate byte

    /* 0415 */ "\0" "\t" "\x8B\t\xDC\x80"
    // Template 124: MOV PSW ← Immediate (D)
    // Object: 8B DC
    // Followed by immediate byte

    /* 041B */ "\xB7" "\tORA\tA" "\x80"
    // Template 125: ORA A (Logical OR with A)
    // Object: B7
    // No operands

    /* 0423 */ "\xB5" "\tORA\tL" "\x80"
    // Template 126: ORA L (Logical OR with L)
    // Object: B5
    // No operands

    /* 042B */ "\xD3" "\tOUT\t" "\x86\x80"
    // Template 127: OUT (Output to Port)
    // Object: D3 [port]
    // Followed by port number byte

    /* 0433 */ "\xE9" "\tPCHL" "\x80"
    // Template 128: PCHL (Push PC to HL)
    // Object: E9
    // No operands

    /* 043A */ "\0; PROC" "\t" "\x81\x80"
    // Template 129: PROC directive
    // Object: 81
    // Followed by procedure initial address

    /* 0444 */ "\xC1" "\tPOP\tB" "\x80"
    // Template 130: POP B (Pop Register Pair DE)
    // Object: C1
    // No operands

    /* 044C */ "\xC1" "\tPOP\t" "\xC0\t\x8A\x80"
    // Template 131: POP with extended encoding
    // 0xC0 = registers affected

    /* 0456 */ "\xF1" "\tPOP\tPSW" "\x80"
    // Template 132: POP PSW (Pop Program Status Word)
    // Object: F1
    // No operands

    /* 0460 */ "\xE5" "\tPUSH\tH" "\x80"
    // Template 133: PUSH H (Push Register Pair HL)
    // Object: E5
    // No operands

    /* 0469 */ "\xC5" "\tPUSH\t" "\xC0\t\x8A\x80"
    // Template 134: PUSH with extended encoding
    // 0xC0 = registers affected

    /* 0474 */ "\xF5" "\tPUSH\tPSW" "\x80"
    // Template 135: PUSH PSW (Push Program Status Word)
    // Object: F5
    // No operands

    /* 047F */ "\x17" "\tRAL" "\x80"
    // Template 136: RAL (Rotate A Left)
    // Object: 17
    // No operands

    /* 0485 */ "\x1F" "\tRAR" "\x80"
    // Template 137: RAR (Rotate A Right)
    // Object: 1F
    // No operands

    /* 048B */ "\xC9" "\tRET" "\x80"
    // Template 138: RET (Return from Subroutine)
    // Object: C9
    // No operands

    /* 0491 */ "\x9F" "\tSBB\tA" "\x80"
    // Template 139: SBB A (Subtract with Borrow from A)
    // Object: 9F
    // No operands

    /* 0499 */ "\x98" "\tSBB\tB" "\x80"
    // Template 140: SBB B (Subtract with Borrow from B)
    // Object: 98
    // No operands

    /* 04A1 */ "\x9C" "\tSBB\tH" "\x80"
    // Template 141: SBB H (Subtract with Borrow from H)
    // Object: 9C
    // No operands

    /* 04A9 */ "\x9E" "\tSBB\tM" "\x80"
    // Template 142: SBB M (Subtract with Borrow from Memory)
    // Object: 9E
    // No operands

    /* 04B1 */ "\x22" "\tSHLD\t" "\x82\x80"
    // Template 143: SHLD (Store HL Direct)
    // Object: 22 [addr]
    // 0x82 = append word operand

    /* 04BA */ "\xF9" "\tSPHL" "\x80"
    // Template 144: SPHL (Set SP = HL)
    // Object: F9
    // No operands

    /* 04C1 */ "\x32" "\tSTA\t" "\x82\x80"
    // Template 145: STA (Store Accumulator Direct)
    // Object: 32 [low addr] [high addr]
    // 0x82 = append word operand

    /* 04C9 */ "\x97" "\tSUB\tA" "\x80"
    // Template 146: SUB A (Subtract A from A)
    // Object: 97
    // No operands

    /* 04D1 */ "\x91" "\tSUB\tC" "\x80"
    // Template 147: SUB C (Subtract C from A)
    // Object: 91
    // No operands

    /* 04D9 */ "\x95" "\tSUB\tL" "\x80"
    // Template 148: SUB L (Subtract L from A)
    // Object: 95
    // No operands

    /* 04E1 */ "\x96" "\tSUB\tM" "\x80"
    // Template 149: SUB M (Subtract Memory from A)
    // Object: 96
    // No operands

    /* 04E9 */ "\xD6" "\tSUI\t" "\x84\x1\x80"
    // Template 150: SUI (Subtract Immediate with Borrow)
    // Object: D6 01
    // Follows with immediate byte

    /* 04F2 */ "\xEB" "\tXCHG" "\x80"
    // Template 151: XCHG (Exchange DE with HL)
    // Object: EB
    // No operands

    /* 04F9 */ "\xE3" "\tXTHL\t" "\t\x8A\x80"
    // Template 152: XTHL (Exchange SP with HL)
    // Object: E3
    // No operands

    /* 0503 */ "\x3" "\tINX\t" "\xC0\x80"
    // Template 153: INX (Increment Register Pair)
    // Object: 03
    // No operands

    /* 050B */ "\xB" "\tDCX\t" "\xC0\x80"
    // Template 154: DCX (Decrement Register Pair)
    // Object: 0B
    // No operands

    /* 0513 */ "\x69" "\tMOV\tL,C" "\x80"
    // Template 155: MOV L,C (Move Data Reg C to L)
    // Object: 69
    // No operands

    /* 051D */ "\x60" "\tMOV\tH,B" "\x80"
    // Template 156: MOV H,B (Move Data Reg B to H)
    // Object: 60
    // No operands

    /* 0527 */ "\x4E" "\tMOV\tC,M" "\x80"
    // Template 157: MOV C,M (Move Memory to C)
    // Object: 4E
    // No operands

    /* 0531 */ "\x46" "\tMOV\tB,M" "\x80"
    // Template 158: MOV B,M (Move Memory to B)
    // Object: 46
    // No operands

    /* 053B */ "\x1A" "\tLDAX\tD" "\x80"
    // Template 159: LDAX D (Load Accumulator from DE)
    // Object: 1A
    // No operands

    /* 0544 */ "\xA" "\tLDAX\tB" "\x80"
    // Template 160: LDAX B (Load Accumulator from BC)
    // Object: 0A
    // No operands

    /* 054D */ "\x12" "\tSTAX\tD" "\x80"
    // Template 161: STAX D (Store A to DE)
    // Object: 12
    // No operands

    /* 0556 */ "\xC2" "\tJNZ\t" "\x87\xFB\x80"
    // Template 162: JNZ (Jump if Not Zero) with offset
    // Object: C2 [offset]
    // 0x87 = append byte offset

    /* 055F */ "\x6" "\tMVI\tB," "\x84\xC\x80"
    // Template 163: MVI B,Immediate (Move Immediate to B)
    // Object: 06 C
    // Followed by immediate byte

    /* 056A */ "\x48" "\tMOV\tC,B" "\x80"
    // Template 164: MOV C,B (Move Data Reg B to C)
    // Object: 48
    // No operands

    /* 0574 */ "\xC2" "\tJNZ\t" "\x87\xFF\x80"
    // Template 165: JNZ with extended address
    // Object: C2
    // Followed by address bytes

    /* 057D */ "\xC2" "\tJNZ\t" "\x87\xFA\x80"
    // Template 166: JNZ with extended address (variant)
    // Object: C2
    // Followed by address bytes

    /* 0586 */ "\x21" "\tLXI\tH," "\x92\x80"
    // Template 167: LXI (Load Register Pair Immediate)
    // Object: 21 92
    // Followed by address bytes

    /* 0590 */ "\x3" "\tINX\tB" "\x80"
    // Template 168: INX (Increment Register Pair BC)
    // Object: 03
    // No operands

    /* 0598 */ "\x78" "\tMOV\tA," "\xE3\x80"
    // Template 169: MOV A,Direct (Move Direct to A)
    // Object: 78 E3
    // Followed by effective address

    /* 05A2 */ "\x36" "\tMVI\tM," "\x86\x80"
    // Template 170: MVI Memory ← Immediate
    // Object: 36 86
    // Followed by immediate byte

    /* 05AC */ "\0" "\t" "\x8B\t\xD2\x80"
    // Template 171: Align/template padding
    // Ignored by code generator

    /* 05B2 */ "\0" "\t" "\xBB\t\xE2\x80"
    // Template 172: Align/template padding with E2
    // Ignored by code generator

    /* 05B8 */ "\x2D" "\tDCR\tL" "\x80"
    // Template 173: DCR L (Decrement Register L)
    // Object: 2D
    // No operands

    /* 05C0 */ "\0" "\x89\x80"
    // Template 174: Conditional byte/word addition
    // Object: 89
    // Followed by condition byte

    /* 05C3 */ "\x40" "\tMOV\t" "\xD0,\xe2\x80"
    // Template 175: MOV indirect (D0) ← Immediate (with byte)
    // Object: 40 D0
    // Followed by immediate byte

    /* 05CD */ "\x33" "\tINX\tSP" "\x80"
    // Template 176: INX SP (Increment Stack Pointer)
    // Object: 33
    // No operands

    /* 05D6 */ "\x3B" "\tDCX\tSP" "\x80"
    // Template 177: DCX SP (Decrement Stack Pointer)
    // Object: 0B
    // No operands

    /* 05DF */ "\xDA" "\tJC\t" "\x82\x80"
    // Template 178: JC (Jump if Carry)
    // Object: DA [byte1] [byte2]
    // 0x82 = append word operand

    /* 05E6 */ "\x2C" "\tINR\tL" "\x80"
    // Template 179: INR L (Increment Register L)
    // Object: 2C
    // No operands

    /* 05EE */ "\xC3" "\tJMP\t" "\x87\x7\x80"
    // Template 180: JMP (Jump Unconditionally) with offset
    // Object: C3 07
    // Followed by address bytes
};

// clang-format on
/**
 * Flattened sequence program interpreted by pass-4 fragment emitter.
 * Entries index into `codeTable`.
 */
uint16_t codeSeq[] = {
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
/**
 * P-string used for stack-origin symbol emission.
 */
uint8_t stackOrigin[] = "\xE@STACK$ORIGIN ";

/* ------------------------------------------------------------------------- */
/*  Runtime handlers                                                         */
/* ------------------------------------------------------------------------- */

/**
 * Emit source line(s) referenced by a T2 line-info record.
 *
 * Record payload:
 * - start line
 * - stmt count
 * - blk count or end line (legacy compact encoding)
 */
static void EmitSource() {
    uint16_t endLine;

    uint16_t line = Rd1Word();
    uint16_t stmt = Rd1Word();
    uint16_t blk  = Rd1Word();
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

/**
 * Validates that a label/symbol address matches current `baseAddr`.
 * Fatal on mismatch (internal consistency guard).
 */
static void AddrCheck(uint16_t arg1w) {
    if (arg1w != baseAddr)
        fatal_ov46(0xD8);
}

/**
 * Handles statement-count record and emits statement marker in listing/debug.
 */
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

/**
 * Emit local/case label definition after address consistency check.
 */
static void EmitLocalLabel() {
    locLabelNum = Rd1Word();
    AddrCheck(localLabels[locLabelNum]);
    sprintf(locPStr, "@%d", locLabelNum);
    EmitLabel(locPStr);
}

/**
 * Emit symbol label definition after address consistency check.
 */
static void EmitSymLabel() {
    info = FromIdx(Rd1Word());
    curSym = info->sym;
    AddrCheck(info->linkVal);
    EmitLabel(curSym->name->str);
}

/**
 * Emit syntax error with current statement context.
 */
static void EmitSimpleError() {
    errData.num  = Rd1Word();
    errData.info = 0;
    errData.stmt = stmtNo;
    EmitError();
}

/**
 * Emit token/near error with current statement context.
 */
static void EmitNearError() {
    errData.num  = Rd1Word();
    errData.info = Rd1Word();
    errData.stmt = stmtNo;
    EmitError();
}

/**
 * Emit fully specified error (explicit stmt context in record payload).
 */
static void EmitFullError() {
    errData.num  = Rd1Word();
    errData.info = Rd1Word();
    errData.stmt = Rd1Word();
    EmitError();
}

/**
 * Set new section-relative base address using symbol link base + offset.
 * Flushes pending records because relocation context changed.
 */
static void SetNewAddr() {
    info = FromIdx(Rd1Word());
    baseAddr = Rd1Word();
    baseAddr += info->linkVal;
    FlushRecs();
}

/**
 * Process one pass-4 input record from `utf1`.
 *
 * Dispatch table (by `cfCode`):
 * - T2_LINEINFO   -> EmitSource
 * - T2_SETSTMTNO  -> direct stmt number set
 * - T2_STMTCNT    -> NewStatementNo
 * - T2_LOCALLABEL/T2_CASELABEL -> EmitLocalLabel
 * - T2_LABELDEF   -> EmitSymLabel
 * - T2_SETADDR    -> SetNewAddr
 * - T2_SYNTAXERROR-> EmitSimpleError
 * - T2_TOKENERROR -> EmitNearError
 * - T2_LIST..T2_INCLUDE -> MiscControl
 * - T2_EOF        -> stop pass-4 loop
 * - T2_ERROR      -> EmitFullError
 * - otherwise     -> EmitCurrentFragment
 */
void ProcessPass4Record() {
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
        EmitCurrentFragment();
}
