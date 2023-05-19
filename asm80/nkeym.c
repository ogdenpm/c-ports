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

tokensym_t *in_word_set(register const char *str, register size_t len) {
    // this table could be reduced by replacing with a mapping table that had
    // indexes into the wordlist. The wordlist table would then only be 134 entries
    static tokensym_t wordlist[] = { { "" },
                                     { "" },
                                     { "" },
                                     { "C", { .base = 1 }, K_REGNAME, { 0 } },
                                     { "RP", { .base = 0xF0 }, K_SINGLE, { 0 } },
                                     { "CP", { .base = 0xF4 }, K_IMM16, { 0 } },
                                     { "E", { .base = 3 }, K_REGNAME, { 0 } },
                                     { "RAR", { .base = 0x1F }, K_SINGLE, { 0 } },
                                     { "RC", { .base = 0xD8 }, K_SINGLE, { 0 } },
                                     { "CC", { .base = 0xDC }, K_IMM16, { 0 } },
                                     { "RPE", { .base = 0xE8 }, K_SINGLE, { 0 } },
                                     { "CPE", { .base = 0xEC }, K_IMM16, { 0 } },
                                     { "D", { .base = 2 }, K_REGNAME, { 0 } },
                                     { "" },
                                     { "B", { .base = 0 }, K_REGNAME, { 0 } },
                                     { "RNC", { .base = 0xD0 }, K_SINGLE, { 0 } },
                                     { "CNC", { .base = 0xD4 }, K_IMM16, { 0 } },
                                     { "RZ", { .base = 0xC8 }, K_SINGLE, { 0 } },
                                     { "CZ", { .base = 0xCC }, K_IMM16, { 0 } },
                                     { "DW", { .base = 0 }, K_DW, { 0 } },
                                     { "DCR", { .base = 5 }, K_INRDCR, { 0 } },
                                     { "" },
                                     { "" },
                                     { "A", { .base = 7 }, K_REGNAME, { 0 } },
                                     { "DB", { .base = 0 }, K_DB, { 0 } },
                                     { "" },
                                     { "" },
                                     { "DAD", { .base = 9 }, K_REG16, { 0 } },
                                     { "END", { .base = 0 }, K_END, { 0 } },
                                     { "ENDM", { .base = 0 }, K_ENDM, { 0 } },
                                     { "ENDIF", { .base = 0 }, K_ENDIF, { 0 } },
                                     { "RM", { .base = 0xF8 }, K_SINGLE, { 0 } },
                                     { "CM", { .base = 0xFC }, K_IMM16, { 0 } },
                                     { "RPO", { .base = 0xE0 }, K_SINGLE, { 0 } },
                                     { "CPO", { .base = 0xE4 }, K_IMM16, { 0 } },
                                     { "CMC", { .base = 0x3F }, K_SINGLE, { 0 } },
                                     { "RRC", { .base = 0xF }, K_SINGLE, { 0 } },
                                     { "JP", { .base = 0xF2 }, K_IMM16, { 0 } },
                                     { "DAA", { .base = 0x27 }, K_SINGLE, { 0 } },
                                     { "CPI", { .base = 0xFE }, K_IMM8, { 0 } },
                                     { "" },
                                     { "JC", { .base = 0xDA }, K_IMM16, { 0 } },
                                     { "CMP", { .base = 0xB8 }, K_ARITH, { 0 } },
                                     { "JPE", { .base = 0xEA }, K_IMM16, { 0 } },
                                     { "IN", { .base = 0xDB }, K_IMM8, { 0 } },
                                     { "AND", { .base = 0 }, K_AND, { 0 } },
                                     { "INR", { .base = 4 }, K_INRDCR, { 0 } },
                                     { "L", { .base = 5 }, K_REGNAME, { 0 } },
                                     { "JNC", { .base = 0xD2 }, K_IMM16, { 0 } },
                                     { "RLC", { .base = 7 }, K_SINGLE, { 0 } },
                                     { "JZ", { .base = 0xCA }, K_IMM16, { 0 } },
                                     { "H", { .base = 4 }, K_REGNAME, { 0 } },
                                     { "RAL", { .base = 0x17 }, K_SINGLE, { 0 } },
                                     { "" },
                                     { "CALL", { .base = 0xCD }, K_IMM16, { 0 } },
                                     { "CMA", { .base = 0x2F }, K_SINGLE, { 0 } },
                                     { "ANA", { .base = 0xA0 }, K_ARITH, { 0 } },
                                     { "INPAGE", { .base = 1 }, K_SPECIAL, { 0 } },
                                     { "SP", { .base = 6 }, K_SP, { 0 } },
                                     { "" },
                                     { "OR", { .base = 0 }, K_OR, { 0 } },
                                     { "DCX", { .base = 0xB }, K_REG16, { 0 } },
                                     { "EQ", { .base = 0 }, K_EQ, { 0 } },
                                     { "ACI", { .base = 0xCE }, K_IMM8, { 0 } },
                                     { "JM", { .base = 0xFA }, K_IMM16, { 0 } },
                                     { "M", { .base = 6 }, K_REGNAME, { 0 } },
                                     { "JPO", { .base = 0xE2 }, K_IMM16, { 0 } },
                                     { "ANI", { .base = 0xE6 }, K_IMM8, { 0 } },
                                     { "PCHL", { .base = 0xE9 }, K_SINGLE, { 0 } },
                                     { "REPT", { .base = 0 }, K_REPT, { 0 } },
                                     { "PAGE", { .base = 2 }, K_SPECIAL, { 0 } },
                                     { "" },
                                     { "LT", { .base = 0 }, K_LT, { 0 } },
                                     { "MACRO", { .base = 0 }, K_MACRO, { 0 } },
                                     { "JMP", { .base = 0xC3 }, K_IMM16, { 0 } },
                                     { "IRP", { .base = 0 }, K_IRP, { 0 } },
                                     { "IRPC", { .base = 0 }, K_IRPC, { 0 } },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "EI", { .base = 0xFB }, K_SINGLE, { 0 } },
                                     { "GT", { .base = 0 }, K_GT, { 0 } },
                                     { "SBB", { .base = 0x98 }, K_ARITH, { 0 } },
                                     { "ORA", { .base = 0xB0 }, K_ARITH, { 0 } },
                                     { "STC", { .base = 0x37 }, K_SINGLE, { 0 } },
                                     { "IF", { .base = 0 }, K_IF, { 0 } },
                                     { "DI", { .base = 0xF3 }, K_SINGLE, { 0 } },
                                     { "INX", { .base = 3 }, K_REG16, { 0 } },
                                     { "PUBLIC", { .base = 0 }, K_PUBLIC, { 0 } },
                                     { "RET", { .base = 0xC9 }, K_SINGLE, { 0 } },
                                     { "POP", { .base = 0xC1 }, K_REG16, { 0 } },
                                     { "" },
                                     { "" },
                                     { "SHR", { .base = 0 }, K_SHR, { 0 } },
                                     { "ORI", { .base = 0xF6 }, K_IMM8, { 0 } },
                                     { "" },
                                     { "" },
                                     { "XRA", { .base = 0xA8 }, K_ARITH, { 0 } },
                                     { "" },
                                     { "" },
                                     { "NE", { .base = 0 }, K_NE, { 0 } },
                                     { "XCHG", { .base = 0xEB }, K_SINGLE, { 0 } },
                                     { "SBI", { .base = 0xDE }, K_IMM8, { 0 } },
                                     { "LE", { .base = 0 }, K_LE, { 0 } },
                                     { "STA", { .base = 0x32 }, K_IMM16, { 0 } },
                                     { "STAX", { .base = 2 }, K_LDSTAX, { 0 } },
                                     { "STACK", { .base = 0 }, K_SPECIAL, { 0x1B } },
                                     { "ELSE", { .base = 0 }, K_ELSE, { 0 } },
                                     { "XRI", { .base = 0xEE }, K_IMM8, { 0 } },
                                     { "CSEG", { .base = 0 }, K_CSEG, { 0 } },
                                     { "SPHL", { .base = 0xF9 }, K_SINGLE, { 0 } },
                                     { "DS", { .base = 0 }, K_DS, { 0 } },
                                     { "GE", { .base = 0 }, K_GE, { 0 } },
                                     { "NAME", { .base = 0 }, K_NAME, { 0 } },
                                     { "" },
                                     { "XOR", { .base = 0 }, K_XOR, { 0 } },
                                     { "ORG", { .base = 0 }, K_ORG, { 0 } },
                                     { "" },
                                     { "DSEG", { .base = 0 }, K_DSEG, { 0 } },
                                     { "" },
                                     { "XTHL", { .base = 0xE3 }, K_SINGLE, { 0 } },
                                     { "OUT", { .base = 0xD3 }, K_IMM8, { 0 } },
                                     { "LOCAL", { .base = 0 }, K_LOCAL, { 0 } },
                                     { "" },
                                     { "NOP", { .base = 0 }, K_SINGLE, { 0 } },
                                     { "" },
                                     { "HLT", { .base = 0x76 }, K_SINGLE, { 0 } },
                                     { "" },
                                     { "PUSH", { .base = 0xC5 }, K_REG16, { 0 } },
                                     { "ASEG", { .base = 0 }, K_ASEG, { 0 } },
                                     { "LHLD", { .base = 0x2A }, K_IMM16, { 0 } },
                                     { "SUB", { .base = 0x90 }, K_ARITH, { 0 } },
                                     { "RST", { .base = 0xC7 }, K_RST, { 0 } },
                                     { "" },
                                     { "" },
                                     { "RNZ", { .base = 0xC0 }, K_SINGLE, { 0 } },
                                     { "CNZ", { .base = 0xC4 }, K_IMM16, { 0 } },
                                     { "" },
                                     { "SHL", { .base = 0 }, K_SHL, { 0 } },
                                     { "SHLD", { .base = 0x22 }, K_IMM16, { 0 } },
                                     { "ADC", { .base = 0x88 }, K_ARITH, { 0 } },
                                     { "RIM", { .base = 0x20 }, K_SINGLE, { 2 } },
                                     { "MVI", { .base = 6 }, K_MVI, { 0 } },
                                     { "SET", { .base = 0 }, K_SET, { 0 } },
                                     { "" },
                                     { "NOT", { .base = 0 }, K_NOT, { 0 } },
                                     { "" },
                                     { "MOD", { .base = 0 }, K_MOD, { 0 } },
                                     { "" },
                                     { "ADD", { .base = 0x80 }, K_ARITH, { 0 } },
                                     { "" },
                                     { "SUI", { .base = 0xD6 }, K_IMM8, { 0 } },
                                     { "NUL", { .base = 0 }, K_NUL, { 0 } },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "STKLN", { .base = 0 }, K_STKLN, { 0 } },
                                     { "" },
                                     { "" },
                                     { "MOV", { .base = 0x40 }, K_MOV, { 0 } },
                                     { "EQU", { .base = 0 }, K_EQU, { 0 } },
                                     { "EXTRN", { .base = 0 }, K_EXTRN, { 0 } },
                                     { "" },
                                     { "" },
                                     { "EXITM", { .base = 0 }, K_EXITM, { 0 } },
                                     { "" },
                                     { "" },
                                     { "JNZ", { .base = 0xC2 }, K_IMM16, { 0 } },
                                     { "" },
                                     { "PSW", { .base = 6 }, K_REGNAME, { 0 } },
                                     { "ADI", { .base = 0xC6 }, K_IMM8, { 0 } },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "LOW", { .base = 0 }, K_LOW, { 0 } },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "HIGH", { .base = 0 }, K_HIGH, { 0 } },
                                     { "" },
                                     { "LDA", { .base = 0x3A }, K_IMM16, { 0 } },
                                     { "LDAX", { .base = 0xA }, K_LDSTAX, { 0 } },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "MEMORY", { .base = 0 }, K_SPECIAL, { 0x1C } },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "SIM", { .base = 0x30 }, K_SINGLE, { 2 } },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "" },
                                     { "LXI", { .base = 1 }, K_LXI, { 0 } } };

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
