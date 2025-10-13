/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m20 macin.perf  */
/* Computed positions: -k'1-3' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

struct mnemonic;

#include "mac.h"
enum {ONECHARWORDS = 16};
#include <string.h>
enum
  {
    TOTAL_KEYWORDS = 140,
    MIN_WORD_LENGTH = 1,
    MAX_WORD_LENGTH = 6,
    MIN_HASH_VALUE = 4,
    MAX_HASH_VALUE = 186
  };

/* maximum key range = 183, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static unsigned char asso_values[] =
    {
      187, 187, 187, 187, 187, 187, 187, 187, 187, 187,
      187, 187, 187,  52, 187, 187, 187, 187, 187, 187,
      187, 187, 187, 187, 187, 187, 187, 187, 187, 187,
      187, 187, 187, 187, 187, 187, 187, 187, 187, 187,
       39,  30,  28,  16,   3,   6, 187,   3, 187, 187,
        3, 187, 187, 187, 187, 187, 187, 187, 187, 187,
      187, 187, 187, 187, 187,  27,  64,   4,  15,   5,
       85,  46,  26,  42,  31,  69,  50,  88,  68,  13,
       10,  36,   3,  36,  29, 114,  45,  59,  48,   8,
       89, 187, 119, 187, 187, 187, 187, 187, 187, 187,
      187, 187, 187, 187, 187, 187, 187, 187, 187, 187,
      187, 187, 187, 187, 187, 187, 187, 187, 187, 187,
      187, 187, 187, 187, 187, 187, 187, 187, 187, 187
    };
  register unsigned int hval = (unsigned int)len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]+2];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static struct mnemonic wordlist[] =
  {
    {"",0,0,0}, {"",0,0,0}, {"",0,0,0}, {"",0,0,0},
    {"/",0,_div,0x50},
    {"C",0,_regs,0x01},
    {"E",0,_regs,0x03},
    {"-",0,_minus,0x46},
    {"RP",244,_1byte,0xf0},
    {"CP",238,_jpcl,0xf4},
    {"RC",243,_1byte,0xd8},
    {"CC",237,_jpcl,0xdc},
    {",0,",_comma,0x0a},
    {"RAR",181,_1byte,0x1f},
    {"RPE",251,_1byte,0xe8},
    {"CPE",249,_jpcl,0xec},
    {"D",0,_regs,0x02},
    {"+",0,_plus,0x46},
    {"RM",245,_1byte,0xf8},
    {"CM",239,_jpcl,0xfc},
    {"RNC",242,_1byte,0xd0},
    {"CNC",236,_jpcl,0xd4},
    {"RPO",250,_1byte,0xe0},
    {"CPO",248,_jpcl,0xe4},
    {"CMC",151,_1byte,0x3f},
    {"DW",131,_pseudo,_dw},
    {"DCR",156,_rop,0x05},
    {"H",0,_regs,0x04},
    {"A",0,_regs,0x07},
    {"*",0,_mul,0x50},
    {"CMP",152,_alur,0xb8},
    {")",0,_rpar,0x1e},
    {"DB",128,_pseudo,_db},
    {"END",158,_pseudo,_end},
    {"ENDM",203,_pseudo,_endm},
    {"ENDIF",218,_pseudo,_endif},
    {"JP",232,_jpcl,0xf2},
    {"DAD",155,_rp,0x09},
    {"JC",231,_jpcl,0xda},
    {"RRC",184,_1byte,0x0f},
    {"(",0,_lpar,0x14},
    {"SP",142,_regs,0x06},
    {"JPE",247,_jpcl,0xea},
    {"EQ",133,_eq,0x41},
    {"OR",141,_or,0x28},
    {"PCHL",210,_1byte,0xe9},
    {"JM",233,_jpcl,0xfa},
    {"CMA",150,_1byte,0x2f},
    {"JNC",230,_jpcl,0xd2},
    {"DAA",154,_1byte,0x27},
    {"JPO",246,_jpcl,0xe2},
    {"L",0,_regs,0x05},
    {"CPI",153,_im8,0xfe},
    {"\015",0,_cr,0x0a},
    {"IN",137,_io,0xdb},
    {"AND",148,_and,0x32},
    {"",0,0,0},
    {"JMP",164,_jpcl,0xc3},
    {"INR",161,_rop,0x04},
    {"POP",178,_pshp,0xc1},
    {"RAL",180,_1byte,0x17},
    {"",0,0,0},
    {"CALL",199,_jpcl,0xcd},
    {"REPT",212,_pseudo,_rept},
    {"PAGE",209,_pseudo,_page},
    {"B",0,_regs,0x00},
    {"",0,0,0},
    {"ANA",147,_alur,0xa0},
    {"INPAGE",225,_pseudo,_inpage},
    {"SPHL",214,_1byte,0xf9},
    {"IF",136,_pseudo,_if},
    {"DCX",157,_rpop,0x0b},
    {"ORA",174,_alur,0xb0},
    {"SHR",190,_shr,0x50},
    {"",0,0,0}, {"",0,0,0},
    {"EI",132,_1byte,0xfb},
    {"ACI",143,_im8,0xce},
    {"RLC",183,_1byte,0x07},
    {"",0,0,0}, {"",0,0,0},
    {"RET",182,_1byte,0xc9},
    {"ANI",149,_im8,0xe6},
    {"XCHG",216,_1byte,0xeb},
    {"IRP",163,_pseudo,_irp},
    {"IRPC",205,_pseudo,_irpc},
    {"DI",129,_1byte,0xf3},
    {"ORI",176,_im8,0xf6},
    {"STC",192,_1byte,0x37},
    {"M",0,_regs,0x06},
    {"XOR",195,_xor,0x28},
    {"ORG",175,_pseudo,_org},
    {"",0,0,0},
    {"GT",135,_gt,0x41},
    {"GE",134,_ge,0x41},
    {"LOCAL",221,_pseudo,_local},
    {"SBI",187,_im8,0xde},
    {"LT",139,_lt,0x41},
    {"LE",138,_le,0x41},
    {"",0,0,0}, {"",0,0,0},
    {"MACRO",222,_pseudo,_macro},
    {"MACLIB",226,_pseudo,_maclib},
    {"INX",162,_rpop,0x03},
    {"OUT",177,_io,0xd3},
    {"RNZ",240,_1byte,0xc0},
    {"CNZ",234,_jpcl,0xc4},
    {"XRA",196,_alur,0xa8},
    {"",0,0,0},
    {"PUSH",211,_pshp,0xc5},
    {"",0,0,0},
    {"STA",191,_lsd,0x32},
    {"STAX",215,_lsax,0x02},
    {"ELSE",202,_pseudo,_else},
    {"SET",188,_pseudo,_set},
    {"",0,0,0},
    {"NE",140,_ne,0x41},
    {"NOP",171,_1byte,0x00},
    {"SBB",186,_alur,0x98},
    {"ADC",144,_alur,0x88},
    {"SHL",189,_shl,0x50},
    {"SHLD",213,_lsd,0x22},
    {"XRI",197,_im8,0xee},
    {"XTHL",217,_1byte,0xe3},
    {"RZ",241,_1byte,0xc8},
    {"CZ",235,_jpcl,0xcc},
    {"HLT",160,_1byte,0x76},
    {"CSEG",200,_pseudo,_cseg},
    {"EXTRN",220,_pseudo,_extrn},
    {"",0,0,0},
    {"ADD",145,_alur,0x80},
    {"DS",130,_pseudo,_ds},
    {"TITLE",224,_pseudo,_title},
    {"JNZ",228,_jpcl,0xc2},
    {"",0,0,0},
    {"LHLD",207,_lsd,0x2a},
    {"NOT",172,_not,0x3c},
    {"",0,0,0},
    {"DSEG",201,_pseudo,_dseg},
    {"PUBLIC",227,_pseudo,_public},
    {"SUI",194,_im8,0xd6},
    {"EXITM",219,_pseudo,_exitm},
    {"MOD",168,_mod,0x50},
    {"",0,0,0}, {"",0,0,0},
    {"HIGH",204,_high,0x1e},
    {"",0,0,0}, {"",0,0,0},
    {"LOW",166,_low,0x1e},
    {"RST",185,_rst,0xc7},
    {"ASEG",198,_pseudo,_else},
    {"",0,0,0},
    {"JZ",229,_jpcl,0xca},
    {"",0,0,0}, {"",0,0,0},
    {"STKLN",223,_pseudo,_stkln},
    {"",0,0,0},
    {"ADI",146,_im8,0xc6},
    {"EQU",159,_pseudo,_equ},
    {"",0,0,0}, {"",0,0,0}, {"",0,0,0},
    {"SUB",193,_alur,0x90},
    {"",0,0,0},
    {"NAME",208,_pseudo,_name},
    {"LDA",165,_lsd,0x3a},
    {"LDAX",206,_lsax,0x0a},
    {"",0,0,0}, {"",0,0,0}, {"",0,0,0}, {"",0,0,0},
    {"",0,0,0},
    {"MOV",169,_mov,0x40},
    {"",0,0,0}, {"",0,0,0}, {"",0,0,0}, {"",0,0,0},
    {"",0,0,0}, {"",0,0,0}, {"",0,0,0},
    {"NUL",173,_nul,0x00},
    {"MVI",170,_mvi,0x06},
    {"",0,0,0}, {"",0,0,0},
    {"LXI",167,_rp16,0x01},
    {"",0,0,0},
    {"PSW",179,_regs,0x06}
  };

struct mnemonic *
GetMnemo (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}

static uint8_t map[129];    // max is 127, map[128] is flag

char *getMnemStr(uint8_t code) {
    if (map[128] == 0) {
        for (uint8_t i = 0; i < (uint8_t)(sizeof(wordlist)/sizeof(wordlist[0])); i++)
            if (wordlist[i].tokId)
                map[wordlist[i].tokId - 128] = i;
        map[128] = 1;
    }
    return code < 128 ? "" : wordlist[map[code - 128]].name;
}

