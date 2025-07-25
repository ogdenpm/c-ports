/****************************************************************************
 *  nkeym.c: part of the C port of Intel's ISIS-II asm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/* base ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m 20 -7 -t in  */
/* Computed positions: -k'1-3' */
#include "asm80.h"
#include "plm80types.h"

struct keyword {
    char *name;
    byte base;
    byte type;
    byte flags;
};

#define TOTAL_KEYWORDS  134
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 6
#define MIN_HASH_VALUE  3
#define MAX_HASH_VALUE  204
/* maximum key range = 202, duplicates = 0 */

static unsigned int hash(register const char *str, register size_t len) {
    // this table could be reduced by using isupper on the str characters
    // and subtracting 'A' from the char. This would then only require
    // the values from 0ffset 0x40 to 0x5C
    static unsigned char asso_values[] = {
        205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205,
        205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205,
        205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205,
        205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 22,  13,  2,
        11,  5,   113, 55,  50,  33,  34,  73,  46,  64,  43,  28,  9,   69,  1,   55,  30,
        98,  24,  60,  42,  6,   122, 205, 14,  205, 205, 205, 205, 205, 205, 205, 205, 205,
        205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205,
        205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205
    };
    register unsigned hval = (unsigned)len;

    switch (hval) {
    default:
        hval += asso_values[(unsigned char)str[2]];
    /*FALLTHROUGH*/
    case 2:
        hval += asso_values[(unsigned char)str[1] + 2];
    /*FALLTHROUGH*/
    case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
    return hval;
}

tokensym_t *in_word_set(register const char *str) {
    // this table could be reduced by replacing with a mapping table that had
    // indexes into the wordlist. The wordlist table would then only be 134 entries
     static tokensym_t wordlist[] = { { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "C", { .base = 1 }, K_REGNAME, { 0 } },
                                     { "RP", { .base = 0xF0 }, SINGLE, { 0 } },
                                     { "CP", { .base = 0xF4 }, IMM16, { 0 } },
                                     { "E", { .base = 3 }, K_REGNAME, { 0 } },
                                     { "RAR", { .base = 0x1F }, SINGLE, { 0 } },
                                     { "RC", { .base = 0xD8 }, SINGLE, { 0 } },
                                     { "CC", { .base = 0xDC }, IMM16, { 0 } },
                                     { "RPE", { .base = 0xE8 }, SINGLE, { 0 } },
                                     { "CPE", { .base = 0xEC }, IMM16, { 0 } },
                                     { "D", { .base = 2 }, K_REGNAME, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "B", { .base = 0 }, K_REGNAME, { 0 } },
                                     { "RNC", { .base = 0xD0 }, SINGLE, { 0 } },
                                     { "CNC", { .base = 0xD4 }, IMM16, { 0 } },
                                     { "RZ", { .base = 0xC8 }, SINGLE, { 0 } },
                                     { "CZ", { .base = 0xCC }, IMM16, { 0 } },
                                     { "DW", { .base = 0 }, DW, { 0 } },
                                     { "DCR", { .base = 5 }, INRDCR, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "A", { .base = 7 }, K_REGNAME, { 0 } },
                                     { "DB", { .base = 0 }, DB, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "DAD", { .base = 9 }, REG16, { 0 } },
                                     { "END", { .base = 0 }, END, { 0 } },
                                     { "ENDM", { .base = 0 }, ENDM, { 0 } },
                                     { "ENDIF", { .base = 0 }, ENDIF, { 0 } },
                                     { "RM", { .base = 0xF8 }, SINGLE, { 0 } },
                                     { "CM", { .base = 0xFC }, IMM16, { 0 } },
                                     { "RPO", { .base = 0xE0 }, SINGLE, { 0 } },
                                     { "CPO", { .base = 0xE4 }, IMM16, { 0 } },
                                     { "CMC", { .base = 0x3F }, SINGLE, { 0 } },
                                     { "RRC", { .base = 0xF }, SINGLE, { 0 } },
                                     { "JP", { .base = 0xF2 }, IMM16, { 0 } },
                                     { "DAA", { .base = 0x27 }, SINGLE, { 0 } },
                                     { "CPI", { .base = 0xFE }, IMM8, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "JC", { .base = 0xDA }, IMM16, { 0 } },
                                     { "CMP", { .base = 0xB8 }, ARITH, { 0 } },
                                     { "JPE", { .base = 0xEA }, IMM16, { 0 } },
                                     { "IN", { .base = 0xDB }, IMM8, { 0 } },
                                     { "AND", { .base = 0 }, AND, { 0 } },
                                     { "INR", { .base = 4 }, INRDCR, { 0 } },
                                     { "L", { .base = 5 }, K_REGNAME, { 0 } },
                                     { "JNC", { .base = 0xD2 }, IMM16, { 0 } },
                                     { "RLC", { .base = 7 }, SINGLE, { 0 } },
                                     { "JZ", { .base = 0xCA }, IMM16, { 0 } },
                                     { "H", { .base = 4 }, K_REGNAME, { 0 } },
                                     { "RAL", { .base = 0x17 }, SINGLE, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "CALL", { .base = 0xCD }, IMM16, { 0 } },
                                     { "CMA", { .base = 0x2F }, SINGLE, { 0 } },
                                     { "ANA", { .base = 0xA0 }, ARITH, { 0 } },
                                     { "INPAGE", { .base = 1 }, K_SPECIAL, { 0 } },
                                     { "SP", { .base = 6 }, K_SP, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "OR", { .base = 0 }, OR, { 0 } },
                                     { "DCX", { .base = 0xB }, REG16, { 0 } },
                                     { "EQ", { .base = 0 }, EQ, { 0 } },
                                     { "ACI", { .base = 0xCE }, IMM8, { 0 } },
                                     { "JM", { .base = 0xFA }, IMM16, { 0 } },
                                     { "M", { .base = 6 }, K_REGNAME, { 0 } },
                                     { "JPO", { .base = 0xE2 }, IMM16, { 0 } },
                                     { "ANI", { .base = 0xE6 }, IMM8, { 0 } },
                                     { "PCHL", { .base = 0xE9 }, SINGLE, { 0 } },
                                     { "REPT", { .base = 0 }, REPT, { 0 } },
                                     { "PAGE", { .base = 2 }, K_SPECIAL, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "LT", { .base = 0 }, LT, { 0 } },
                                     { "MACRO", { .base = 0 }, MACRO, { 0 } },
                                     { "JMP", { .base = 0xC3 }, IMM16, { 0 } },
                                     { "IRP", { .base = 0 }, IRP, { 0 } },
                                     { "IRPC", { .base = 0 }, IRPC, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "EI", { .base = 0xFB }, SINGLE, { 0 } },
                                     { "GT", { .base = 0 }, GT, { 0 } },
                                     { "SBB", { .base = 0x98 }, ARITH, { 0 } },
                                     { "ORA", { .base = 0xB0 }, ARITH, { 0 } },
                                     { "STC", { .base = 0x37 }, SINGLE, { 0 } },
                                     { "IF", { .base = 0 }, IF, { 0 } },
                                     { "DI", { .base = 0xF3 }, SINGLE, { 0 } },
                                     { "INX", { .base = 3 }, REG16, { 0 } },
                                     { "PUBLIC", { .base = 0 }, PUBLIC, { 0 } },
                                     { "RET", { .base = 0xC9 }, SINGLE, { 0 } },
                                     { "POP", { .base = 0xC1 }, REG16, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "SHR", { .base = 0 }, SHR, { 0 } },
                                     { "ORI", { .base = 0xF6 }, IMM8, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "XRA", { .base = 0xA8 }, ARITH, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "NE", { .base = 0 }, NE, { 0 } },
                                     { "XCHG", { .base = 0xEB }, SINGLE, { 0 } },
                                     { "SBI", { .base = 0xDE }, IMM8, { 0 } },
                                     { "LE", { .base = 0 }, LE, { 0 } },
                                     { "STA", { .base = 0x32 }, IMM16, { 0 } },
                                     { "STAX", { .base = 2 }, LDSTAX, { 0 } },
                                     { "STACK", { .base = 0 }, K_SPECIAL, { 0x1B } },
                                     { "ELSE", { .base = 0 }, ELSE, { 0 } },
                                     { "XRI", { .base = 0xEE }, IMM8, { 0 } },
                                     { "CSEG", { .base = 0 }, CSEG, { 0 } },
                                     { "SPHL", { .base = 0xF9 }, SINGLE, { 0 } },
                                     { "DS", { .base = 0 }, DS, { 0 } },
                                     { "GE", { .base = 0 }, GE, { 0 } },
                                     { "NAME", { .base = 0 }, NAME, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "XOR", { .base = 0 }, XOR, { 0 } },
                                     { "ORG", { .base = 0 }, ORG, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "DSEG", { .base = 0 }, DSEG, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "XTHL", { .base = 0xE3 }, SINGLE, { 0 } },
                                     { "OUT", { .base = 0xD3 }, IMM8, { 0 } },
                                     { "LOCAL", { .base = 0 }, LOCAL, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "NOP", { .base = 0 }, SINGLE, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "HLT", { .base = 0x76 }, SINGLE, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "PUSH", { .base = 0xC5 }, REG16, { 0 } },
                                     { "ASEG", { .base = 0 }, ASEG, { 0 } },
                                     { "LHLD", { .base = 0x2A }, IMM16, { 0 } },
                                     { "SUB", { .base = 0x90 }, ARITH, { 0 } },
                                     { "RST", { .base = 0xC7 }, RST, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "RNZ", { .base = 0xC0 }, SINGLE, { 0 } },
                                     { "CNZ", { .base = 0xC4 }, IMM16, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "SHL", { .base = 0 }, SHL, { 0 } },
                                     { "SHLD", { .base = 0x22 }, IMM16, { 0 } },
                                     { "ADC", { .base = 0x88 }, ARITH, { 0 } },
                                     { "RIM", { .base = 0x20 }, SINGLE, { 2 } },
                                     { "MVI", { .base = 6 }, MVI, { 0 } },
                                     { "SET", { .base = 0 }, SET, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "NOT", { .base = 0 }, NOT, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "MOD", { .base = 0 }, MOD, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "ADD", { .base = 0x80 }, ARITH, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "SUI", { .base = 0xD6 }, IMM8, { 0 } },
                                     { "NUL", { .base = 0 }, NUL, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "STKLN", { .base = 0 }, STKLN, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "MOV", { .base = 0x40 }, MOV, { 0 } },
                                     { "EQU", { .base = 0 }, EQU, { 0 } },
                                     { "EXTRN", { .base = 0 }, EXTRN, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "EXITM", { .base = 0 }, EXITM, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "JNZ", { .base = 0xC2 }, IMM16, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "PSW", { .base = 6 }, K_REGNAME, { 0 } },
                                     { "ADI", { .base = 0xC6 }, IMM8, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "LOW", { .base = 0 }, LOW, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "HIGH", { .base = 0 }, HIGH, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "LDA", { .base = 0x3A }, IMM16, { 0 } },
                                     { "LDAX", { .base = 0xA }, LDSTAX, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "MEMORY", { .base = 0 }, K_SPECIAL, { 0x1C } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "SIM", { .base = 0x30 }, SINGLE, { 2 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "", { 0 }, 0, { 0 } },
                                     { "LXI", { .base = 1 }, LXI, { 0 } } };
    size_t len                   = strlen(str);
    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
        register unsigned int key = hash(str, len);

        if (key <= MAX_HASH_VALUE) {
            register const char *s = wordlist[key].name;

            if (*str == *s && !strcmp(str + 1, s + 1))
                return &wordlist[key];
        }
    }
    return 0;
}