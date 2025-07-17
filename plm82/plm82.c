/****************************************************************************
 *  plm82.c: part of the C port of Intel's cross compiler plm82             *
 *  The original application is Copyright Intel                             *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// end of change
/*************************************************************************/

/*         8 0 8 0   p l / m   c o m p i l e r ,   p a s s - 2          */
/*                                 plm82                                */
/*                              version 2.0/4.0 hybrid                  */
/*                             january/November, 1975                   */

/*                          copyright (c) 1975                          */
/*                          intel corporation                           */
/*                          3065 bowers avenue                          */
/*                          santa clara, california 95051               */

/*  modifyed by jeff ogden (um), december 1977.                         */

/*************************************************************************/

/*         p a s s - 2     e r r o r    m e s s a g e s*/

/*  error                     message*/
/*  number*/
/*  ------  --- -------------------------------------------------------*/

/*   101     reference to storage locations outside the virtual memory*/
/*           of pass-2.  RE-compile pass-2 with larger 'memory' array.*/

/*   102         "*/

/*   103     virtual memory overflow.  program is too large to compile*/
/*           with present size of 'memory.'  either shorten program OR*/
/*           recompile pass-2 with a larger virtual memory.*/

/*   104     (same as 103).*/

/*   105     $toggle used improperly IN pass-2.  attempt to complement*/
/*           a toggle which has a value other than 0 OR 1.*/

/*   106     register allocation table underflow.  may be due to a pre-*/

/*   107     register allocation error.  no registers available.  may*/
/*           be caused by a previous error, OR pass-2 compiler error.*/

/*   108     pass-2 symbol table overflow.  reduce number of*/
/*           symbols, OR RE-compile pass-2 with larger symbol table.*/

/*   109     symbol table overflow (see error 108).*/

/*   110     memory allocation error.  too much storage specified IN*/
/*           the source program (16k max).  reduce source program*/
/*           memory requirements.*/

/*   111     inline data format error.  may be due to improper*/
/*           record size IN symbol table file passed to pass-2.*/

/*   112     (same as error 107).*/

/*   113     register allocation stack overflow.  either simplify the*/
/*           program OR increase the size of the allocation stacks.*/

/*   114     pass-2 compiler error IN 'litadd' -- may be due to a*/
/*           previous error.*/

/*   115     (same as 114).*/

/*   116     (same as 114).*/

/*   117     line width set too narrow for code dump (use $width=n)*/

/*   118     (same as 107).*/

/*   119     (same as 110).*/

/*   120     (same as 110, but may be a pass-2 compiler error).*/

/*   121     (same as 108).*/

/*   122     program requires too much program AND variable storage.*/
/*           (program AND variables exceed 16k).*/

/*   123     initialized storage overlaps previously initialized storage.*/

/*   124     initialization table format error.  (see error 111).*/

/*   125     inline data error.  may have been caused by previous error.*/
//           This can also be caused by insufficient symbol space when
//           an inline constant is being defined (See card 44130)

/*   126     built-IN function improperly called.*/

/*   127     invalid intermediate language format. (see error 111).*/

/*   128     (same as error 113).*/

/*   129     invalid use of built-IN function IN an assignment.*/

/*   130     pass-2 compiler error.  invalid variable precision (NOT */
/*           single byte OR double byte).  may be due to previous error.*/

/*   131     LABEL resolution error IN pass-2 (may be compiler error).*/

/*   132     (same as 108).*/

/*   133     (same as 113).*/

/*   134     invalid program transfer (only computed jumps are allowed */
/*           with a 'go to').*/

/*   135     (same as 134).*/

/*   136     error IN built-IN function call. */

/*   137     (NOT used)*/

/*   138     (same as 107). */

/*   139     error IN changing variable to address reference. may*/
/*           be a pass-2 compiler error, OR may be caused by pre- */
/*           vous error.*/

/*   140     (same as 107).*/

/*   141     invalid origin.  code has already been generated IN the*/
/*           specified locations.*/

/*   142     a symbol table dump has been specified (using the $memory */
/*           toggle IN pass-1), but no file has been specified to RE-*/
/*           ceive the bnpf tape (use the $bnpf=n control).*/

/*   143     invalid format for the simulator symbol table dump (see*/
/*           error 111). */

/*   144     stack NOT empty at END of compilation.  possibly caused */
/*           by previous compilation error.*/

/*   145     procedures nested too deeply (hl optimization) */
/*           simplify nesting, OR RE-compile with larger pstack */

/*   146     procedure optimization stack underflow.  may be a */
/*           return IN outer block. */

/*   147     stack NOT empty at END of compilation. register */
/*           stack order is invalid.  may be due to previous error. */

/*   148     pass-2 compiler error.  attempt to unstack too*/
/*           many values.  may be due to previous error.*/

/*   149     pass-2 compiler error. attempt to convert invalid */
/*           value to address type.  may be due to previous error.*/

/*   150     (same as 147) */

/*   151     pass-2 compiler error. unbalanced  execution stack*/
/*           at block END.  may be due to a previous error.*/

/*    152    invalid stack order IN apply.  may be due to previous*/
/*           error.*/
#ifdef V4
//    153    Illegal Control Record: First nonblank characterr was
//           other than a dollar sign

//    154    Bad code origin from pass-1 (e.g. Pass-1 error 48)
#endif

/*              i m p l e m e n t a t i o n    n o t e s */
/*              - - - - - - - - - - - - - -    - - - - -*/

#include "utility.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#define DIRSEP "/\\:"
#else
#include <limits.h>
#define _MAX_PATH PATH_MAX
#define DIRSEP    "/"
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))
#endif

#define symAttrib(ch)                  symbol[symbol[ch] - 1]
#define symAddr(ch)                    symbol[symbol[ch]]
#define symRef(ch)                     symbol[symbol[ch] - 2]
#define symF3(ch)                      symbol[symbol[ch] - 3]
#define symIProcDepth(ch)              symbol[symbol[ch] - 4]
// VARB e..e ssss 0001   e..e - number of elements, ssss size of element
#define INFO_TYPE(a)                   (abs(a) & 0xf)
#define INFO_PREC(a)                   ((abs(a) >> 4) & 0xf)
#define INFO_ECNT(a)                   ((abs(a) >> 8) & 0xffff)
#define PACK_ATTRIB(extra, prec, type) ((extra) * 256 + (prec) * 16 + type)
#define MAXSYM                         16000
#define MAXMEM                         0x10000
FILE *files[20];

/* global variables*/
#define ZPAD 0

/* cntrl */
int contrl[64 + 1];

/* types */
/* prstrasnlitv*/
enum { VARB = 1, INTR, PROC, LABEL, LITER, NUMBER };

/* bifloc */
// int inloc = 16;
const int outloc = 17;
const int firsti = 7;
int casjmp       = 0;

/* rgmapp */
int regmap[9 + 1] = { ZPAD, 7, 0, 1, 2, 3, 4, 5, 6, 6 };

/* ops */
// clang-format off
enum ops {
    LD = 1, IN,   DC,   AD,  AC,   SU,   SB,   ND,  XR,  OR,
    CP,     ROT,  JMP,  JMC, CAL,  CLC,  RTN,  RTC, RST, INP,
    OUT,    HALT, STA,  LDA, XCHG, SPHL, PCHL, CMA, STC, CMC,
    DAA,    SHLD, LHLD, EI,  DI,   LXI,  PUSH, POP, DAD, STAX,
    LDAX,   INCX, DCX
};
enum reg { RA = 1, RB = 2, RC = 3, RD = 4, RE = 5, RH = 6, RL = 7, RSP = 9, ME = 8 };
enum flags {
    LFT   = 9,  RGT  = 10, TRU  = 12, FAL    = 11, CY = 13, ACC = 14,
    CARRY = 15, ZERO = 16, SIGN = 17, PARITY = 18
};
// clang-format on
// flag values stored  in rasn bits 8-12
#define ZFLAG  ((16 + 2) << 8)
#define NZFLAG (2 << 8)
#define CFLAG  ((16 + 1) << 8)
#define NCFLAG (1 << 8)

/* titles */ // "PLM2 VERS "
// int title[10 + 1] = { ZPAD, 27, 23, 24, 4, 1, 33, 16, 29, 30, 1 };

/* ... plm2 vers ...*/
int vers = 40;

/* symbl */
int symbol[MAXSYM + 1];
int symax  = MAXSYM;
int sytop  = 0;
int syinfo = MAXSYM;
int lmem;

/* xfropt is used IN branch optimiztion*/
int xfrloc = -1;
int xfrsym = 0;
int tstloc = -1;
int conloc = -1;
int defsym = 0;
int defrh  = -1;
int defrl  = -1;

/* inter */

int intpro[8];

// the builtins are encoded as items, each proceeded by a tag
// if the tag is zero then all done
// if tag is 1-254 then tag number of bytes following are stuffed into memory
// if the tag is 0xff, then the following byte is added
// to the start location and stuffed as a word into memory

// clang-format off

uint8_t multiply[] = {
//  tag   data
    5,    0x79,             /*        mov   a, c  */
          0x93,             /*        sub   e     */
          0x78,             /*        mov   a, b  */
          0x9A,             /*        sbb   d     */
          0xF2,             /*        jp    L000C */
    0xff, 0x0c,
    20,   0x60,             /*        mov   h, b  */
          0x69,             /*        mov   l, c  */
          0xEB,             /*        xchg        */
          0x44,             /*        mov   b, h  */
          0x4D,             /*        mov   c, l  */
          0x21, 0x00, 0x00, /* L000C: lxi   h, 0  */
          0xEB,             /*        xchg        */
          0x78,             /* L0010: mov   a, b  */
          0xB1,             /*        ora   c     */
          0xC8,             /*        rz          */
          0xEB,             /*        xchg        */
          0x78,             /*        mov   a, b  */
          0x1F,             /*        rar         */
          0x47,             /*        mov   b, a  */
          0x79,             /*        mov   a, c  */
          0x1F,             /*        rar         */
          0x4F,             /*        mov   c, a  */
          0xD2,             /*        jnc   L001E */
    0xff, 0x1e,
    4,    0x19,             /*        dad   d     */
          0xEB,             /* L001E: xchg        */
          0x29,             /*        dad   h     */
          0xC3,             /*        jmp   L1010 */
    0xff, 0x10,
    0
};

uint8_t divide[] = {
    //  tag   data
    15,   0x7A,             /*        mov  a, d   */
          0x2F,             /*        cma         */
          0x57,             /*        mov  d, a   */
          0x7B,             /*        mov  a, e   */
          0x2F,             /*        cma         */
          0x5F,             /*        mov  e, a   */
          0x13,             /*        inx  d      */
          0x21, 0x00, 0x00, /*        lxi  h, 0   */
          0x3E, 0x11,       /*        mvi  a, 11h */   
          0xE5,             /* L000C: push h      */
          0x19,             /*        dad  d      */
          0xD2,             /*        jnc  L0012  */
    0xff, 0x12,
    18,   0xE3,             /*        xthl        */
          0xE1,             /* L0012: pop  h      */
          0xF5,             /*        push psw    */
          0x79,             /*        mov  a, c   */
          0x17,             /*        ral         */
          0x4F,             /*        mov  c, a   */
          0x78,             /*        mov  a, b   */
          0x17,             /*        ral         */
          0x47,             /*        mov  b, a   */
          0x7D,             /*        mov  a, l   */
          0x17,             /*        ral         */
          0x6F,             /*        mov  l, a   */
          0x7C,             /*        mov  a, h   */
          0x17,             /*        ral         */
          0x67,             /*        mov  h, a   */
          0xF1,             /*        pop  psw    */
          0x3D,             /*        dcr  a      */
          0xC2,             /*        jnz  L000C  */
    0xff, 0x0c,
    8,
          0xB7,             /*        ora  a      */
          0x7C,             /*        mov  a, h   */
          0x1F,             /*        rar         */
          0x57,             /*        mov  d, a   */
          0x7D,             /*        mov  a, l   */
          0x1F,             /*        rar         */
          0x5F,             /*        mov  e, a   */
          0xC9,             /*        ret         */
    0
};
// clang-format on
uint16_t biftab[2];

/* code */
int codloc = 0;
bool alter;

/* inst */
// replace original ctran array with pre expanded strings to simplify the logic
// the first byte is the number of additional bytes in ascii
// the original compiler uses SBC vs. SBB and has non standard immediate forms for mov and arith
// if STD is defined these are replaced by standard ones, although currently commas are omitted as
// per the original
#ifdef STD
#define MVI(R) "MVI " #R
#define ADI    "ADI"
#define ACI    "ACI"
#define SUI    "SUI"
#define SBI    "SBI"
#define ANI    "ANI"
#define XRI    "XRI"
#define ORI    "ORI"
#define CPI    "CPI"
#define SBB(R) "SBB" #R
#else
#define MVI(R) "MOV " #R "I"
#define ADI    "ADD I"
#define ACI    "ADC I"
#define SUI    "SUB I"
#define SBI    "SBC I"
#define ANI    "ANA I"
#define XRI    "XRA I"
#define ORI    "ORA I"
#define CPI    "CMP I"
#define SBB(R) "SBC " #R
#endif

// clang-format off
struct {
    uint8_t extra;
    uint8_t opcode[7];
} ctran[256] = {
   { 0, "NOP" },    { 2, "LXI B" },  { 0, "STAX B" }, { 0, "INX B" },  { 0, "INR B" },  { 0, "DCR B" },  { 1,  MVI(B) },  { 0, "RLC" },
   { 0, "DB 08H" }, { 0, "DAD B" },  { 0, "LDAX B" }, { 0, "DCX B" },  { 0, "INR C" },  { 0, "DCR C" },  { 1,  MVI(C) },  { 0, "RRC" },
   { 0, "DB 10H" }, { 2, "LXI D" },  { 0, "STAX D" }, { 0, "INX D" },  { 0, "INR D" },  { 0, "DCR D" },  { 1,  MVI(D) },  { 0, "RAL" },
   { 0, "DB 18H" }, { 0, "DAD D" },  { 0, "LDAX D" }, { 0, "DCX D" },  { 0, "INR E" },  { 0, "DCR E" },  { 1,  MVI(E) },  { 0, "RAR" },
   { 0, "RIM" },    { 2, "LXI H" },  { 2, "SHLD" },   { 0, "INX H" },  { 0, "INR H" },  { 0, "DCR H" },  { 1,  MVI(H) },  { 0, "DAA" },
   { 0, "DB 28H" }, { 0, "DAD H" },  { 2, "LHLD" },   { 0, "DCX H" },  { 0, "INR L" },  { 0, "DCR L" },  { 1,  MVI(L) },  { 0, "CMA" },
   { 0, "SIM" },    { 2, "LXI SP" }, { 2, "STA" },    { 0, "INX SP" }, { 0, "INR M" },  { 0, "DCR M" },  { 1,  MVI(M) },  { 0, "STC" },
   { 0, "DB 38H" }, { 0, "DAD SP" }, { 2, "LDA" },    { 0, "DCX SP" }, { 0, "INR A" },  { 0, "DCR A" },  { 1,  MVI(A) },  { 0, "CMC" },
   { 0, "MOV BB" }, { 0, "MOV BC" }, { 0, "MOV BD" }, { 0, "MOV BE" }, { 0, "MOV BH" }, { 0, "MOV BL" }, { 0, "MOV BM" }, { 0, "MOV BA" },
   { 0, "MOV CB" }, { 0, "MOV CC" }, { 0, "MOV CD" }, { 0, "MOV CE" }, { 0, "MOV CH" }, { 0, "MOV CL" }, { 0, "MOV CM" }, { 0, "MOV CA" },
   { 0, "MOV DB" }, { 0, "MOV DC" }, { 0, "MOV DD" }, { 0, "MOV DE" }, { 0, "MOV DH" }, { 0, "MOV DL" }, { 0, "MOV DM" }, { 0, "MOV DA" },
   { 0, "MOV EB" }, { 0, "MOV EC" }, { 0, "MOV ED" }, { 0, "MOV EE" }, { 0, "MOV EH" }, { 0, "MOV EL" }, { 0, "MOV EM" }, { 0, "MOV EA" },
   { 0, "MOV HB" }, { 0, "MOV HC" }, { 0, "MOV HD" }, { 0, "MOV HE" }, { 0, "MOV HH" }, { 0, "MOV HL" }, { 0, "MOV HM" }, { 0, "MOV HA" },
   { 0, "MOV LB" }, { 0, "MOV LC" }, { 0, "MOV LD" }, { 0, "MOV LE" }, { 0, "MOV LH" }, { 0, "MOV LL" }, { 0, "MOV LM" }, { 0, "MOV LA" },
   { 0, "MOV MB" }, { 0, "MOV MC" }, { 0, "MOV MD" }, { 0, "MOV ME" }, { 0, "MOV MH" }, { 0, "MOV ML" }, { 0, "HLT" },    { 0, "MOV MA" },
   { 0, "MOV AB" }, { 0, "MOV AC" }, { 0, "MOV AD" }, { 0, "MOV AE" }, { 0, "MOV AH" }, { 0, "MOV AL" }, { 0, "MOV AM" }, { 0, "MOV AA" },
   { 0, "ADD B" },  { 0, "ADD C" },  { 0, "ADD D" },  { 0, "ADD E" },  { 0, "ADD H" },  { 0, "ADD L" },  { 0, "ADD M" },  { 0, "ADD A" },
   { 0, "ADC B" },  { 0, "ADC C" },  { 0, "ADC D" },  { 0, "ADC E" },  { 0, "ADC H" },  { 0, "ADC L" },  { 0, "ADC M" },  { 0, "ADC A" },
   { 0, "SUB B" },  { 0, "SUB C" },  { 0, "SUB D" },  { 0, "SUB E" },  { 0, "SUB H" },  { 0, "SUB L" },  { 0, "SUB M" },  { 0, "SUB A" },
   { 0,  SBB(B) },  { 0,  SBB(C) },  { 0,  SBB(D) },  { 0,  SBB(E) },  { 0,  SBB(H) },  { 0,  SBB(L) },  { 0,  SBB(M) },  { 0,  SBB(A) },
   { 0, "ANA B" },  { 0, "ANA C" },  { 0, "ANA D" },  { 0, "ANA E" },  { 0, "ANA H" },  { 0, "ANA L" },  { 0, "ANA M" },  { 0, "ANA A" },
   { 0, "XRA B" },  { 0, "XRA C" },  { 0, "XRA D" },  { 0, "XRA E" },  { 0, "XRA H" },  { 0, "XRA L" },  { 0, "XRA M" },  { 0, "XRA A" },
   { 0, "ORA B" },  { 0, "ORA C" },  { 0, "ORA D" },  { 0, "ORA E" },  { 0, "ORA H" },  { 0, "ORA L" },  { 0, "ORA M" },  { 0, "ORA A" },
   { 0, "CMP B" },  { 0, "CMP C" },  { 0, "CMP D" },  { 0, "CMP E" },  { 0, "CMP H" },  { 0, "CMP L" },  { 0, "CMP M" },  { 0, "CMP A" },
   { 0, "RNZ" },    { 0, "POP B" },  { 2, "JNZ" },    { 2, "JMP" },    { 2, "CNZ" },    { 0, "PUSH B" }, { 1,  ADI },     { 0, "RST 0" },
   { 0, "RZ" },     { 0, "RET" },    { 2, "JZ" },     { 0, "DB CBH" }, { 2, "CZ" },     { 2, "CALL" },   { 1,  ACI },     { 0, "RST 1" },
   { 0, "RNC" },    { 0, "POP D" },  { 2, "JNC" },    { 1, "OUT" },    { 2, "CNC" },    { 0, "PUSH D" }, { 1,  SUI },     { 0, "RST 2" },
   { 0, "RC" },     { 0, "DB D9H" }, { 2, "JC" },     { 1, "IN" },     { 2, "CC" },     { 0, "DB DDH" }, { 1,  SBI },     { 0, "RST 3" },
   { 0, "RPO" },    { 0, "POP H" },  { 2, "JPO" },    { 0, "XTHL" },   { 2, "CPO" },    { 0, "PUSH H" }, { 1,  ANI },     { 0, "RST 4" },
   { 0, "RPE" },    { 0, "PCHL" },   { 2, "JPE" },    { 0, "XCHG" },   { 2, "CPE" },    { 0, "DB EDH" }, { 1,  XRI },     { 0, "RST 5" },
   { 0, "RP" },     { 0, "POP A" },  { 2, "JP" },     { 0, "DI" },     { 2, "CP" },     { 0, "PUSH A" }, { 1,  ORI },     { 0, "RST 6" },
   { 0, "RM" },     { 0, "SPHL" },   { 2, "JM" },     { 0, "EI" },     { 2, "CM" },     { 0, "DB FDH" }, { 1,  CPI },     { 0, "RST 7"}
};
// clang-format on
bool errflg = false;

/* peep is used IN peephole optimization (see emit)*/
/* lapol is a one element polish look-ahead*/
/* lastld is codloc of last register to memory store*/
/* lastrg is the effected register*/
/* lastin is the codloc of the last increment*/
/* (used IN do-loop index increment)*/
/* lastex is location of last XCHG operator*/
/* lastir is the codloc of the last register increment*/
/* (used IN apply AND gensto to gen inr memory)*/
int lapol       = -1;
int lastLoad    = 0;
int lastReg     = 0;
int lastin      = 0;
int lastex      = 0;
int lastIncReg  = 0;
int lastLoadImm = 0; // location of last load reg immediate
int lastRegImm  = 0;

/* pstack is the procedure stack used IN hl optimization*/
int prstk[15 + 1];
int maxdep[16];
int curdep[16];
int prsmax = 15;
int prsp   = 0;
int lxis   = 0;

/* ilcod */
// clang-format off
enum { OPR = 0, ADR, VLU, DEF, LIT, LIN };
enum {
    NOP = 0,  ADD, ADC, SUB, SBC, MUL, DIV, MDF, NEG, AND, IOR, XOR, NOT, EQL, LSS,
    GTR, NEQ, LEQ, GEQ, INX, TRA, TRC, PRO, RET, STO, STD, XCH, DEL, DAT, LOD, BIF,
    INC, CSE, END, ENB, ENP, HAL, RTL, RTR, SFL, SFR, HIV, LOV, CVA, ORG, DRT, ENA,
    DIS, AX1, AX2, AX3
};

// note RTL -> LOV are not used by the compiler, instead built in functions are
// assumed for symbols < intbas
// the following enums are for built in functions
enum {
    B_ROL = 1, B_ROR,     B_SHL,    B_SHR,     B_SCL,  B_SCR,  B_TIME,   B_HIGH,
    B_LOW,     B_INPUT,   B_OUTPUT,  B_LENGTH, B_LAST, B_MOVE, B_DOUBLE, B_DEC
};
// clang-format on
/* memory */
int memtop = MAXMEM + 1;
unsigned char mem[MAXMEM]; // upto max memory of 8080
int offset = 0;
int preamb;

/* regall */
int regs[7 + 1];
int regv[7 + 1] = { ZPAD, -1, -1, -1, -1, -1, -1, -1 };
bool lock[7 + 1];

int prec[16 + 1];
int st[16 + 1];

int rasn[16 + 1];

#define HIGHNIBBLE(n)     (((n) >> 4) & 0xf)
#define LOWNIBBLE(n)      ((n) & 0xf)
#define REGPAIR(h, shift) (((h) << 4) + (shift))
#define REGLOW(hl)        ((hl) & 0x7)
#define REGHIGH(hl)       (((hl) >> 4) & 0x7)
#define HIGH(n)           (((n) >> 8) & 0xff)
#define LOW(n)            ((n) & 0xff)
#define HIGHWORD(n)       (((n) >> 16) & 0xffff)
#define LOWWORD(n)        ((n) & 0xffff)

int litv[16 + 1];
int sp    = 0;
int maxsp = 16;

/* intbas is the largest intrinsic symbol number*/
int intbas = 23;

/* function declarations */
int main(const int argc, char **argv);
int get(int ip);
int getword(int ip);
void put(int ip, const int x);
void putword(int ip, const int x);

// void error(const int i, const int level);

void error(char const *msg, ...);
void fatal(char const *msg, ...);

int shr(const int i, const int j);
int shl(const int i, const int j);
int right(const int i, const int j);
void _delete(int n);
void apply(const int op, const int op2, const bool com, const int cyflag);
void genreg(const int np, int *ia, int *ib);
void loadsy();
void loadv(int is, int typv);
void setadr(const int val);
void ustack();
int chain(const int sy, const int loc);
void gensto(const int keep);
void litadd(const int s);
void dump(int l, const int u, bool symbolic);
void emit(const int opr, const int opa, const int opb);
void puncod(const int lb, const int ub, const int mode);
void cvcond(const int s);
void saver();
void reloc();
int loadin();
void emitbf(const int l);
void inldat();
void unary(const int ival);
void exch();
void stack(const int n);
void readcd();
bool operat(int val);
void sydump();
void cmpuse();
void inx(int jp);
void builtin(int bf, int result);
void compare16(bool icom, int flag, int iq);
void updateHL(int jp);

int errorCnt;
int C_ANALYSIS;
int C_COUNT;           // line counter
bool C_FINISH  = true; // dump code at finish
int C_GENERATE = 1;
int C_LOAD; // load address of program
bool C_MAP     = true;
bool C_NUMERIC = false;
bool v4Opt     = false;
int C_SYMBOLS;
int C_VARIABLES;
int C_WIDTH    = 120;
bool C_HEXFILE = true;
int C_STACKHANDLING; // 0->compiler handles stack pointer
                     // 1 - programmer handles stack pointer
                     // > 1 n is stack size

// HL tracking uses the following flags
// the LVALUE is in bits 0-7, the HVALUE is in bits 8-16 (9 bits wide)
#define LVALID  0x20000
#define HVALID  0x40000
#define LNOTSET 0x80000
#define HNOTSET 0x100000

FILE *inFp;
FILE *hexFp;
FILE *outFp;
FILE *polFp;
FILE *lstFp;
FILE *symFp;
char *src;

void closefiles(void) {
    if (polFp) {
        fclose(polFp);
        if (errorCnt == 0)
            unlink(makeFilename(src, ".pol", true));
    }
    if (symFp) {
        fclose(symFp);
        if (errorCnt == 0)
            unlink(makeFilename(src, ".sym", true));
    }
    if (lstFp)
        fclose(lstFp);
    if (hexFp) {
        fclose(hexFp);
        if (errorCnt)
            unlink(makeFilename(src, ".hex", true));
    }
}

void openfiles(char *srcFile) {
    src = srcFile;
    char *path;
    atexit(closefiles);
    if (!(polFp = fopen(path = makeFilename(srcFile, ".pol", true), "rb"))) {
        fprintf(stderr, "can't open pol file %s\n", path);
        exit(1);
    }
    if (!(symFp = fopen(path = makeFilename(srcFile, ".sym", true), "rb"))) {
        fprintf(stderr, "can't open symbol file %s\n", path);
        exit(1);
    }
    if (!(lstFp = fopen(path = makeFilename(srcFile, ".lst", true), "at"))) {
        fprintf(stderr, "can't open listing file %s\n", path);
        exit(1);
    }
    if (C_HEXFILE && !(hexFp = fopen(path = makeFilename(srcFile, ".hex", true), "at"))) {
        fprintf(stderr, "can't create hex file %s\n", path);
        exit(1);
    }
}

char const help[] =
    "Usage: %s [-a n] [-d nn] [-f] [-g n] [-l nn] [-m] [-n] [-o] [-s n] [-v nn] [-x] [-# nn] "
    "plmfile\n"
    "Where\n"
    "-a n       debug - set analysis level != 0 show state, >= 2 show registers\n"
    "-f         disable code dump at finish\n"
    "-g n       trace - 0 no trace, 1 lines vs locs, 2 full interlist- default 1\n"
    "-l nn      start machine code generation at location nn\n"
    "-m         turn off symbol map\n"
    "-n         write emitter trace\n"
    "-o         enable the additional V4 load and arith immediate with 0 optimisations\n"
    "-s n       debug - write symbol info. 0 none, != 0 address , >= 2 detailed info\n"
    "-v nn      set first page of RAM\n"
    "-w nn      set output width, min 72 default 120\n"
    "-x         disable hex file\n"
    "-@ nn      stack handling. 0 system determined, 1 user specified, > 1 stack size\n"
    "plmfile    is the same source file name used in plm81 of the form prefix.ext\n"
    "           intermediate files prefix.lst, prefix.pol and prefix.sym are used\n"
    "           optionally prefix.cfg can be used to hold plm82 configuration flags\n"
    "           prefix.lst is updated with pass2 output added and prefix.hex is created\n"
    "           Note the .pol and .sym files are deleted if pass 2 is successful\n\n";

int main(int argc, char **argv) {
    int i, j, jp, jl, jn, np, k;

    while (getopt(argc, argv, "a:fg:l:mn:os:v:w:x@:") != EOF) {
        switch (optopt) {
        case 'a':
            C_ANALYSIS = atoi(optarg);
            break;
        case 'w':
            if ((C_WIDTH = atoi(optarg)) < 72)
                C_WIDTH = 72;
            else if (C_WIDTH > 132)
                C_WIDTH = 132;
            break;
        case 'f':
            C_FINISH = !C_FINISH;
            break;
        case 'g':
            C_GENERATE = atoi(optarg);
            break;
        case 'l':
            C_LOAD = atoi(optarg);
            break;
        case 'm':
            C_MAP = !C_MAP;
            break;
        case 'n':
            C_NUMERIC = !C_NUMERIC;
            break;
        case 'o':
            v4Opt = true;
            break;
        case 's':
            C_SYMBOLS = atoi(optarg);
            break;
        case 'V':
            if ((C_VARIABLES = atoi(optarg)) > 63)
                C_VARIABLES = 0;
            break;
        case 'x':
            C_HEXFILE = !C_HEXFILE;
            break;
        case '@':
            if ((C_STACKHANDLING = atoi(optarg)) >= 0x10000)
                C_STACKHANDLING = 0;
            break;
        }
    }
    if (optind != argc - 1)
        usage("Expected single file name");

    openfiles(argv[optind]);

    time_t now;
    time(&now);
    fprintf(lstFp, "         pl/m-8080 pass2 Version 4.0 - %s\n", ctime(&now));

    /* change margins for reading intermediate language */

    codloc = C_LOAD;
    loadsy();
    readcd();
    if (!errflg) {
        /* make sure compiler stack is empty */
        if (sp != 0)
            error("144: stack not empty at end of compilation");

        /* make sure execution stack is empty */
        if (curdep[0] != 0)
            error("150: stack not empty at end of compilation. Register stack order is invalid");
        reloc();

        /* may want a symbol table for the simulator */

        sydump();
        if (C_FINISH) {
            /* dump the preamble */
            i      = offset;
            offset = 0;
            for (int ii = 0; ii < 64; ii += 8)
                if (intpro[ii / 8])
                    dump(ii, ii + 2, true);

            offset = i;

            /* dump the symbol table by segments until codloc-1 */
            i = offset + preamb;

            do {
                jp = 99999;
                jl = 0;

                /* locate next inline data at or above i */
                jn = 0;
                np = intbas + 1;
                for (int n = np; n <= sytop; n++) {
                    int attrib = symAttrib(n);
                    if (attrib >= 0 && INFO_TYPE(attrib) == VARB) {
                        j = LOWWORD(abs(symAddr(n)));
                        if (j <= jp && j >= i && (k = INFO_PREC(attrib))) { // check candidate at j
                            jp = j;
                            jn = n;
                            if (k > 2)
                                k = 1;
                            jl = k * INFO_ECNT(attrib);
                        }
                    }
                }

                /* jp is base address of next data stmt, jl is length IN bytes */
                if (i < jp) /* code is printed below */
                    dump(i, min(jp - 1, codloc - 1), true);

                if (jp < codloc) { /* then the data segments */
                    if (C_SYMBOLS != 0)
                        fprintf(lstFp, "S%05d", jn);
                    dump(jp, jp + jl - 1, false);
                }
                i = jp + jl;
            } while (i < codloc);
        }

        int isave = codloc;
        int kval  = loadin();

        if (codloc != isave && C_FINISH)
            dump(kval, codloc - 1, false); /* dump the initialized variables */

        if (C_HEXFILE) {
            /* punch deck */
            k         = offset;
            offset    = 0;

            int mode1 = 1;
            for (int ii = 0; ii < 64; ii += 8)
                if (intpro[ii / 8]) {
                    puncod(ii, ii + 2, mode1);
                    mode1 = 3;
                }

            offset = k;
            if (codloc != isave) {
                puncod(offset + preamb, isave - 1, mode1);
                puncod(kval, codloc - 1, 3);
            } else
                puncod(offset + preamb, codloc - 1, mode1);
            puncod(0, 0, 2);
        }
        /* write error count */
        if (errorCnt == 0) {
            fputs("\nNO PROGRAM ERRORS\n\n", lstFp);
            fputs("\nNO PROGRAM ERRORS\n\n", stdout);
        } else {
            fprintf(lstFp, "%d PROGRAM ERROR%s\n\n", errorCnt, errorCnt != 1 ? "S" : "");
            fprintf(stdout, "%d PROGRAM ERROR%s\n\n", errorCnt, errorCnt != 1 ? "S" : "");
        }
    }
    closefiles();
    cmpuse();
    return errorCnt;
}

int get(int ip) {
    if (ip < offset || ip >= MAXMEM) {
        fatal("101: pass-2 address outside available storage");
        return 0;
    }
    return mem[ip - offset];
}

int getword(int ip) {
    return get(ip) + get(ip + 1) * 256;
}

void put(int ip, const int x) {
    if (ip < offset || ip >= MAXMEM)
        fatal("102: pass-2 address outside available storage");
    mem[ip - offset] = x;
}

void putword(int ip, const int x) {
    put(ip, x % 256);
    put(ip + 1, x / 256);
}

// string msg variant of error
void error(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    errorCnt++;
    fprintf(lstFp, "\n(%05d)  Error ", C_COUNT);
    vfprintf(lstFp, fmt, args);
    putc('\n', lstFp);
    va_end(args);
}

void fatal(char const *msg, ...) {
    error(msg);
    fprintf(lstFp, "\nCompilation Terminated\n");
    fprintf(stderr, "\nCompilation Terminated\n");
    errflg = true;
}

int shr(const int i, const int j) {
    return i / (1 << j);
}

int shl(const int i, const int j) {
    return i << j;
}

int right(const int i, const int j) {
    return i % (1 << j);
}

void _delete(int n) /* _delete the top n elements from the stack */
{
    int i;
    while (n-- > 0) {
        if (sp <= 0) {
            error("106: register allocation table underflow");
            return;
        }
        if ((i = (rasn[sp] >> 4) & 0x7)) {
            if (regs[RA] == i)
                regs[RA] = 0;
            lock[i] = false;
            regs[i] = 0;
        }
        if ((i = rasn[sp] & 0x7)) {
            if (regs[RA] == i)
                regs[RA] = 0;
            lock[i] = false;
            regs[i] = 0;
        }

        sp--;
    }
}

void apply(const int op, const int op2, const bool com, const int cyflag) {
    int ia, ib, l, lp, jp, k;

    /* apply op to top elements of stack */
    /* use op2 for high order bytes if double byte operation */
    /* com = 1 if commutative operator, 0 otherwise */
    /* cyflag = 1 if the CARRY is involved IN the operation */

    /* may want to clear the CARRY for this operation */

    /* check for one of the operands IN the stack (only one can be there) */
    for (int ip = 0, j = sp - 1; j <= sp; j++)
        if (st[j] == 0 && rasn[j] == 0 && litv[j] < 0) { /* operand is stacked */
            genreg(-2, &ia, &ib);
            regs[ia] = j;
            if (ip != 0)
                error("152: Invalid stack order in 'apply'");
            ip = ib;
            if (prec[j] > 1) /* double byte operand */
                regs[ib] = j;
            else /* single precision result */
                ib = 0;
            rasn[j] = REGPAIR(ib, ia);
            emit(POP, ip, 0);
            ustack();
        }

    /* make a quick check for possible accumulator match */
    /* with the second operand */
    if ((ia = rasn[sp]) > 255)
        cvcond(sp);
    if ((ib = rasn[sp - 1]) > 255)
        cvcond(sp - 1);
    l = regs[RA];
    if (ia && ib && l && com &&
        l == ia % 16) /* commutative operator, one may be IN the accumulator */
        exch();       /* second operand IN gpr's, l.o. byte IN accumulator */

    bool storeSetup = false; // replaces use of goto L110

    for (;; exch()) {
        if (!rasn[sp - 1]) {   /* is op1 IN gpr's */
            if (rasn[sp]) {    /* is op2 IN gpr's */
                               /* yes - can we exchange AND try again */
                litv[sp] = -1; /* after insuring that a literal has no regs assigned */
                if (com)
                    continue;
            }
        } else if ((ia = REGLOW(rasn[sp - 1])) == 0) { /* reg assigned, lock regs containing var */
            fatal("107: register allocation error. No registers available");
            return;
        } else {
            lock[ia] = true;
            if ((ib = REGHIGH(rasn[sp - 1])))
                lock[ib] = true;

            /* may have to generate one free reg */
            if (prec[sp - 1] < prec[sp])
                ib = ia - 1;

            /* check for pending register store */
            if ((jp = regs[RA]) != ia) {
                if (jp)
                    emit(LD, jp, RA);
                regs[RA] = ia;
                emit(LD, RA, ia);
            }
        }

        /* op2 NOT IN gpr's OR op is NOT commutative */
        /* check for literal value - is op2 literal */
        if ((k = litv[sp]) >= 0) {
            if (prec[sp] <= 1 && prec[sp - 1] <= 1 &&
                k == 1 /* make special check for possible increment OR decrement */
                && (op == AD || op == SU) /* must be ADD OR subtract without CARRY */
                && prec[sp - 1] == 1) {   /* first operand must be single byte variable */
                if (ia <= 1) {            /* op1 must be IN memory, so load into gpr */
                    loadv(sp - 1, 0);
                    if ((ia = rasn[sp - 1] & 0xf) == 0) {
                        fatal("107: register allocation error. No registers available");
                        return;
                    }
                    lastIncReg = codloc; /* ...may change to inr memory if STD to op1 follows... */
                }
                emit(op == AD ? IN : DC, regs[RA] == ia ? RA : ia, 0);
                storeSetup = true;
            }
            break;
        }

        /* op1 NOT a literal,  check for literal op2 */
        if (litv[sp - 1] < 0 || !com)
            break;
    }

    if (!storeSetup) {
        /* generate registers to hold results IN loadv */
        /* (loadv will load the low order byte into the ACC) */
        loadv(sp - 1, 1);
        if ((ia = REGLOW(rasn[sp - 1])) == 0) {
            fatal("107: register allocation error. No registers available");
            return;
        }
        lock[ia] = true;
        ib       = rasn[sp - 1] / 16;

        /* is this a single byte / double byte operation */
        if (ib <= 0 && prec[sp] != 1) {
            /* get a spare register */
            if ((ib = ia - 1) == 0) { // ia was A reg so can't double byte
                fatal("107: register allocation error. No registers available");
                return;
            }
            lock[ib] = true;
        } else
            ib &= 7;

        /* now ready to perform operation */
        /* l.o. byte is IN AC, h.o. byte is IN ib. */
        /* result goes to ia (l.o.) AND ib (h.o.) */

        /* is op2 IN gpr's */
        k = -1;
        if ((lp = rasn[sp]) > 0) /* perform ACC-reg operation */
            emit(op, REGLOW(lp), 0);
        else if ((k = litv[sp]) < 0) { /* is op2 a literal */
            loadv(sp, 2);              /* perform operation with low order byte */
            emit(op, ME, 0);
        } else if (op == XR && k % 256 == 255) /* use CMA if op is XR AND op2 is LIT 255 */
            emit(CMA, 0, 0);
        else /* perform ACC-immediate operation */
            emit(op, -k % 256, 0);

        /* set up a pending register store */
        /* if this is NOT a compare */
        if (op != CP)
            regs[RA] = ia;
        if (prec[sp] == 2) {
            if (k < 0 && lp <= 0) { /* is h.o. byte of op2 IN memory */
                emit(IN, RL, 0);    /* point to h.o. byte with h AND l */
                regv[RL]++;
            }

            /* do we need to pad with h.o. ZERO for op1 */
            if ((jp = regs[RA])) {
                if (jp != ib) { // is store pending
                    emit(LD, jp, RA);
                    regs[RA] = 0;
                    if (prec[sp - 1] > 1)
                        emit(LD, RA, ib);
                    else
                        emit(cyflag ? LD : XR, RA, 0);
                }
            } else if (prec[sp - 1] > 1)
                emit(LD, RA, ib);
            else
                emit(cyflag ? LD : XR, RA, 0);

            if (lp) /* op2 IN gpr's - perform ACC-register operation */
                emit(op2, REGHIGH(lp), 0);
            else if (k < 0) /* perform ACC-memory operation */
                emit(op2, ME, 0);
            else if (op2 != XR || k != 65535) /* yes - perform ACC-immediate operation */
                emit(op2, -HIGH(k), 0);
            else /* use CMA if op1 is XR AND op2 is 65535 */
                emit(CMA, 0, 0);
        } else if (prec[sp - 1] >= 2) { /* second operand is single byte */
            /* may NOT need to perform operations for certain operators, but ... */
            /* perform operation with h.o. byte of op1 */
            /* op1 must be IN the gpr's - perform dummy operation with ZERO */
            if ((jp = regs[RA])) {
                if (jp != ib) {
                    emit(LD, jp, RA);
                    regs[RA] = 0;
                    emit(LD, RA, ib);
                }
            } else
                emit(LD, RA, ib);
            emit(op2, 0, 0);
        } else
            storeSetup = true;
    }
    /* set up pending register store */
    if (!storeSetup)
        regs[RA] = ib;

    /* save the pending accumulator - register store */
    jp = regs[RA];
    _delete(2);
    regs[RA]   = jp;
    prec[++sp] = 1;
    rasn[sp]   = REGPAIR(ib, ia);
    lock[ia]   = false;
    st[sp]     = 0;
    litv[sp]   = -1;
    regs[ia]   = sp;
    regv[ia]   = -1;
    if (ib > 0) {
        prec[sp] = 2;
        regs[ib] = sp;
        lock[ib] = false;
        regv[ib] = -1;
    }
}

void genreg(const int np, int *ia, int *ib) {
    int ip, k;

    /* generate abs(np) free registers for subsequent operation */
    // Note -2 <= np <= 2 np negative if no pushing allowed

    *ib = 0;
    *ia = 0;

    /* look for free RC OR RE AND allocate IN pairs (RC/RB,RE/RD) */
    for (int idump = 0; regs[k = RC] && regs[k = RE];) {
        if (idump > 0) {
            *ia = 0;
            return;
        }
        ip = 0;
        if (np >= 0 && sp > 0) {
            /* generate temporaries IN the stack AND RE-try */
            /* search for lowest register pair assignment IN stack */
            for (int i = 1; i <= sp; i++) {
                k = rasn[i];
                if (k == 0) {
                    if (st[i] == 0 && litv[i] < 0)
                        ip = 0;
                } else if (k <= 255 && ip == 0) {
                    int j  = REGLOW(k);
                    int jp = REGHIGH(k);
                    if (!lock[j] && (jp == 0 || (jp == j - 1 && !lock[jp])))
                        ip = i;
                }
            }
        }
        if (ip == 0) {
            idump = 1;
            saver();
        } else { /* found entry to PUSH at ip */
            int j   = REGLOW(rasn[ip]);
            int jp  = REGHIGH(rasn[ip]);

            regs[j] = 0;
            if (jp > 0)
                regs[jp] = 0;

            /* check pending register store */
            if ((k = regs[RA]) && (k == j || k == jp)) {
                emit(LD, k == j ? j : jp, RA);
                regs[RA] = 0;
            }

            /* free the register for allocation */
            stack(1);
            emit(PUSH, j - 1, 0);

            /* mark element as stacked (st=0, rasn=0) */
            st[ip] = rasn[ip] = 0;
            litv[ip]          = -1;
            ;
        }
    }
    *ia = k;
    if (abs(np) > 1)
        *ib = *ia - 1;
    return;
}

int32_t getSym32() {
    int32_t val;
    fread(&val, sizeof(val), 1, symFp);
    return val;
}

uint16_t getSym16() {
    uint16_t val;
    fread(&val, sizeof(val), 1, symFp);
    return val;
}

void loadsy() {
    int ch;
    bool ok = false;

    // binary format now dumps the 8 interrupt symbols as little endian 16 bit numbers
    // 0 implies none
    for (int i = 0; i < 8; i++) {
        intpro[i] = getc(symFp);
        intpro[i] += getc(symFp) * 256;
        if (intpro[i] && C_SYMBOLS >= 2)
            fprintf(lstFp, "I%d=S%05d\n", i, intpro[i]);
    }

    /* interrupt procedures are handled. */

    while ((ch = getc(symFp))) { // process next symbol table entry
        if (ch != 1 && ch != 2)
            goto badData;
        if (++sytop >= syinfo) {
            fatal("108: pass-2 symbol table overflow");
            syinfo = symax;
        }
        if (C_SYMBOLS >= 2) // write symbol number AND symbol table address
            fprintf(lstFp, "S%05d", sytop);

        symbol[sytop] = syinfo;
        int attribIdx = --syinfo;
        if (syinfo - ch <= sytop) {
            fatal("109: symbol table overflow");
            syinfo = symax;
        }
        while (ch-- > 0) {
            int info         = getSym32();
            symbol[syinfo--] = ch ? -info : info;

            if (C_SYMBOLS >= 2) // write symbol table address AND entry
                fprintf(lstFp, "    %05d %c%08XH", syinfo, ch ? '-' : ' ', info);
        }
        if (C_SYMBOLS >= 2)
            putc('\n', lstFp);
        /* check for special case at END of an entry */
        int attrib = abs(symbol[attribIdx]);
        // allocate additional cell count
        switch (attrib & 0xf) {
        case VARB:
            syinfo -= 1;
            break;
        case PROC:
            syinfo -= 3;
            break;
        case LABEL:
            syinfo -= HIGH(attrib) == 1 ? 2 : 1;
            break; // check for single reference to the label
        }
    }

    /* assign relative memory addresses to variables IN symbol table */
    lmem = 0xff00;
    for (int i = sytop; i > 0; i--) { /* process symbols (backwards) */
        int addr   = -1;
        int attrib = symAttrib(i);

        if (attrib >= 0 && INFO_TYPE(attrib) == VARB) {
            int prec = INFO_PREC(attrib);
            int ecnt = INFO_ECNT(attrib);

            if (prec > 2) /* probably an inline data variable */
                prec = -1;
            else {
                if (prec == 2) // align words to even boundary
                    lmem &= ~1;

                if ((lmem -= prec * ecnt) < 0) {
                    error("110: data storage too big");
                    lmem = 0xff00;
                }
                addr = lmem;
                if (C_SYMBOLS && i > 4 && i != 6) /* write address assignment for real variables */
                    fprintf(lstFp, "S%05d=%05d\n", i, addr);
            }
        }
        symAddr(i) = addr;
    }
    ok = true;
badData:
    if (!ok)
        error("111: inline data format error");

    /* now assign the last address to the variable 'memory' */
    /* ** note that 'memory' must be at location 5 IN the symbol table ** */
    symAddr(5) = 0xff00;
}

void loadv(int s, int typ) {
    int m, l, lp, jp, i, j, k;
    static int ia, ib;

    /* load value to register if NOT a literal */
    /* typ = 1 if call from 'apply' IN which case the l.o. byte is */
    /* loaded into the accumulator instead of a gpr. */
    /* if typ = 2, the address is loaded, but the variable is NOT. */
    /* if typ = 3, a double byte (address) fetch is forced. */
    /* if typ = 4 then do a quick load into h AND l */
    /* if typ = 5, a double byte quick load into h AND l is forced */
    i = 0;
    if (typ != 2) {
        if (rasn[s] > 255)
            cvcond(s);
        if (typ == 4 || typ == 5) {
            m = litv[s];
            k = st[s];
            if (rasn[s]) { /* registers are assigned */
                j = regs[RA];
                l = REGLOW(rasn[s]);
                i = REGHIGH(rasn[s]);
                if (j != 0 && j == i)
                    i = RA;
                if (j != 0 && j == l)
                    l = RA;
                if (l == RE && i == RD)
                    emit(XCHG, 0, 0);
                else { /* NOT IN d AND e, so use two byte move */
                    emit(LD, RL, l);
                    /* note that the following may be a lhi 0 */
                    emit(LD, RH, i);
                }
            } else {
                if (k) {
                    /* variable , literal  OR address reference */
                    if (k <= 0)
                        litadd(sp); /* ADR ref - set h AND l with litadd */
                    else if (m < 0) {
                        /* simple variable OR literal ref, may use LHLD */
                        /* may want to check for possible INX OR DCX, but now... */
                        m = regv[RH];
                        l = regv[RL];
                        if (m != -3 || (-l) != k) {
                            if (m == (-4) && (-l) == k)
                                emit(DCX, RH, 0);
                            else
                                emit(LHLD, chain(k, codloc + 1), 0);
                        }
                        regv[RH] = -1;
                        regv[RL] = -1;
                        if (prec[s] <= 1 && typ != 5)
                            /* this is a single byte value */
                            emit(LD, RH, 0);
                        else {
                            regv[RH] = -3;
                            regv[RL] = -k;
                        }
                    } else {
                        /* literal value to h l */
                        emit(LXI, RH, m);
                        regv[RH] = HIGH(m);
                        regv[RL] = LOW(m);
                        return;
                    }
                } else if (m < 0) {
                    /* value stacked, so... */
                    ustack();
                    emit(POP, RH, 0);
                    if (prec[s] < 2)
                        emit(LD, RH, 0);
                    regv[RH] = -1;
                    regv[RL] = -1;
                } else {
                    /* literal value to h l */
                    emit(LXI, RH, m);
                    regv[RH] = m / 256;
                    regv[RL] = m % 256;
                    return;
                }
            }
            if (rasn[s] == 0)
                rasn[s] = REGPAIR(RH, RL);
            return;
        } else if (rasn[s] > 0)
            return;
        else {
            /* check for previously stacked value */
            if (st[s] != 0 || litv[s] >= 0) {
                /* no registers assigned.  allocate registers AND load value. */
                i = prec[s];
                if (typ == 3) {
                    /* force a double byte load */
                    i   = 2;
                    typ = 0;
                }
                genreg(i, &ia, &ib);

                /* ia is low order byte, ib is high order byte. */
                if (ia <= 0) {
                    fatal("112: register allocation error");
                    return;
                }
            } else {
                genreg(2, &k, &i);
                /* check to ensure the stack is in good shape */
                i = s + 1;
                for (;;) {
                    if (i <= sp) {
                        if (st[i] == 0 && rasn[i] == 0 && litv[i] < 0)

                            /* found another stacked value */
                            error("147: stack not empty at end of compilation. Register stack "
                                  "order is invalid");
                        i = i + 1;
                    } else {
                        /* available cpu register is based at k */
                        emit(POP, k - 1, 0);
                        regs[k] = s;
                        if (prec[sp] >= 2) {
                            regs[k - 1] = s;
                            k           = REGPAIR(k - 1, k);
                        }
                        rasn[s] = k;
                        // V4
                        if (typ == 1) {
                            if (regs[RA])
                                emit(LD, regs[RA], RA);
                            emit(LD, RA, k);
                        }

                        /* decrement the stack count for this level */
                        ustack();
                        break;
                    }
                }
                return;
            }
        }
    }

    /* check for literal value (IN arith exp) */
    l = litv[s];
    if (l >= 0 && l <= 65535) {
        // V4
        litv[s]  = -1;
        lp       = l % 256;
        regs[ia] = s;
        regv[ia] = lp;
        if (typ == 1) {
            /* check for pending register store */
            jp = regs[RA];
            if (jp != 0) {
                /* store ACC into register before continuing */
                emit(LD, jp, RA);
                regs[RA] = 0;
            }
            if (lp == 0)
                emit(XR, RA, 0);
            if (lp != 0)
                emit(LD, RA, -lp);
            if (ib != 0) {
                emit(LD, ib, -l / 256);
                regs[ib] = s;
                regv[ib] = -l;
            }
        } else {
            /* typ = 0, load directly into registers */
            /* may be possible to LXI */
            ib &= 7; // avoid compiler warning
            if (ib != ia - 1) {
                emit(LD, ia, -lp);
                if (ib != 0) {
                    emit(LD, ib, -l / 256);
                    regs[ib] = s;
                    regv[ib] = -l;
                }
            } else {
                emit(LXI, ib, l);
                regs[ib] = s;
                regv[ib] = -l;
            }
        }
        rasn[s] = REGPAIR(ib, ia);
    } else {
        /* otherwise fetch from memory */
        sp = sp + 1;
        j  = st[s];
        setadr(j);
        litadd(sp);

        /* address of variable is IN h AND l */
        jp = typ + 1;
        switch (jp) {
        case 1:

            /* call from gensto (typ = 0) */
            emit(LD, ia, ME);
            break;
        case 2:

            /* call from apply to load value of variable */
            jp = regs[RA];

            /* check for pending register store */
            if (jp != 0) {
                /* have to store ACC into register before reloading */
                emit(LD, jp, RA);
                regs[RA] = 0;
            }
            emit(LD, RA, ME);
            break;
        case 3: // goto removed
            break;
        }

        /* check for double byte variable */
        if (typ != 2 && i > 1) { // avoids goto when typ == 2
            /* load high order byte */
            emit(IN, RL, 0);
            regv[RL] = regv[RL] + 1;
            emit(LD, ib, ME);
        }

        /* value is now loaded */
        _delete(1);
        if (typ != 2) {
            rasn[s]  = REGPAIR(ib, ia);
            regs[ia] = s;
            regv[ia] = -1;
            if (ib) {
                regs[ib] = s;
                regv[ib] = -1;
            }
        }
    }
}

void setadr(const int val) { // set top of stack to address reference
    alter = true;
    if (sp > maxsp) {
        fatal("113: register allocation stack overflow.");
        sp = 1;
    } else { // mark as address reference
        st[sp]   = -val;
        prec[sp] = INFO_PREC(abs(symAttrib(val)));
        litv[sp] = symAddr(val) < 0 ? (-symAddr(val) & 0xffff) : 0x10000 + (symAddr(val) & 0xffff);
        rasn[sp] = 0;
    }
}

void ustack() {
    /* decrement curdep AND check for underflow */
    if (--curdep[prsp] < 0) {
        curdep[prsp] = 0;
        error("148: pass-2 compiler error. Attempt to unstack too many values");
    }
}

int chain(const int sy, const int loc) {
    /* chain in double-byte refs to symbol sy, if necessary */
    if (symAddr(sy) < 0)
        return -symAddr(sy) & 0xffff; // absolute address already assigned

    int _chain = symRef(sy);
    symRef(sy) = loc;
    return _chain;
}

void gensto(const int keep) {
    int l, i1, i2, lp, jp, i, j, k;
    static int iq;

    /* keep = 0 if STD, keep = 1 if STO (value retained) */
    /* generate a store into the address at stack top */
    /* load value if NOT literal */
    l = litv[sp - 1];
    if (l < 0)
        loadv(sp - 1, iq = 0);

    i1 = rasn[sp - 1];
    i2 = REGLOW(i1);
    i1 /= 16;

    /* check for pending register store */
    jp = regs[RA];
    if (jp) {
        if (jp == i1)
            i1 = RA;
        if (jp == i2)
            i2 = RA;
    }

    do {                    // used to change goto L230 into break
        if (-st[sp] != 6) { /* ** note that this assumes 'stackptr' is at 6 IN sym tab */
            if (litv[sp] < 0) {
                bool skipUnlockHL = false; // used to eliminate goto L220
                do {                       // used change goto L210 & L220 to break
                    i = rasn[sp];
                    if (i <= 0 &&
                        st[sp] == 0) {    /* registers NOT allocated - check for stacked value */
                        emit(POP, RH, 0); // address is stacked so POP to h AND l
                        ustack();
                        break;
                    } else if ((i = st[sp]) <=
                               intbas) { /* check for ref to simple based variable */
                        if (i2 != 0)
                            lock[i2] = true;
                        if (i1 != 0)
                            lock[i1] = true;

                        loadv(sp, 3); /* force a double byte fetch into gprs */
                        i = rasn[sp];
                    } else { /* may be able to simplify (OR eliminate) the LHLD */
                        k  = regv[RH];
                        lp = regv[RL];
                        if (k != -3 || (-lp) != i) {
                            if (k == (-4) && (-lp) == i) {
                                emit(DCX, RH, 0);
                                regv[RH] = -3;
                            } else {
                                j = chain(i, codloc + 1);
                                emit(LHLD, j, 0);
                                regv[RH] = -3;
                                regv[RL] = -i;
                            }
                        }
                        skipUnlockHL = true;
                        break;
                    }
                    jp = regs[RA];
                    j  = REGLOW(i);
                    i /= 16; // PMO - might be able to use REGHIGH
                    if (i2 == 0 || i != j - 1) {
                        if (j == jp)
                            j = RA;
                        if (i == jp)
                            i = RA;
                        if (i != RD || j != RE) {
                            emit(LD, RL, j);
                            emit(LD, RH, i);
                            break;
                        }
                    } else if (i != RD ||
                               lastex != codloc - 1) { /* if prevous syllable is XCHG then do
                                                          another - peep will fix it */
                        if (i2 != RA) {                /* use STAX - set up accumulator */
                            if (jp != 0)
                                emit(LD, jp, RA);
                            if (i1 == RA)
                                i1 = jp;
                            emit(LD, RA, i2);
                            regs[RA] = 0;
                        }
                        emit(STAX, i, 0);
                        if (prec[sp] >= 2) { /* if byte dest we are done */
                            emit(INCX, i, 0);
                            if (i1 != 0) { /* store high order byte */
                                if (i2 == 1 && keep != 0) {
                                    emit(LD, REGLOW(rasn[sp - 1]), RA);
                                    regs[RA] = 0;
                                }
                                emit(LD, RA, i1);
                                emit(STAX, i, 0);
                            } else {                      /* store high order ZERO */
                                if (i2 == 1 && keep != 0) // V4 fix
                                    emit(LD, REGLOW(rasn[sp - 1]), RA);
                                regs[RA] = 0;
                                emit(XR, RA, 0);
                                emit(STAX, i, 0);
                            }
                        }
                        _delete(1);
                        return;
                    }
                    emit(XCHG, 0, 0);
                } while (false);
                if (!skipUnlockHL) {
                    if (i1 != 0)
                        lock[i1] = false;
                    if (i2 != 0)
                        lock[i2] = false;
                    regv[RH] = regv[RL] = -1;
                }
            } else if (i1 != RD || i2 != RE || lastex != codloc - 1 ||
                       prec[sp] != 2) /* otherwise this is a literal address */
                litadd(sp);           /* if possible, generate a SHLD */
            else {
                emit(XCHG, 0, 0);
                i = abs(st[sp]);
                j = chain(i, codloc + 1);
                emit(SHLD, j, 0);
                regv[RH] = -3;
                regv[RL] = -i;
                if (keep != 0)
                    emit(XCHG, 0, 0);
                break;
            }

            /* we may change mov r,m inr r mov m,r to inr m. */
            /* if so, AND this is a non-destructive store, the register */
            /* assignment must be released. */
            iq = lastIncReg;

            /* generate low order byte store */
            emit(LD, ME, i2 != 0 ? i2 : -abs(l) % 256); /* check for immediate store */
            if (prec[sp] != 1) {                        /* now store high order byte (if any) */
                emit(INCX, RH, 0);                      /* store second byte */
                /* regv(RH) = -3 then LHLD has occurred on symbol -regv(RL) */
                /* regv(RH) = -4 then LHLD AND INCX h has occurred */
                j = regv[RH];
                if (j >= 0)
                    regv[RL]++;
                else {
                    regv[RH] = -4;
                    if (j != -3) /* RH AND RL have unknown values */
                        regv[RH] = regv[RL] = -1;
                }
                if (prec[sp - 1] >= 2) {
                    if (i1 == 0) /* second byte is literal */
                        emit(LD, ME, -abs(l / 256));
                    else /* LD memory from register */
                        emit(LD, ME, i1);
                } else
                    emit(LD, ME, 0);
            }
        } else if (i2 == 0) /* load sp immediate */
            emit(LXI, RSP, l);
        else {
            emit(LD, RL, i2);
            emit(LD, RH, i1);
            emit(SPHL, 0, 0);
            regv[RH] = regv[RL] = -1;
        }
    } while (false);

    /* now release register containing address */
    /* release register assignment for value */
    /* if mov r,m inr r mov m,r was changed to inr m. */
    if (iq != codloc)
        _delete(1);
    else {
        i = -st[sp];
        _delete(2);
        st[++sp] = i;
        rasn[sp] = 0;
        prec[sp] = 1;
        litv[sp] = -1;
    }
}

void litadd(const int s) {
    int ih, il, ir, kp, it, lp, l, jp, i, j, k;
    /* load h AND l with the address of the variable at s IN */
    /* the stack */
    il = litv[s] % 256;
    ih = litv[s] / 256;
    ir = RH;
    l  = ih;

    if (ih < 0)
        error("114: pass-2 compiler error in 'litadd'");
    else if ((i = rasn[s]) != REGPAIR(RH, RL)) { /* deassign registers */
        jp = regs[RA];
        if ((k = REGLOW(i))) {
            if (k == jp)
                regs[RA] = 0;
            regs[k] = 0;
            lock[k] = false;
            regv[k] = -1;
        }
        if ((k = REGHIGH(i))) {
            if (k == jp)
                regs[RA] = 0;
            regs[k] = 0;
            lock[k] = false;
            regv[k] = -1;
        }
        rasn[s] = 0;
        for (int i = RH; i <= RL; i++) {
            if ((j = regs[i])) {
                kp      = REGLOW(rasn[j]);
                k       = REGHIGH(rasn[j]);
                rasn[j] = REGPAIR(k == i ? 0 : k, kp == i ? 0 : kp);
            }
            if ((lp = regv[i]) != l) {
                if (lp == l + 1)
                    emit(DC, ir, 0);
                else if (lp == l - 1 && l != 0) {
                    emit(IN, ir, 0);
                } else if (i == 6 &&
                           il != regv[RL]) { /* no INC/dec possible, see if l does NOT match */
                    regv[RL] = il;
                    if (l <= 255) /* otherwise this is a real address */
                        emit(LXI, RH, il + ih * 256);
                    else { /* the LXI must be backstuffed later */
                        it = st[s];
                        if (it >= 0) {
                            error("115: pass-2 compiler error in 'litadd'");
                            return;
                        }
                        it = -it;
                        emit(LXI, RH, symRef(it)); /* place reference into chain */
                        symRef(it) = codloc - 2;
                    }
                } else if (l <= 255)
                    emit(LD, ir, -l);
                else if ((it = st[s]) >= 0) { /* the address must be backstuffed later */
                    error("115: pass-2 compiler error in 'litadd'");
                    return;
                } else {
                    it = -it;
                    if (symAddr(it) <= 0) {
                        error("116: pass-2 compiler error in 'litadd'");
                        break;
                    }
                    /* place link into code */
                    int target  = HIGHWORD(symAddr(it));
                    symAddr(it) = ((codloc + 1) << 16) + LOWWORD(symAddr(it));
                    emit(0, LOW(target), 0);
                    emit(0, HIGH(target), 0);
                }
            }
            /* fix values IN stack AND reg */
            if (i == RL)
                rasn[s] = REGPAIR(RH, RL);

            regs[i] = s;
            regv[i] = l;
            l       = il;
            ir      = RL;
        }
    }
}

/*
    simplified as only base 16 is used
    also uses pre decoded instructions to simplify the logic
 */
void dump(int lp, const int u, bool symbolic) {
    // items on line = (line width - address width) / (length of item + a space)
    int itemsOnLine = symbolic ? (C_WIDTH - 5) / 7 : (C_WIDTH - 5) / 4;

    if (itemsOnLine <= 0)
        error("117: line width set too narrow for code dump");

    else {
        uint16_t accum[33];

        for (int i = 0; i < 29; i++) // initialise line to no values
            accum[i] = 256;
        int nsame = 0;
        int opcnt = 0;
        while (lp <= u) {
            bool same     = true;
            int lineStart = lp;
            int nItems;
            for (nItems = 0; nItems < itemsOnLine; nItems++) {
                if (lp > u) { // data finishes mid line
                    same = false;
                    break;
                } else {
                    int j = get(lp++);        // check if same
                    if (j != accum[nItems]) { // if not mark and update value
                        same          = false;
                        accum[nItems] = j;
                    }
                }
            }
            if (same) {
                if (nsame++ == 0) // blank line for repeat lines
                    putc('\n', lstFp);
            } else if (nItems == 0) // only happens if no more data
                return;
            else { // non same line
                nsame = 0;
                fprintf(lstFp, "%04XH", lineStart); // print the address line
                for (int item = 0; item < nItems; item++) {
                    if (symbolic) {
                        int width;
                        if (opcnt-- > 0)
                            width = fprintf(lstFp, " %02XH", accum[item]);
                        else {
                            width = fprintf(lstFp, " %s", ctran[accum[item]].opcode);
                            opcnt = ctran[accum[item]].extra;
                        }
                        if (item != nItems - 1)
                            fprintf(lstFp, "%*s", 7 - width, "");
                    } else
                        fprintf(lstFp, " %02XH", accum[item]);
                }
                putc('\n', lstFp);
            }
        }
    }
}

/* STA    011    000    LDA    011    000    XCHG   SPHL   PCHL*/
/* CMA    STC    CMC    DAA    SHLD   011    000    LHLD   011*/
/* 000    EI     DI     LXI b  011    000    PUSH b POP b  DAD b*/
/* STAX b LDAX b INX b  DCX b  NOP    NOP    NOP    NOP    NOP*/
/* 050 011 000 058 011 000 235 249 233 047 055 063 039 034 011 000*/
/* 042 011 000 251 243 001 011 000 197 193 009 002 010 003 011 000*/

unsigned char cbits[43 + 1] = { ZPAD, 0x40, 4,   5,   0x80, 136, 144, 152, 160, 168, 176,
                                184,  7,    195, 194, 205,  196, 201, 192, 199, 219, 211,
                                118,  50,   58,  235, 249,  233, 47,  55,  63,  39,  34,
                                42,   251,  243, 1,   197,  193, 9,   2,   10,  3,   11 };
unsigned char cc[2][4]      = { { 0x10, 0x00, 0x30, 0x20 }, { 0x18, 0x08, 0x38, 0x28 } };
// fortan used bits(3)
// bits(1) -> opcode
// bits(2) -> operand low
// bits(3) -> operand high

/* the following comments are sample calls to the emit */
/* routine.  note that emit requires three argument at all times */
/* (the unused arguments are ZERO). */

/* call emit(LD,RA,RB) */
/* call emit(LD,RC,-34) */
/* call emit(LD,RD,ME) */
/* call emit(LD,ME,RE) */
/* call emit(IN,RH,0) */
/* call emit(DC,RL,0) */
/* call emit(AD,RB,0) */
/* call emit(AD,ME,0) */
/* call emit(AD,-5,0) */
/* call emit(SU,RB,0) */
/* call emit(SB,ME,0) */
/* call emit(ND,-5,0) */
/* call emit(XR,0,0) */
/* call emit(OR,RB,0) */
/* call emit(CP,RH,0) */
/* call emit(ROT,ACC,LFT) */
/* call emit(ROT,CY,LFT) */
/* call emit(ROT,CY,RGT) */
/* call emit(JMP,148,0) */
/* call emit(JMC,TRU*32+ZERO,148) */
/* call emit(CAL,1048,0) */
/* call emit(CLC,FAL*32+PARITY,148) */
/* call emit(RTN,0,0) */
/* call emit(RTC,FAL*32+CARRY,255) */
/* call emit(RST,3,0) */
/* call emit(INP,6,0) */
/* call emit(OUT,10,0) */
/* call emit(HALT,0,0) */
/* emit a literal between 0 AND 255 */
/* call emit(0,44,0) */
/* call emit(STA,300,0) */
/* call emit(LDA,300,0) */
/* call emit(XCHG,0,0) */
/* call emit(SPHL,0,0) */
/* call emit(PCHL,0,0) */
/* call emit(CMA,0,0) */
/* call emit(STC,0,0) */
/* call emit(CMC,0,0) */
/* call emit(DAA,0,0) */
/* call emit(SHLD,300,0) */
/* call emit(LHLD,300,0) */
/* call emit(EI,0,0) */
/* call emit(DI,0,0) */
/* call emit(LXI,(RB,RD,RH,RSP),300) */
/* call emit(PUSH,(RB,RD,RH,RA),0) */
/* call emit(POP,(RB,RD,RH,RA),0) */
/* call emit(DAD,(RB,RD,RH,RSP),0) */
/* call emit(STAX,(RB,RD),0) */
/* call emit(LDAX,(RB,RD),0) */
/* call emit(INX,(RB,RD,RH,RSP),0) */
/* call emit(DCX,(RB,RD,RH,RSP),0) */

void emit(const int opr, const int opa, const int opb) {
    int opcode, operand, n, i;

    n = 1;
    if (C_NUMERIC) /* write emitter trace */
        fprintf(lstFp, "E(%d,%d,%d)\n", opr, opa, opb);

    if (opr <= 0)
        opcode = opa;

    else {
        opcode = cbits[opr];
        switch (opr) {
        case LD:            /* mov group */
            if (opb <= 0) { /* lri operation */
                if (v4Opt && lastLoadImm == codloc - 2 && lastRegImm / 2 == opa / 2 &&
                    lastRegImm < 8 && lastRegImm > 1) {
                    // here we have a lri operation following a lri to the other
                    // register in this pair (b/c,d/e,h/l) or to the same reg.
                    if (lastRegImm == opa) { // here to same register
                        put(codloc - 1, -opb);
                        return;
                    }
                    // here to the other register in the pair
                    // we have to change the opcode to a lxi of the high reg (b,d,h)
                    // also we will only add 1 word and we can clear lastli
                    opcode =  1 + (opa / 2 - 1) * 16; // replace mvi with the lxi instruction
                    put(codloc - 2, opcode);
                    n           = 1;
                    lastLoadImm = 0;
                    if (opa % 2 == 0) {
                        // here the low register (c,e,l) was already there so we just add
                        // the high reg.
                        opcode = -opb;
                    } else {
                        // here the high register (b,d,h) was already there so we need to
                        // insert the low register between the opcode and it.
                        opcode = get(codloc - 1);
                        put(codloc - 1, -opb);
                    }
                } else { // here the lri didn-t follow a lri that we could change
                    lastLoadImm = codloc;
                    lastRegImm  = opa;

                    n           = 2;                     /* 2 byte instruction */
                    opcode      = 0x6 + regmap[opa] * 8; /* gen MVI instruction */
                    operand     = -opb;                  /* and the value */
                }
            } else {
                /* check for possible load register elimination */
                /* is this a lmr OR lrm instruction... */
                // if mov m,r followed by mov r,m then optimise 2nd istruction away
                if (opa != ME) {
                    if (opb == ME && lastLoad == codloc - 1 && lastReg == opa)
                        return;

                    /* may change a mov r,m inr/dcr r mov m,r to inr/dcr m */
                } else if (lastIncReg == codloc - 1) {
                    // other code has flagged this is inc of same register
                    i = (get(codloc - 1) & 7) + 0x30; // generate the inr/dcr

                    /* the register load may have been eliminated... */
                    if (lastLoad != codloc - 2 || opb != lastReg)
                        codloc--;
                    put(codloc - 1, i);
                    lastIncReg = 0; // prevent further attempts to optimise
                    lastReg    = 0;
                    lastLoad   = 0;
                    if (lastin == codloc || lastin == codloc + 1)
                        lastin = codloc - 1;
                    return;
                } else {
                    lastLoad = codloc;
                    lastReg  = opb;
                }
                /* this is a load memory from register operation - save */

                opcode += regmap[opa] * 8 + regmap[opb];
            }
            break;
        case IN:
        case DC:                       /* inr & dcr */
            opcode += regmap[opa] * 8; /* gen inr / dcr */
            break;
            /* Arith group */
        case AD:
        case AC:
        case SU:
        case SB:
        case ND:
        case XR:
        case OR:
        case CP:
            if (opa > 0)
                opcode += regmap[opa];
            else { /* immediate operand */
                n = 2;  // normally 2 byte code
                if (v4Opt && opa == 0) {
                    switch (opr) {
                    case AD:
                    case SU:
                    case XR:
                    case OR:
                        // replace adi,0 sui,0 xri,0 or ori 0 with ora,a
                        opcode = cbits[OR] + 7;
                        n      = 1;
                        break;
                    case ND: // replace ani,0 with xra,a
                        opcode = cbits[XR] + 7;
                        n      = 1;
                        break;
                    }
                }
                if (n == 2) {   // make the 2 byte code
                    opcode += 0x46; /* gen imm arith instruction */
                    operand = -opa; /* the immediate value*/
                }
            }
            break;
        case ROT: /* rotate group */
            i = (opa - CY) * 2 + (opb - LFT);
            opcode += i * 8;
            break;

        case JMC:
        case CLC: /* condiitonal jmp & call */
            n       = 3;
            operand = opb;
            /* fall through to common cc adjustment */
        case RTC: /* conditional return */
            opcode += cc[(opa / 32 - FAL) & 1][(opa % 32 - CARRY) & 3];
            break;
        case RST: /* rst xx */
            opcode += opa % 8 * 8;
            break;
        case INP:
        case OUT: /* in & out */
            n       = 2;
            operand = opa;
            break;
        case JMP:
        case CAL: /* jmp & call */
        case STA:
        case LDA:
        case SHLD:
        case LHLD: /* STA LDA SHLD LHLD */
            n       = 3;
            operand = opa;
            break;
        case XCHG:
            if (lastex == codloc - 1) { /* remove double xchg*/
                codloc--;
                lastex = 0;
                return;
            }
            lastex = codloc; /* mark this xchg */
            break;
        case RTN:
        case HALT: /* ret & halt */
        case SPHL:
        case PCHL:
        case CMA:
        case STC:
        case CMC:
        case DAA:
        case EI:
        case DI:
            break;
        case LXI:                      /* LXI (get immediate part) */
            n = 3;                     /* 3 byte instruction */
            opcode += regmap[opa] * 8; /* merge in the register */
            operand = opb;             /* and the location */
            break;
        case PUSH:
            /* PUSH r - check for XCHG PUSH d combination. change to PUSH h */
            // PMO, relies on no code being generated that uses existing DE or HL after this
            if (lastex == codloc - 1 && opa == RD) {
                codloc--;
                lastex = 0;
                opcode += regmap[RH] * 8;
                break;
            }
            /* else drop through... */
        case POP:
        case DAD:
        case STAX:
        case LDAX:
        case INCX:
        case DCX:
            /* adjust for push/pop psw */
            i = opa == RA ? 6 : regmap[opa];
            opcode += i * 8;
        }
    }
    put(codloc++, opcode);
    if (n > 1) {
        put(codloc++, operand % 256);
        if (n > 2)
            put(codloc++, operand / 256);
    }
    return;
}

// BPNF support removed
void puncod(int start, const int end, const int mode) {
    unsigned char crc;

    /* punch code from lower bound (lb) to upper bound (ub) */
    /* mode = 1  - - punch header only */
    /* mode = 2 - - punch trailer only */
    /* mode = 3 - - V2 punch header AND trailer , V4 punch code only */
    if (mode == 2) {
        // write end of file record
        int entry = offset == 0 && C_STACKHANDLING != 1 ? 0 : offset + preamb;
        crc       = -(1 + entry / 256 + entry % 256);
        fprintf(hexFp, ":00%04X01%02X\n", entry, crc);
    } else {
        int toWrite;
        while ((toWrite = end - start + 1) > 0) {
            if (toWrite > 16)
                toWrite = 16;

            fprintf(hexFp, ":%02X%04X00", toWrite, start);
            crc = -(toWrite + start % 256 + start / 256);
            for (int i = 0; i < toWrite; i++, start++) {
                int val = get(start);
                crc -= val;
                fprintf(hexFp, "%02X", val);
            }
            fprintf(hexFp, "%02X\n", crc);
        }
    }
}

void cvcond(const int s) {
    int i, j, ia, jp;

    /* convert the condition code at s IN the stack to a boolean value */
    i        = rasn[s];
    ia       = i & 0xf;
    int test = (i >> 8) & 0xf;
    j        = (i >> 12) & 0xf;

    /* j = 1 if true , j = 0 if false */

    /* k = 1 if CARRY, 2 if ZERO, 3 if SIGN, AND 4 if PARITY */

    /* we may generate a short sequence */
    if (test <= 2 && ia != 0 && regs[RA] == ia) {
        if (test != 2) {
            /* short conversion for true OR false CARRY */
            emit(SB, RA, 0);
            if (j == 0)
                emit(CMA, 0, 0);
        } else {
            /* short conversion for true OR false ZERO */
            if (j == 0)
                emit(AD, -255, 0);
            if (j == 1)
                emit(SU, -1, 0);
            emit(SB, RA, 0);
        }
    } else {
        /* do we have to assign a register */
        if (ia == 0) {
            genreg(1, &ia, &jp);
            if (ia != 0) {
                regs[ia] = sp;
                i        = ia;
            } else {
                fatal("118: register allocation error");
                return;
            }
        }

        /* check pending register store */
        jp = regs[RA];
        if (jp != 0)
            if (jp != ia) {
                emit(LD, jp, RA);
                regs[RA] = 0;
            }
        emit(LD, RA, -255);
        j = (FAL + j) * 32 + (CARRY + test - 1);
        emit(JMC, j, codloc + 4);
        emit(XR, RA, 0);
    }

    /* set up pending register store */
    regs[RA] = ia;
    rasn[s]  = i & 0xff;
    return;
}

void saver() {
    int byteCnt, wordCnt, byteChain, wordChain, i;

    /* save the active registers AND reset tables */
    /* first determine the stack elements which must be saved */

    if (sp != 0) {
        wordChain = byteChain = wordCnt = byteCnt = 0;
        for (int j = 1; j <= sp; j++) {
            int regPair = rasn[j];
            if (regPair > 255) {
                cvcond(j);
                regPair = rasn[j];
            }
            if (regPair >= 16) {
                /* double byte */
                if (!(lock[REGLOW(regPair)] || lock[REGHIGH(regPair)])) {
                    st[j]     = wordChain;
                    wordChain = j;
                    wordCnt++;
                }
            } else if (regPair > 0 && !lock[REGLOW(regPair)]) { /* single byte */
                st[j]     = byteChain;
                byteChain = j;
                byteCnt++;
            }
        }
        lmem -= byteCnt + wordCnt * 2;
        if (byteCnt == 0 && wordCnt != 0 && lmem % 2)
            lmem--;

        /* lmem is now properly aligned. */
        if (lmem < 0) {
            error("119: memory allocation error");
            return;
        } else {
            int loc = lmem;
            while (byteChain + wordChain) {
                if (loc % 2 || wordChain == 0) /* single byte */
                    byteChain = st[i = byteChain];
                else /* even byte boundary with double bytes to store */
                    wordChain = st[i = wordChain];

                if (i <= 0) {
                    error("120: memory allocation error");
                    return;
                }

                /* place temporary into symbol table */
                st[i]          = ++sytop;
                symbol[sytop]  = syinfo;

                symbol[syinfo] = loc;
                loc += rasn[i] >= 16 ? 2 : 1;

                symbol[--syinfo] = 256 + VARB + (rasn[i] >= 16 ? 32 : 16);

                /* length is 1*256 */

                /* leave room for LXI chain */
                symbol[--syinfo] = 0;
                if (sytop > --syinfo) {
                    fatal("121: pass-2 symbol table overflow");
                    return;
                }

                /* store into memory */
                int regPair = rasn[i];
                rasn[i]     = 0;
                sp++;
                setadr(sytop);
                litadd(sp);
                while (regPair) {
                    i = REGLOW(regPair);
                    if (i == regs[RA]) {
                        i        = RA;
                        regs[RA] = 0;
                        regv[RA] = -1;
                    }
                    emit(LD, ME, i);
                    if ((regPair = REGHIGH(regPair))) { /* double byte store */
                        emit(IN, RL, 0);
                        regv[RL]++;
                    }
                }
                _delete(1);
            }
        }
    }
    for (int i = 2; i <= 7; i++)
        if (!lock[i]) {
            regs[i] = 0;
            regv[i] = -1;
        }
    return;
}

void reloc() {
    int stloc;
    int stsize;
    int i, j, k, l, n, ip;
    if (C_SYMBOLS >= 2) {
        for (int i = 1; i <= sytop; i++)
            fprintf(lstFp, "%4d = %6d\n", i, symbol[i]);
        for (int i = syinfo; i <= symax; i++)
            fprintf(lstFp, "%5d = %c%08XH\n", i, (symbol[i] < 0) ? '-' : ' ', abs(symbol[i]));
    }

    /* compute max stack depth required for correct execution */
    stsize = maxdep[0];
    for (int n = 0; n < 8; n++) {
        if (intpro[n]) /* get interrupt procedure depth */
            /* note that i exceeds depth by 1 since RET may be pending */
            stsize += symIProcDepth(intpro[n]) + 1;
    }
    stsize *= 2;
    n = stsize;
    if (C_STACKHANDLING != 0)
        n = 0;

    /* align to even boundary, if necessary */
    if (n != 0 && lmem % 2 == 1)
        lmem--;
    stloc = lmem;
    lmem -= n;

    /* stsize is number of bytes reqd for stack, stloc is addr */
    // iw = C_WIDTH / 14;   not used
    n = 0;

    /* compute page to start variables */
    i = 0;
    if (codloc % 256 > lmem % 256)
        i = 1;
    i += codloc / 256;
    if (C_VARIABLES > i)
        i = C_VARIABLES;

    /* compute first relative address page */
    j = lmem / 256 - i;
    if (j < 0)
        error("122: program requires too much program and variable storage");

    else {
        for (int i = 1; i <= sytop; i++) {
            k = symAddr(i);
            if (k >= 0) {                 /* now fix page number */
                l          = HIGH(k) - j; /* l is relocated page number */
                symAddr(i) = (l << 8) + LOW(k);
                int target = HIGHWORD(k);
                for (;;) {
                    if (target) { /* backstuff lhi l into location target-1 */
                        ip = get(target - 1) * 256 + get(target);
                        put(target - 1, 0x26);
                        put(target, l);
                        target = ip;
                    } else { /* backstuff LXI references to this variable */
                        target = symAddr(i);
                        for (int link = symRef(i); link;) {
                            int next = getword(link);
                            putword(link, target);
                            link = next;
                        }
                        break;
                    }
                }
            }
        }
        if (C_MAP)
            putc('\n', lstFp);

        /* relocate AND backstuff the stack top references */
        stloc -= j * 256;
        while (lxis != 0) {
            i    = lxis;
            lxis = getword(i);
            putword(i, stloc);
        }
        if (C_STACKHANDLING == 1)
            fprintf(lstFp, "STACK SIZE OVERRIDDEN\n");
        else
            fprintf(lstFp, "STACK SIZE = %d BYTES\n", stsize);

        /* now backstuff all other TRC, TRA, AND PRO addresses */
        for (int i = 1; i <= sytop; i++) {
            k = -symAddr(i);
            l = INFO_TYPE(abs(symAttrib(i)));
            if (l == LABEL || l == PROC) {
                int n       = k & 3;
                int addr    = k / 4; // does right thing if k -1 .. -3
                int backref = symRef(i);
                while (addr) { // V4
                    int next = getword(addr);
                    putword(addr, backref);
                    addr = next;
                }
                symAddr(i) = n;
            }
        }
        if (preamb > 0) {
            for (int i = 0; i < 8; i++)
                if (intpro[i])
                    intpro[i] = symRef(intpro[i]) * 256 + 0xc3;
            if (intpro[0] == 0 && offset == 0 && C_STACKHANDLING != 1) // V4
                intpro[0] = (offset + preamb) * 256 + 0xc3;

            k      = offset;
            offset = 0;
            for (int i = 0, j = 0, l; i < preamb; i++) {
                if (i % 8 == 0)
                    l = intpro[j++];
                put(i, l % 256);
                l /= 256;
            }
            offset = k;
            return;
        }
    }
    return;
}

void put2(int ch1, int ch2, int prec) {
    if (prec == 2) {
        put(codloc++, ch2);
        put(codloc++, ch1);
    } else {
        put(codloc++, ch1);
        put(codloc++, ch2);
    }
}

void put1(int ch, int prec) {
    put(codloc++, ch);
    if (prec == 2)
        put(codloc++, 0);
}

int loadin() // modified for V4
{
    int addr   = -1;
    int marker = getc(symFp);
    while (marker == 4) {

        // we have a new block
        int symNo = getSym16();
        int prec  = INFO_PREC(symAttrib(symNo));
        int sAddr = symAddr(symNo);
        if (codloc > sAddr)
            error("123: initialized storage overlaps previously initialized storage at %04XH",
                  sAddr);
        while (codloc < sAddr)
            put(codloc++, 0);
        if (addr < 0)
            addr = sAddr;
        // load in data
        while ((marker = getc(symFp)) >= 1 && marker <= 3) {
            int ch1 = getc(symFp);
            switch (marker) {
            case 1:
                put1(ch1, prec);
                break;
            case 2:
                put2(ch1, getc(symFp), prec);
                break;
            case 3:
                while (ch1 > 0) {
                    int ch2;
                    if ((ch2 = getc(symFp)) <= 0) {
                        put1(ch1, prec);
                        break;
                    }
                    put2(ch1, ch2, prec);
                    ch1 = getc(symFp);
                }
                break;
            }
        }
    }
    if (marker != 0)
        error("124: initialization table format error");
    return addr;
}

void emitbf(int bf) {

    // modified code to emit a call to the builtin in fuction bf
    // bf - 0->multiply 1->divide
    // emits routine in line if not yet done so.
    // biftab[bf] is zero if code not emitted, else location of routine

    if (!biftab[bf]) {
        emit(JMP, 0, 0);     // jump around the code - back stuffed later
        biftab[bf] = codloc; // update to record location of routine

        for (uint8_t *codePtr = bf == 0 ? multiply : divide; *codePtr; codePtr++) {
            if (*codePtr == 0xff) { // relocatable address
                int target = biftab[bf] + *++codePtr;
                put(codloc++, LOW(target));
                put(codloc++, HIGH(target));
            } else {
                for (int i = *codePtr; i != 0; i--)
                    put(codloc++, *++codePtr);
            }
        }
        /* backstuff branch around function */
        putword(biftab[bf] - 2, codloc);
    }

    /* emit call on the function */
    emit(CAL, biftab[bf], 0);
    return;
}

int getNextPol() {
    uint16_t pol;
    if (fread(&pol, sizeof(pol), 1, polFp) != 1)
        return EOF;
    return pol;
}

void inldat() {
    int ic   = 0; // assign to appease GCC
    int type = 0;
    int val  = 0;

    /* emit data inline */
    int iq  = codloc;
    int cnt = 0;
    for (;;) {
        while (lapol != 0) {
            val  = lapol / 8;
            type = lapol % 8;
            if ((lapol = getNextPol()) == EOF) {
                lapol = 0;
                type  = 0;
                break;
            }
            /* kp is typ AND k is data */
            if (cnt > 0) {
                if (type == OPR && val == DAT) {
                    /* backstuff jump address */
                    /* now fix symbol table entries */
                    symAddr(abs(ic)) = -iq;
                    int j            = INFO_ECNT(symAttrib(abs(ic)));
                    /* check symbol length against count */
                    symAttrib(abs(ic)) = PACK_ATTRIB(--cnt, 1, VARB);
                    if (ic < 0) { /* this is an address reference to a constant, so.. */
                        st[++sp] = ic;
                        rasn[sp] = 0;
                        litv[sp] = iq;
                        prec[sp] = 2;
                    } else if (j != cnt) /* check size declared against size read */
                        break;
                    return;
                } else if (type == LIT)
                    emit(0, val, 0);
                else
                    break;
            } else { /* define inline data symbol */
                if (type != DEF)
                    break;
                ic = val;
                if (val <= 0) { /* inline constant -- set up symbol entry */
                    ic            = -(++sytop);
                    symbol[sytop] = syinfo;
                    syinfo -= 2;

                    /* will be filled later */
                    if (syinfo < sytop)
                        break;
                }
            }
            cnt++;
        }

        if (type == LIN)
            C_COUNT = val;
        else {
            error("125: inline data error");
            return;
        }
    }
}

void unary(const int val) {
    int i, j, k, ip, ia, ib, jp, iq;

    /* 'val' is an integer corresponding to the operations-- */
    /* RTL(1) RTR(2) SFL(3) SFR(4) scl(5) scr(6) HIV(7) LOV(8) */
    if (rasn[sp] > 255)
        cvcond(sp);
    ip = prec[sp];
    switch (val) {
    case B_ROL:
    case B_ROR:
        if (ip <= 1) {
            if (rasn[sp] == 0) {
                loadv(sp, 1);
                regs[RA] = REGLOW(rasn[sp]);
            }
            i = REGLOW(rasn[sp]);
            k = regs[RA];
            if (k != 0) {
                if (k != i) {
                    emit(LD, k, RA);
                    emit(LD, RA, i);
                    regs[RA] = i;
                }
            } else {
                emit(LD, RA, i);
                regs[RA] = i;
            }
            i = LFT;
            if (val == B_ROR)
                i = RGT;
            emit(ROT, CY, i);
            return;
        }
        break;
    case B_SHL:
    case B_SHR:
    case B_SCL: // SCL
    case B_SCR: // SCR
        j = 1;
        if ((val == B_SHR || val == B_SCR) && ip > 1)
            j = 0;
        i = rasn[sp];
        if (i <= 0) {
            /* load from memory */
            loadv(sp, j);
            i = rasn[sp];
            if (j == 1)
                regs[RA] = REGLOW(i);
        }

        /* may have to store the accumulator */
        ia = REGLOW(i);
        ib = REGHIGH(i);
        k  = ia;
        if (j != 1)
            k = ib;
        jp = regs[RA];

        /* we want register k to be IN the accumulator */
        if (jp != k) {
            if (jp != 0)
                emit(LD, jp, RA);
            emit(LD, RA, k);
        }
        regs[RA] = k;

        /* SFL AND SFR take separate paths now... */
        if (val == B_SHR || val == B_SCR) {
            if (val == B_SHR)
                emit(OR, RA, 0);
            emit(ROT, ACC, RGT);
            if (ip >= 2) {
                emit(LD, ib, RA);
                emit(LD, RA, ia);
                emit(ROT, ACC, RGT);
                regs[RA] = ia;
            }
        } else {
            /* SFL - clear CARRY AND shift */
            if (val == B_SHL)
                emit(AD, RA, RA);
            if (val == B_SCL)
                emit(ROT, ACC, LFT);
            if (ip >= 2) {
                emit(LD, ia, RA);
                emit(LD, RA, ib);
                emit(ROT, ACC, LFT);
                regs[RA] = ib;
            }
        }
        return;
        break;
    case B_TIME:
        break;
    case B_HIGH:
        if (ip >= 2) {
            if (rasn[sp] <= 0)
                loadv(sp, 0);
            ip = REGHIGH(rasn[sp]);
            iq = REGLOW(rasn[sp]);
            if (regs[RA] == iq)
                regs[RA] = 0;
            regs[ip] = 0;
            regv[ip] = -1;
            rasn[sp] = iq;
            prec[sp] = 1;
            if (regs[RA] != ip)
                emit(LD, iq, ip);

            else
                regs[RA] = iq;
            return;
        }
        break;
    case B_LOW:
        prec[sp] = 1;
        /* may have to release register */
        if ((i = REGHIGH(rasn[sp]))) {
            regs[i] = 0;
            regv[i] = -1;
            if (regs[RA] == i)
                regs[RA] = 0;
        }
        rasn[sp] = REGLOW(rasn[sp]);
        return;
    }
    error("126: built-in function improperly called");
    return;
}

void exch() {
    int i, j, ia, ib;

    /* exchange the top two elements of the stack */
    j = sp - 1;
    if (st[j] == 0 && rasn[j] == 0 && litv[j] < 0)

        /* second element is pushed - check top elt */
        if (rasn[sp] == 0 && litv[sp] < 0)

            /* second elt is pushed, top elt is NOT IN cpu */
            if (st[sp] == 0) {
                /* both are pushed, so go thru 20 twice */
                j = sp;
                for (;;) {
                    /* POP element (second if drop thru, top if from 30) */
                    genreg(-1, &ia, &ib);
                    if (ia == 0) {
                        fatal("107: register allocation error. No registers available");
                        break;
                    } else {
                        if (prec[j] > 1)
                            ib = ia - 1;
                        emit(POP, ia - 1, 0);
                        ustack();
                        regs[ia] = j;
                        if (ib != 0)
                            regs[ib] = j;
                        rasn[j] = REGPAIR(ib, ia);
                        if (j != sp)
                            break;
                        j = sp - 1;
                    }
                }
            }
    j = sp - 1;
    for (int i = 2; i <= 7; i++)
        if (regs[i] == sp)
            regs[i] = j;

        else if (regs[i] == j)
            regs[i] = sp;
    i        = prec[sp];
    prec[sp] = prec[j];
    prec[j]  = i;
    i        = rasn[sp];
    rasn[sp] = rasn[j];
    rasn[j]  = i;
    i        = st[sp];
    st[sp]   = st[j];
    st[j]    = i;
    i        = litv[sp];
    litv[sp] = litv[j];
    litv[j]  = i;
    return;
}

void stack(const int n) {
    /* ADD n to current depth, test for stacksize exc maxdepth */
    curdep[prsp] += n;
    if (curdep[prsp] > maxdep[prsp])
        maxdep[prsp] = curdep[prsp];
    return;
}

void readcd() {
    int lcnt, typ, lline, lloc, polcnt, val, i, j, k, l;
    int ibase, ip, kp, lp, ia, ib;
    char *rmap       = "-ABCDEFGHIJKLMOP";
    char polchr[][4] = { "OPR", "ADR", "VAL", "DEF", "LIT", "LIN" };
    char opcval[][4] = { "NOP", "ADD", "ADC", "SUB", "SBC", "MUL", "DIV", "MDF", "NEG",
                         "AND", "IOR", "XOR", "NOT", "EQL", "LSS", "GTR", "NEQ", "LEQ",
                         "GEQ", "INX", "TRA", "TRC", "PRO", "RET", "STO", "STD", "XCH",
                         "DEL", "DAT", "LOD", "BIF", "INC", "CSE", "END", "ENB", "ENP",
                         "HAL", "RTL", "RTR", "SFL", "SFR", "HIV", "LOV", "CVA", "ORG",
                         "DRT", "ENA", "DIS", "AX1", "AX2", "AX3" };

    C_COUNT          = 1;
    lline            = 0;
    lloc             = 0;
    lcnt             = C_WIDTH / 12;
    alter            = false;

    polcnt           = 0;

    /* reserve space for interrupt locations */
    preamb = 0;
    for (int i = 7; i >= 0; i--) {
        if (intpro[i] != 0) {
            preamb = i * 8 + 3;
            break;
        }
    }

    /* adjust codloc to account for preamble */
    if (codloc < preamb)
        codloc = preamb;

    /* allocate 'preamble' cells at start of code */
    offset = codloc - preamb;

    /* set stack pointer upon program entry */
    if (C_STACKHANDLING != 1) {   /* 1 -> user handled */
        if (C_STACKHANDLING == 0) /* 0 -> system handled else stack size */

            /* start chain of lxis */
            lxis = codloc + 1;
        emit(LXI, RSP, C_STACKHANDLING);
    }
    while (!errflg) {
        ibase = 0;

        /* may have been stack overflow so... */
        if (sp < 0)
            sp = 0;
        if (C_ANALYSIS != 0)
            if (alter && sp > 0) {
                /* write stack */
                fprintf(lstFp, "\n  PR   ST   RASN  LITV\n");
                for (int ip = sp; ip > 0; ip--) {
                    fprintf(lstFp, "%02d %d ", ip, prec[ip]);
                    if (st[ip])
                        fprintf(lstFp, "%c%05d", st[ip] < 0 ? 'A' : 'S', abs(st[ip]));
                    else
                        fputs("      ", lstFp);

                    fprintf(lstFp, "  %c %c", rmap[HIGHNIBBLE(rasn[ip])],
                            rmap[LOWNIBBLE(rasn[ip])]);
                    if (litv[ip] >= 0)
                        fprintf(lstFp, " %c%05d", HIGHWORD(litv[ip]) ? 'R' : ' ',
                                LOWWORD(litv[ip]));
                    putc('\n', lstFp);
                }

                /* write registers */
                if (C_ANALYSIS >= 2) {
                    for (int i = 1; i <= 7; i++) {
                        ip = regs[i];
                        kp = lock[i];
                        lp = regv[i];
                        if (kp + ip + lp >= 0) {
                            fprintf(lstFp, " %c(%c,", rmap[i], kp == 1 ? 'L' : 'U');
                            if (ip == 0)
                                putc('*', lstFp);

                            else
                                fprintf(lstFp, "%02d", ip);
                            putc(',', lstFp);
                            if (lp < 0)
                                putc('*', lstFp);

                            else
                                fprintf(lstFp, "%04XH", lp);
                            putc(')', lstFp);
                        }
                    }
                    putc('\n', lstFp);
                }
            }

        do {
            /* copy the elt just read to the polish look-ahead, AND */
            /* interpret the previous elt */
            k = lapol;
            if (lapol != 0 && (lapol = getNextPol()) == EOF) {
                lapol = 0;
                fatal("127: invalid intermediate language format");
                return;
            }
        } while (!(k >= 0));

        /* check for END of code */
        if (k == 0)
            break;
        polcnt++;
        typ = k & 7;
        val = k >> 3;

        /* $g=0 for no trace, $g=1 gives lines vs locs, */
        /* $g=2 yields full interlist of i.l. */
        if (C_GENERATE != 0) {
            if (C_GENERATE > 1) {
                /* otherwise interlist the i.l. */
                fprintf(lstFp, "%05d %04XH %d %s ", codloc, codloc, polcnt, polchr[typ]);
                switch (typ) {
                case OPR:
                    fprintf(lstFp, "%s", opcval[val]);
                    break;
                case ADR:
                case VLU:
                case DEF:
                    fprintf(lstFp, "S%05d", val);
                    break;
                case LIT:
                case LIN:;
                    fprintf(lstFp, " %05d", val);
                }
                putc('\n', lstFp);
            } else
                /* print line number = code location, if altered */
                if (lline != C_COUNT && lloc != codloc) {
                    /* changed completely, so print it */
                    lline = C_COUNT;
                    lloc  = codloc;
                    if (lcnt <= 0) {
                        lcnt = C_WIDTH / 12;
                        putc('\n', lstFp);
                    } else
                        fputs("  ", lstFp);
                    lcnt--;

                    fprintf(lstFp, "%4d=%04XH", lline, lloc);
                }
        }
        if (++sp > maxsp) { /* stack overflow */
            fatal("128: register allocation stack overflow");
            sp = 1;
        }
        prec[sp] = 0;
        st[sp]   = 0;
        rasn[sp] = 0;
        litv[sp] = -1;
        alter    = false;
        switch (typ) {
        case OPR: /* operator */
            sp--;
            alter |= operat(val);
            continue;
        case ADR:
            if (sp > 1) {
                /* check for active condition code which must be changed to boolean */
                if (rasn[sp - 1] > 255)
                    cvcond(sp - 1);
            }
            if (symAttrib(val) >= 0) {
                setadr(val);
                continue;
            } else {
                /* load address of based variable.  change to */
                /* load value of the base, using the variable's precision */
                ibase = INFO_PREC(-symAttrib(val));
                val   = symRef(val);
            }
            break;
        case VLU:
            break;
        case DEF: /* mark last register load nil */
            lastReg     = 0;
            lastex      = 0;
            lastin      = 0;
            lastIncReg  = 0;
            lastLoadImm = 0; // only used in V4
            sp--;

            /* save registers if this is a PROC OR a LABEL which was */
            /* referenced IN a go-to statement OR was compiler-generated. */
            ip = symbol[val];
            i  = abs(symAttrib(val));

            /* save this DEF symbol number AND the literal values of the */
            /* h AND l registers for possible TRA chain straightening. */
            if (INFO_TYPE(i) == LABEL) {
                defsym = val;
                defrh  = regv[RH];
                defrl  = regv[RL];
            }

            /* we may convert the sequence */

            /* TRC l, TRA/PRO/RET, DEF l */

            /* to an equivalent conditional TRA/PRO/RET... */
            if (INFO_ECNT(i) == 1)
                if (tstloc == codloc)
                    if (conloc == xfrloc - 3) {
                        int addr = -symAddr(val);
                        // V4
                        if ((-symAddr(val)) / 4 == conloc + 1) {
                            /* adjust backstuffing chain for JMP OR call */
                            if (xfrsym > 0)
                                /* decrement backstuff location by 3 */
                                symAddr(xfrsym) += 3 << 2;

                            /* arrive here with the configuration TRC...DEF */
                            // V4
                            symAddr(val) = -(addr % 4);
                            if (symAttrib(val) < 0)
                                symAttrib(val) = -(abs(symAttrib(val)) & 0xff);
                            else
                                symAttrib(val) &= 0xff;
                            int j = ((get(conloc) ^ 8) & ~7) + (get(xfrloc) & 6);
                            for (;;) {
                                put(conloc, j);
                                conloc++;
                                xfrloc++;
                                j = get(xfrloc);
                                if (xfrloc == codloc) {
                                    codloc = conloc;
                                    conloc = xfrloc = tstloc = -1;

                                    /* notice that defrh AND defrl are now incorrect */
                                    /* defsym=0 prevents use of these variables... */
                                    /* ... if a TRA immediately follows */
                                    defsym = 0;
                                    break;
                                }
                            }
                        }
                    }
            int type = INFO_TYPE(i);
            if (type == LABEL) {
                /* LABEL found.  check for reference to LABEL */
                i = i / 256;
                if (i == 0)
                    goto L370;

                /* check for single reference, no conflict with h AND l */
                if (i == 1) {
                    // V4
                    i = symF3(val);
                    /* check for previous reference  forward */
                    // V4
                    if (i && i != -1) {
                        l = (i & LVALID) ? i & 0xff : -1;
                        j = (i & HVALID) ? (i >> 8) & 0x1ff : -1;
                        /* j is h reg, l is l reg */
                        lock[RH] = true;
                        lock[RL] = true;
                        saver();

                        /* compare old hl with new hl */
                        lock[RH] = false;
                        lock[RL] = false;
                        regv[RH] = (regv[RH] == (-255) || regv[RH] == j) ? j : -1;
                        regv[RL] = (regv[RL] == (-255) || regv[RL] == l) ? l : -1;
                        goto L370;
                    }
                }
            } else if (type == PROC) {
                /* set up procedure stack for procedure entry */
                if (++prsp > prsmax)
                    fatal("145: procedures nested too deeply");

                else {
                    // V4
                    j           = ip - 3;
                    prstk[prsp] = j;

                    /* mark h AND l as unaltered initially */
                    /* /  1b  /  1b  /  1b  /  1b  /  9b  /  8b  / */
                    /* /h unal/l unal/h vald/l vald/h valu/l valu/ */
                    /* ------------------------------------------- */
                    symbol[j] = HNOTSET | LNOTSET;
                    saver();
                    regv[RH] = -254;
                    regv[RL] = -254;
                    k        = codloc;

                    /* set up stack depth counters */
                    maxdep[prsp] = 0;
                    curdep[prsp] = 0;
                    for (int i = 0; i < 8; i++)
                        if (val == intpro[i]) {
                            /* interrupt procedure is marked with ho 1 */
                            prstk[prsp] = j + 65536;
                            emit(PUSH, RH, 0);
                            emit(PUSH, RD, 0);
                            emit(PUSH, RB, 0);
                            emit(PUSH, RA, 0);
                            stack(4);
                        }
                    goto L380;
                }
            }
            saver();

            /* LABEL is resolved.  last two bits of entry must be 01 */
        L370:
            k = codloc;
        L380:
            i = -symAddr(val);
            j = i % 4;
            i = i / 4;
            if (j != 1)
                error("131: label resolution error in pass-2");
            // V4
            symAddr(val) = -(shl(i, 2) + 3);
            symRef(val)  = k;
            /* now check for procedure entry point */
            i = symAttrib(val);
            if (right(i, 4) == PROC) {
                i >>= 8;

                /* build receiving sequence for register parameters */
                if (i >= 1) {
                    k = i - 2;
                    if (k < 0)
                        k = 0;
                    if (i > 2)
                        i = 2;
                    for (int j = 1; j <= i; j++) {
                        if (++sp > maxsp) {
                            fatal("113: register allocation stack overflow");
                            sp = 1;
                        }

                        /* (RD,RE) = 69    (RB,RC) = 35 */
                        rasn[sp] = j == 1 ? 35 : 69;
                        st[sp]   = 0;
                        litv[sp] = -1;
                        prec[sp] = 2;
                        if (++sp > maxsp) {
                            fatal("113: register allocation stack overflow");
                            sp = 1;
                        }
                        rasn[sp] = 0;
                        litv[sp] = -1;
                        setadr(val + k + j);
                        alter |= operat(STD);
                    }
                }
            }
            continue;
            break;
        case LIT:
            /* check for active condition code which must be changed to boolean */
            if (sp > 1 && rasn[sp - 1] > 255)
                cvcond(sp - 1);
            alter    = true;
            litv[sp] = val;
            prec[sp] = litv[sp] > 255 ? 2 : 1;
            continue;
        case LIN:
            /* line number */
            C_COUNT = val;
            sp--;
            continue;
        }
        i = symbol[val];
        j = symAttrib(val);
        if (sp > 1)
            /* allow only a LABEL variable to be stacked */
            if (abs(j) % 16 != LABEL) {
                /* check for active condition code which must be changed to boolean */
                if (rasn[sp - 1] > 255)
                    cvcond(sp - 1);
            }
        /* check for condition codes */
        if (val <= intbas) {
            if (val <= 4) {
                /* CARRY ZERO MINUS PARITY */
                /* set to true/condition (1*16+val) */
                rasn[sp] = (16 + val) * 256;
                st[sp]   = 0;
                prec[sp] = 1;
                alter    = true;
                continue;
            } else if (val < firsti || val > intbas) { /* may be a call to input OR output */
                /* check for reference to 'memory' */
                /* ** note that 'memory' must be at location 5 IN the symbol table ** */
                if (val != 5) {
                    /* ** note that 'stackptr' must be at 6 in sym tab */
                    if (val != 6)
                        error("129: invalid use of built-in function in an assignment");

                    else {
                        /* load value of stackpointer to registers immediately */
                        genreg(2, &ia, &ib);
                        if (ib == 0)
                            fatal("107: register allocation error. No registers available");
                        else {
                            rasn[sp] = REGPAIR(ib, ia);
                            litv[sp] = -1;
                            st[sp]   = 0;
                            regs[ia] = sp;
                            regs[ib] = sp;
                            prec[sp] = 2;
                            emit(LXI, RH, 0);
                            emit(DAD, RSP, 0);
                            emit(LD, ia, RL);
                            emit(LD, ib, RH);
                            regv[RH] = -1;
                            regv[RL] = -1;
                            alter    = true;
                        }
                    }
                    continue;
                }
            }
        }
        if (j < 0)
            /* value reference to based variable. first insure that this */
            /* is NOT a length attribute reference, (i.e., the variable is */
            /* NOT an actual parameter for a call on length OR last) by */
            /* insuring that the next polish elt is NOT an address */
            /* reference to symbol (length+1) OR (last+1) */
            /* note that this assumes length AND last are symbol numbers */
            /* 18 AND 19 */
            if (lapol != 153 && lapol != 161) {
                /* load value of base variable.  change to load */
                /* value of base, followed by a LOD op. */
                ibase = right(shr(-j, 4), 4) + 16;
                val   = symRef(val);
                j     = symAttrib(val);
            }
        alter = true;

        /* examine attributes */
        st[sp]   = val;
        k        = ibase > 0 ? ibase & 0xf : INFO_PREC(j);
        prec[sp] = k;
        if (INFO_TYPE(j) >= LITER) {
            if (k > 0 && k < 3) // byte, word, or string
                litv[sp] = INFO_ECNT(j);
            else {
                error("130: pass-2 compiler error. Invalid variable precision");
                continue;
            }
        }

        /* check for base address which must be loaded */
        if (ibase >= 16) {
            /* must be a based variable value reference. */
            /* load the value of the base AND follow it by */
            /* a load operation. */
            k = prec[sp];

            /* mark as a byte load for the LOD operation IN operat */
            /* leaves 2 if double byte result AND 6 (=2 mod 4) if single byte */
            prec[sp] = 10 - 4 * k;
            alter |= operat(LOD);
        }
    }
    emit(EI, 0, 0);
    emit(HALT, 0, 0);

    /* may be line/loc's left IN output buffer */
    if (C_GENERATE != 0)
        putc('\n', lstFp);
    ;
}

bool doBuiltin(int val) {
    int i, j, k, m;
    int ip, kp;
    int ia, ib;

    _delete(1);
    switch (val) {
    case B_ROL:
    case B_ROR:
    case B_SHL:
    case B_SHR:
    case B_SCL:
    case B_SCR:
    case B_HIGH:
    case B_LOW:
        if (val > B_SCR) /* ** note that this also assumes only 6 such bifs */
            unary(val);
        else {
            i = litv[sp];
            if (i > 0) { /* generate IN-line code for shift counts of */
                         /* 1 OR 2 for address values */
                         /* 1 to 3 for shr of byte values */
                         /* 1 to 6 for all other shift functions on byte values */
                j = prec[sp - 1] != 1 ? 2 : val == B_SHR ? 3 : 6;
                if (i <= j) {
                    _delete(1);
                    for (int j = 1; j <= i; j++)
                        unary(val);
                    return true;
                }
            }
            exch();
            /* load the value to decrement */
            loadv(sp - 1, 0);
            j = REGLOW(rasn[sp - 1]);
            if (regs[RA] == j) {
                emit(LD, j, RA);
                regs[RA] = 0;
            }
            lock[j] = true;

            /* load the value which is to be operated upon */
            kp = prec[sp];
            i  = 1;
            if (kp > 1)
                i = 0;
            if (rasn[sp] == 0) {
                loadv(sp, i);
                if (i == 1)
                    regs[RA] = REGLOW(rasn[sp]);
            }
            m = REGLOW(rasn[sp]);
            if (i != 1 || regs[RA] != m) {
                if (regs[RA]) {
                    emit(LD, regs[RA], RA);
                    regs[RA] = 0;
                }
                if (i) {
                    emit(LD, RA, m);
                    regs[RA] = m;
                }
            }
            i = codloc;
            unary(val);
            if (kp != 1) {
                if (regs[RA])
                    emit(LD, regs[RA], RA);
                regs[RA] = 0;
            }
            emit(DC, j, 0);
            emit(JMC, FAL * 32 + ZERO, i);

            /* END up here after operation completed */
            exch();
            lock[j] = false;
            _delete(1);
        }
        return true;
    case B_TIME:
        if (rasn[sp] > 255)
            cvcond(sp);
        /* emit the following code sequence for 100 usec per loop */
        /* 8080 cpu only */
        /* (get time parameter into the accumulator) */
        /*          mvi   b,12   (7 CY overhead) */
        /* start    mov   c,b    (5 CY * .5 usec = 2.5 usec) */
        /* -------------------- */
        /* tim180   dcr   c      (5 CY * .5 usec = 2.5 usec) */
        /*          jnz   tim180 (10 CY* .5 usec = 5.0 usec) */
        /* -------------------- */
        /*              12 *     (15 CY* .5 usec = 7.5 usec) */
        /*              =        (180 CY* .5 usec = 90 usec) */
        /*          dcr   a      (5 CY * .5 usec = 2.5 usec) */
        /*          jnz   start  (10 CY* .5 usec = 5.0 usec) */

        /*          total time   (200 CY*.5 usec = 100 usec/loop) */
        j  = regs[RA];
        ip = REGHIGH(rasn[sp]);
        i  = REGLOW(rasn[sp]);
        if (!j || j != i) {
            /* get time parameter into the accumulator */
            if (j != 0 && j != ip)
                emit(LD, j, RA);
            regs[RA] = 0;
            if (i == 0)
                loadv(sp, 1);
            i = REGLOW(rasn[sp]);
            if (j)
                emit(LD, RA, i);
        }
        regs[RA] = 0;
        emit(LD, i - 1, -12);
        emit(LD, i, i - 1);
        emit(DC, i, 0);
        emit(JMC, FAL * 32 + ZERO, codloc - 1);
        emit(DC, RA, 0);
        emit(JMC, FAL * 32 + ZERO, codloc - 6);
        _delete(1);
        return true;
    case B_INPUT:
        /* input function. get input port number */
        i = litv[sp];
        if (i >= 0 && i <= 255) {
            _delete(1);
            sp++;
            genreg(1, &j, &k);
            if (j != 0) {
                k = regs[RA];
                if (k != 0)
                    emit(LD, k, RA);
                regs[RA] = rasn[sp] = j;
                litv[sp]            = -1;
                st[sp]              = 0;
                prec[sp]            = 1;
                regs[j]             = sp;
                emit(INP, i, 0);
                return true;
            }
        }
        break;
    case B_OUTPUT:
        /* check for proper output port number */
        i = litv[sp];
        if (i >= 0 && i <= 255) {
            _delete(1);
            /* now build an entry which can be recognized by */
            /* operat. */
            litv[++sp] = i;
            rasn[sp]   = 0;
            prec[sp]   = 1;
            st[sp]     = outloc;
            return true;
        }
        break;
    case B_LENGTH:
    case B_LAST:
        j = st[sp];
        if (j > 0) {
            int rval = abs(symAttrib(j)) / 256 + B_LENGTH - val;
            _delete(1);
            sp       = sp + 1;
            st[sp]   = 0;
            prec[sp] = rval > 255 ? 2 : 1;
            rasn[sp] = 0;
            litv[sp] = rval;
            return true;
        }
        break;
    case B_MOVE: // move is explicitly expanded in pass 1
        break;
    case B_DOUBLE:
        if (prec[sp] > 1)
            return false;
        if (rasn[sp] == 0) {
            if (litv[sp] < 0) {
                /* load value to accumulator AND get a register */
                loadv(sp, 1);
                regs[RA] = REGLOW(rasn[sp]);
            } else {
                prec[sp] = 2;
                st[sp]   = 0;
                return true;
            }
        }
        ia       = rasn[sp];
        prec[sp] = 2;
        st[sp]   = 0;
        if (ia <= CARRY) {
            lock[ia] = true;
            ib       = ia - 1;
            regs[ib] = sp;
            lock[ia] = false;
            rasn[sp] = REGPAIR(ib, ia);
            /* ZERO the register */
            emit(LD, ib, 0);
            if (ib == 0)
                fatal("133: register allocation stack overflow");
        }
        return true;
    case B_DEC:
        j = REGLOW(rasn[sp]);
        if (j != 0)
            if (prec[sp] == 1) {
                i = regs[RA];
                if (i != j) { /* may be a pending register store */
                    if (i != 0)
                        emit(LD, i, RA);
                    emit(LD, RA, j);
                    regs[RA] = j;
                }
                emit(DAA, 0, 0);
                return true;
            }
        break;
    }

    /* built IN function error */
    error("136: error in built-in function call");
    return false;
}

bool operat(int val) {
    static int it;
    int i, j, k, m, l, /* icy, iq,*/ iop = 0, iop2 = 0, ip, jp; // assign to appease GCC
    int jph, kp, id, ml, mh, ia, ib, il, lp;
    bool skip;

    /* ADD ADC SUB SBC MUL div mod NEG AND IOR */
    /* XOR NOT EQL LSS GTR NEQ LEQ GEQ INX TRA */
    /* TRC PRO RET STO STD XCH DEL cat LOD BIF */
    /* INC CSE END ENB ENP HAL RTL RTR SFL SFR */
    /* HIV LOV CVA ORG AX1 AX2 AX3 */
    // icy = 0;
    // iq  = 0;
    switch (val) {
    case INC: /* place a literal 1 at stack top AND apply ADD operator */
        litv[++sp] = 1;

        if (prec[sp - 1] == 1) { /* check for single byte increment, may be comparing with 255 */
            apply(AD, AC, true, 1);
            lastin = codloc;
            return true;
        } // fall through to add
    case ADD:
        /* may do the ADD IN h AND l (using INX operator) */
        if (prec[sp] != 1)
            exch();
        if (prec[sp - 1] != 1)
            inx(1); // prec=1 for inx
        else {
            exch();
            apply(AD, AC, true, 1);
        }
        return true;
    case ADC:
        apply(AC, AC, true, 1);
        return true;
    case SUB: /* change address value - 1 to address value + 65535 AND apply ADD */
        if (prec[sp - 1] != 1 && litv[sp] == 1) {
            litv[sp] = 65535;
            prec[sp] = 2;
            inx(1);
        } else
            apply(SU, SB, false, 1);
        return true;
    case SBC:
        apply(SB, SB, false, 1);
        return true;
    case MUL:
        builtin(0, 2);
        return true;
    case DIV:
        builtin(1, 1);
        return true;
    case MDF: // MOD
        builtin(1, 2);
        return true;
    case NEG:
    case BIF:
    case END:
    case ENB:
    case ENP:
    case AX3:
        return false;
    case AND:
        apply(ND, ND, true, 0);
        return true;
    case IOR:
        apply(OR, OR, true, 0);
        return true;
    case XOR:
        apply(XR, XR, true, 0);
        return true;
    case NOT:
        if (rasn[sp] > 255) /* condition code - change PARITY */
            rasn[sp] ^= 0x1000;
        else { /* perform XOR with 255 OR 65535 (byte OR address) */
            i          = prec[sp];
            litv[++sp] = (1 << (i * 8)) - 1;
            prec[sp]   = i;
            apply(XR, XR, true, 0);
        }
        return true;
    case EQL: /* equal test */
        if (prec[sp] + prec[sp - 1] <= 2) {
            apply(SU, 0, true, 0);
            rasn[sp] += ZFLAG; /* mark as true/ZERO (1*16+2) */
        } else
            compare16(true, ZFLAG, 1);
        return true;
    case GTR: /* GTR - change to LSS */
        exch();
    case LSS: /* LSS set to tru/CARRY (1 * 16 + 1) */
        if (prec[sp] + prec[sp - 1] <= 2) {
            apply(litv[sp] != 1 ? SU : CP, 0, false, 0);
            rasn[sp] += CFLAG; /* mark as condition code */
        } else
            compare16(false, CFLAG, 0);
        return true;
    case NEQ:
        if (prec[sp] + prec[sp - 1] <= 2) {
            apply(SU, 0, true, 0);
            rasn[sp] += NZFLAG; /* mark as false/ZERO (0*16 + 2) */
        } else
            compare16(true, NZFLAG, 1);
        return true;
    case LEQ: /* LEQ - change to GEQ */
        exch();
    case GEQ:
        if (prec[sp] + prec[sp - 1] <= 2) {
            apply(litv[sp] != 1 ? SU : CP, 0, false, 0);
            rasn[sp] += NCFLAG; /* mark as condition code */
        } else
            compare16(false, NCFLAG, 0);
        return true;
    case INX:
        inx(prec[sp - 1]);
        return true;
    case TRC:
        j = sp - 1;
        i = litv[j];
        if (right(i, 1) == 1) { /* this is a do forever (OR something similar) so ignore the jump */
            _delete(2);
            return true;
        }
        iop = 2; /* NOT a literal '1' */
        /* check for condition code */
        if (rasn[j] > 255) { /* active condition code, construct mask for JMC */
            i    = HIGH(rasn[j]) % 16;
            j    = HIGH(rasn[j]) / 16;
            iop2 = (FAL + 1 - j) * 32 + (CARRY + i - 1);
        } else {
            if (rasn[j] == 0) { /* load value to accumulator */
                prec[j] = 1;
                loadv(j, 1);
            } else if (REGLOW(rasn[j]) != regs[RA]) { /* value already loaded */
                if (regs[RA])
                    emit(LD, regs[RA], RA);
                emit(LD, RA, REGLOW(rasn[j]));
            }
            regs[RA] = 0;
            emit(ROT, CY, RGT);
            iop2 = FAL * 32 + CARRY;
        }
        break;
    case PRO:
        i = st[sp];
        if (i > intbas) { /* pass the last two (at most) parameters IN the registers */
            i = right(st[sp], 16);
            i = min(INFO_ECNT(symAttrib(i)), 2);
            if (i < 1) {
                lock[RH] = lock[RL] = true;
                saver();
                lock[RH] = lock[RL] = false;
                iop                 = 3;
            } else {
                j = sp - i - i;
                for (int k = 1; k <= i; k++) {
                    ip = rasn[j] & 0xf;
                    jp = (rasn[j] >> 4) & 0xf;
                    if (ip != 0)
                        lock[ip] = true;
                    if (jp != 0)
                        lock[jp] = true;
                    prec[j] = min(prec[j], prec[j + 1]);
                    if (prec[j] <= 1 && jp != 0) {
                        // bug fix from V4 -- clear pending store if passing address var to byte
                        // var
                        if (regs[RA] == jp)
                            regs[RA] = 0;
                        regs[jp] = lock[jp] = false;
                        jp                  = 0;
                        if (regs[RA] == ip)
                            lock[RA] = true;
                        if (regs[RA] == jp)
                            lock[RA] = true;
                    }
                    rasn[j] = REGPAIR(jp, ip);
                    j += 2;
                }
                j  = sp - 1 - 2 * i;
                it = 0;
                /* stack any stuff which does NOT go to the procedure */
                for (int k = 1; k <= sp; k++) { /* check for value to PUSH */
                    if (rasn[k] == 0) { /* registers NOT assigned - check for stacked value */
                        if (st[k] == 0 && litv[k] < 0 && it != 0)
                            error("150: pass-2 compiler error in 'loadv'");
                    } else if (k <= j) { /* possible PUSH if NOT a parameter */
                                         /* registers must be pushed */
                        jph = REGHIGH(rasn[k]);
                        kp  = regs[RA];
                        jp  = REGLOW(rasn[k]);
                        if (kp != 0) {       /* pending ACC store, check ho AND lo registers */
                            if (kp == jph) { /* pending ho byte store */
                                emit(LD, jph, RA);
                                regs[RA] = 0;
                            } else if (kp == jp) { /* check lo byte */
                                emit(LD, jp, RA);
                                regs[RA] = 0;
                            }
                        }
                        emit(PUSH, jp - 1, 0);
                        stack(1);
                        st[k] = 0;
                        jp    = REGLOW(rasn[k]);
                        if (jp != 0)
                            regs[jp] = 0;
                        jp = REGHIGH(rasn[k]);
                        if (jp != 0)
                            regs[jp] = 0;
                        rasn[k] = 0;
                        litv[k] = -1;
                        it      = k;
                    }
                }
                it = RH;
                j  = sp - i - i;
                for (int k = 1; k <= i; k++) {
                    id = 2 * k + 2;
                    jp = REGHIGH(rasn[j]);
                    ip = REGLOW(rasn[j]);
                    for (;;) {
                        id--;
                        if (ip == 0)
                            break;
                        if (ip != id) {
                            if (regs[id] != 0) {
                                m  = regs[id];
                                mh = REGHIGH(rasn[m]);
                                ml = REGLOW(rasn[m]);
                                if (ml == id)
                                    ml = it;
                                if (mh == id)
                                    mh = it;
                                // Bug fix from V4 -- CLEAR PENDING STORE WHEN REG PAIRS ARE TO
                                // BE EXCHANGED ***
                                if (regs[RA] == id) {
                                    emit(LD, it, RA);
                                    regs[RA] = lock[RA] = false;
                                } else
                                    // end of change
                                    emit(LD, it, id);
                                regs[it] = m;
                                rasn[m]  = REGPAIR(mh, ml);
                                it++;
                            }
                            regs[ip] = lock[ip] = false;
                            if (regs[RA] == ip) {
                                ip       = 1;
                                regs[RA] = lock[RA] = false;
                            }
                            emit(LD, id, ip);
                            regs[id] = j;
                        }
                        lock[id] = true;
                        ip       = jp;
                        if (ip == (-1))
                            break;
                        jp = -1;
                    }
                    j += 2;
                }
                j = sp - 2 * i;
                for (int k = 1; k <= i; k++) {
                    if (rasn[j] == 0)
                        loadv(j, 0);
                    ip       = 2 * k;
                    regs[ip] = j;
                    lock[ip] = true;
                    if (prec[j + 1] == 2 && prec[j] == 1)
                        emit(LD, ip, 0);
                    j += 2;
                }
                if (regs[RA] != 0)
                    emit(LD, regs[RA], RA);
                for (int k = 1; k <= 7; k++) {
                    regs[k] = lock[k] = false;
                    regv[k]           = -1;
                }
                j = 2 * i;
                for (int k = 1; k <= j; k++) {
                    exch();
                    if (st[sp] == 0 && rasn[sp] == 0 && litv[sp] < 0) {
                        emit(POP, RH, 0);
                        ustack();
                        regv[RH] = regv[RL] = -1;
                    }
                    _delete(1);
                }
                iop = 3;
            }
            break;
        } else                                /* this is a built-IN function. */
            return doBuiltin(i - firsti + 1); // pass function number
        break;
    case RET:
        jp = prsp;
        if (jp <= 0) {
            error("146: procedure optimization stack underflow");
            regv[RH] = regv[RL] = -255; /* mark as nil */
            return true;
        }
        // V4 /* check for type AND precision of procedure */
        l = LOWWORD(prstk[jp]) + 2;
        l = symbol[l] / 16 % 16;

        /* l is the precision of the procedure */
        if (l != 0) {
            i = rasn[sp];
            if (i == 0)
                loadv(sp, 1);
            else if (i >= 256)
                cvcond(sp);
            jp = regs[RA];
            j  = REGLOW(rasn[sp]);
            k  = REGHIGH(rasn[sp]);
            if (i != 0 && j != jp) { /* have to load the accumulator.  may have h.o. byte. */
                if (jp != 0 && jp == k)
                    emit(LD, k, RA);
                emit(LD, RA, j);
            }
            if (k != 0 && k != RB)
                emit(LD, RB, k);
            if (l > prec[sp]) /* compare precision of procedure with stack */
                emit(LD, RB, 0);
        }
        _delete(1);
        if (prstk[prsp] > 65535) { /* interrupt procedure - use the DRT code below */
            emit(POP, RA, 0);
            emit(POP, RB, 0);
            emit(POP, RD, 0);
            emit(POP, RH, 0);
            emit(EI, 0, 0);
            emit(RTN, 0, 0);
            if (prsp <= 0) {
                error("146: procedure optimization stack underflow");
                regv[RH] = regv[RL] = -255; /* mark as nil */
                return true;
            }
        } else
            emit(RTN, 0, 0);
        /* merge values of h AND l for this procedure */
        updateHL(prsp);
        return true;
    case STO:
    case STD:
        /* STO AND STD */
        i = st[sp];

        /* check for output function */
        if (i != outloc) { /* check for computed address OR saved address */
            if (i < 0) {   /* check for address reference outside intrinsic range */
                i = -i;
                if (i <= intbas) /* check for 'memory' address reference */
                                 /* ** note that stacktop must be at 6 ** */
                    if (i > 6) {
                        if (val == STD)
                            _delete(1);
                        return true;
                    }
            }
            i = 1;

            /* check for STD */
            if (val == STD)
                i = 0;
            gensto(i);
        } else {
            j = litv[sp];
            i = rasn[sp - 1];
            if (i <= 0 || i >= 256) { /* load value to ACC */
                i = regs[RA];
                if (i > 0)
                    emit(LD, i, RA);
                loadv(sp - 1, 1);
                i = rasn[sp - 1];
            } else { /* operand is IN the gprs */
                i %= 16;
                k = regs[RA];
                if (k > 0 && k != i)
                    emit(LD, k, RA);
                if (k != i)
                    emit(LD, RA, i);
            }
            /* now mark ACC active IN case subsequent STO operator */
            regs[RA] = i % 16;
            emit(OUT, j, 0);
            _delete(1);
        }
        /* * check for STD * */
        if (val == STD)
            _delete(1);
        return true;
    case XCH:
        exch();
        return true;
    case DEL:
        if (st[sp] == 0 && rasn[sp] == 0 && litv[sp] < 0) {
            /* value is stacked, so get rid of it */
            emit(POP, RH, 0);
            regv[RH] = regv[RL] = -1;
            ustack();
        }
        _delete(1);
        return true;
    case DAT:
        inldat();
        return false;
    case LOD:
        il   = 0;
        skip = false;
        k    = prec[sp];

        /* may be a LOD from a base for a based variable */
        prec[sp] = k % 4;
        ia       = rasn[sp];
        if (ia <= 0) {
            /* check for simple based variable case */
            i = st[sp];
            if (i > 0) { /* reserve registers for the result */
                genreg(2, &ia, &ib);
                regs[ia] = sp;
                regs[ib] = sp;
                rasn[sp] = REGPAIR(ib, ia);
                /* may be able to simplify LHLD */
                lp = regv[RH];
                l  = regv[RL];
                if (lp != -3 || (-l) != i) {
                    if (lp == (-4) && (-l) == i) {
                        emit(DCX, RH, 0);
                        regv[RH] = -3;
                    } else {
                        j = chain(i, codloc + 1);
                        emit(LHLD, j, 0);
                        regv[RH] = -3;
                        regv[RL] = -i;
                    }
                }
                skip = true;
            } else if (st[sp] == 0) { /* first check for an address reference */
                loadv(sp, 0);
                ia = rasn[sp];
            } else { /* change the address reference to a value reference */
                st[sp]   = -st[sp];
                litv[sp] = -1;
                return true;
            }
        }
        if (!skip) {
            ib = REGHIGH(ia);
            ia = REGLOW(ia);
            i  = regs[RA];
            if (ia == i)
                ia = 1;
            if (ib == i)
                ib = 1;
            if (ib == ia - 1)
                il = ib;
            if (ia * ib == 0) {
                fatal("138: register allocation error");
                return true;
            }
            /* may be possible to use LDAX OR XCHG */
            if (il != RD) {
                emit(LD, RL, ia);
                emit(LD, RH, ib);
                il       = 0;
                regv[RH] = regv[RL] = -1;
            } else if (lastex == codloc - 1 || prec[sp] % 2 != 1) {
                /* double XCHG OR double byte load with addr IN d AND e */
                emit(XCHG, 0, 0);
                il       = 0;
                regv[RH] = regv[RL] = -1;
            }
        }

        i        = prec[sp] - k / 4;
        prec[sp] = i;

        /* recover the register assignment from rasn */
        ia = REGLOW(rasn[sp]);
        ib = REGHIGH(rasn[sp]);
        j  = regs[RA];

        /* skip if j=0, ia, OR ib */
        if (j != 0 && j != ia && j != ib)
            emit(LD, j, RA);

        /* may be able to change register assignment to bc */
        if (ia == RE)
            if (regs[RB] == 0 && regs[RC] == 0) { /* bc available, so RE-assign */
                regs[ia] = regs[ib] = 0;
                regs[RB] = regs[RC] = sp;
                ia                  = RC;
                ib                  = RB;
                rasn[sp]            = REGPAIR(RB, RC);
            }
        regs[RA] = ia;
        if (il == 0)
            emit(LD, RA, ME);
        else
            emit(LDAX, il, 0);
        if (i > 1) {
            emit(INCX, RH, 0);
            /* may have done a prevous LHLD, if so mark INCX h */
            if (regv[RH] == -3)
                regv[RH] = -4;
            emit(LD, ib, ME);
        } else { /* single byte load - release h.o. register */
            ib = REGHIGH(rasn[sp]);
            if (ib == regs[RA])
                regs[RA] = 0;
            regs[ib] = 0;
            regv[ib] = -1;
            rasn[sp] = REGLOW(rasn[sp]);
        }
        regs[RH] = regs[RL] = st[sp] = 0;
        return true;
    case CSE:

        /* let x be the value of the stack top */
        /* compute 2*x + codloc, fetch to hl, AND jump with PCHL */
        /* reserve registers for the jump table base */
        genreg(2, &ia, &ib);
        lock[ia] = true;
        lock[ib] = true;

        /* index is IN h AND l, so double it */
        emit(DAD, RH, 0);

        /* now load the value of table base, depending upon 9 bytes */
        /* LXI r x y, DAD r, mov em, INX h, mov dm XCHG PCHL */
        emit(LXI, ib, codloc + 9);
        emit(DAD, ib, 0);
        emit(LD, RE, ME);
        emit(INCX, RH, 0);
        emit(LD, RD, ME);
        emit(XCHG, 0, 0);
        emit(PCHL, 0, 0);

        /* phoney entry IN symbol table to keep code dump clean */
        symbol[++sytop]  = syinfo;
        symbol[syinfo--] = -codloc;

        /* set entry to len=0/prec=2/type=VARB/ */
        symbol[syinfo] = PACK_ATTRIB(0, 2, VARB);
        casjmp         = syinfo;

        /* casjmp will be used to update the length field */
        if (--syinfo <= sytop)
            fatal("108: pass-2 symbol table overflow");
        lock[ib] = false;
        regv[RH] = regv[RL] = -255; /* mark as nil */
        return true;
    case HAL:
        emit(EI, 0, 0);
        emit(HALT, 0, 0);
        return true;
    case RTL:
    case RTR:
    case SFL: // not used
    case SFR:
    case HIV:
    case LOV:
        unary(val - 36);
        return true;
    case CVA:
        /* CVA must be immediately preceded by an INX OR ADR ref */
        prec[sp] = 2;

        /* if the address is already IN the gpr's then nothing to do */
        if (rasn[sp] > 0)
            return true;
        if (st[sp] < 0) { /* check for address ref to data IN rom. */
            jp = litv[sp];
            if (jp <= 65535) {
                if (jp < 0)
                    error("149: pass-2 compiler error. Attempt to convert invalid value to "
                          "address type");
                /* leave literal value */
                st[sp] = 0;
                return true;
            } /* do LXI r with the address */
            genreg(2, &ia, &ib);
            if (ia <= 0) {
                fatal("140: register allocation error");
                return false;
            } else {
                j = chain(-st[sp], codloc + 1);
                emit(LXI, ib, j);
                st[sp]   = 0;
                rasn[sp] = REGPAIR(ib, ia);
                regs[ia] = sp;
                regs[ib] = sp;
                return true;
            }
        } else if (st[sp] > 0) {
            /* load value of base for address ref to a based variable */
            loadv(sp, 3);
            return true;
        } else {
            error("139: error in changing variable to address reference");
            return false;
        }
        break; // not needed no path to here
    case ORG:
        i = litv[sp];
        // V4
        if (i < 0)
            error("154: bad code origin from pass-1");
        else {
            if (codloc > i)
                error("141: invalid origin");
            j = C_STACKHANDLING;
            k = j == 1 ? 0 : 3; // allow space for lxi sp, if not user allocated
            if (codloc == offset + preamb + k) {
                /* this is the start of program, change offset */
                offset = i - preamb;
                codloc = i + k;
                if (lxis > 0)
                    lxis = codloc - 2;
            } else {
                while (codloc < i) /* some code has been generated, so LXI if necessary */
                    emit(0, 0, 0);

                if (j != 1) {    // not user handled
                    if (j < 1) { // system allocated
                        j    = lxis;
                        lxis = codloc + 1;
                    }
                    emit(LXI, RSP, j);
                }
            }
        }

        sp--;
        return true;
    case DRT:
        jp = prsp;
        if (prstk[jp] > 65535) /* this is the END of an interrupt procedure */
            curdep[jp] -= 4;
        if (prsp > 0)
            prsp--;

        /* get stack depth for symbol table */
        if (jp > 0) {
            if (curdep[jp] != 0)
                error("150: stack not empty at end of compilation");
            k = maxdep[jp];
            l = prstk[jp] % 65536 - 1;

            /* k is max stack depth, l is symbol table count entry */
            symbol[l] = k;
        }
        k = regv[RH];
        l = regv[RL];
        if (k == (-255) && l == (-255))
            return false;
        if (prstk[jp] > 65535) { /* POP interrupted registers AND enable interrupts */
            emit(POP, RA, 0);
            emit(POP, RB, 0);
            emit(POP, RD, 0);
            emit(POP, RH, 0);
            emit(EI, 0, 0);
        }
        emit(RTN, 0, 0);
        if (k != -254 || l != -254) {
            if (jp > 0) {
                updateHL(jp);
                return true;
            } else
                error("146: procedure optimization stack underflow");
        }
        regv[RH] = regv[RL] = -255; /* mark as nil */
        return true;
    case ENA:
        emit(EI, 0, 0);
        return false;
    case DIS:
        emit(DI, 0, 0);
        return false;
    case AX1:
        /* load case number to h AND l */
        exch();
        loadv(sp, 4);
        _delete(1);
        regv[RH] = -1;
        regv[RL] = -1;
        // fall through to shared code
    case TRA: /* TRA -   check stack for simple LABEL variable */
        iop      = 1;
        lock[RH] = lock[RL] = true; /* IN case there are any pending values ... */
        saver();
        lock[RH] = lock[RL] = false;
        m                   = litv[sp];
        if (m >= 0) { /* absolute jump - probably to assembly language subrtne... */

            regv[RH] = regv[RL] = -1; /* ...so make h AND l registers unknown */
            emit(JMP, m, 0);
            _delete(1);
            return true;
        }
        break;
    case AX2: /* may NOT be omitted even though no obvious path exists). */
        iop = 4;
        /* casjmp points to symbol table attributes - INC len field */
        symbol[casjmp] += 256;
        break;
    }

    i = st[sp];
    if (i > 0) {
        j = INFO_TYPE(symAttrib(i));
        /* may be a simple variable */
        if (iop != 1 || j != VARB) {
            if ((iop != 3 || j != PROC) && j != LABEL) {
                error("135: invalid program transfer (only computed jumps are allowed with a "
                      "'go to')");
                sp--;
                return true;
            } else {
                j = -symAddr(i);
                m = symRef(i);

                if (iop == 1) {
                    it = INFO_PREC(abs(symAttrib(i)));

                    /* it is type of LABEL... */
                    /* 3 is user-defined outer block, 4 is user defined */
                    /* NOT outer block, 5 is compiler defined */
                    if (it == 5 && defsym > 0 && INFO_PREC(symAttrib(defsym)) == 5) {
                        /* this TRA is one of a chain of compiler generated */
                        /* TRA's - straighten the chain if no code has been */
                        /* generated since the previous DEF. */

                        l  = -symAddr(defsym);
                        jp = symRef(defsym);
                        if (jp == codloc) {
                            /* adjust the reference counts AND optimization */
                            /* information for both DEF's. */
                            ia = INFO_ECNT(abs(symAttrib(defsym)));
                            ib = ia == 1 ? symF3(defsym) : 0;

                            if (defrh == -255)
                                ia--;
                            symAttrib(defsym) = 84;

                            /* i.e., ZERO references to compiler generated LABEL */
                            if (INFO_ECNT(abs(symAttrib(i))) == 1)
                                symF3(i) = ib;

                            symAttrib(i) += ia * 256;
                            for (;;) {
                                /* corrected reference count for object of the DEF */
                                /* merge the backstuffing chains */

                                if ((ia = l >> 2) == 0) {
                                    /* equate the defs */
                                    for (ia = 1; ia <= sytop; ia++)
                                        if (symbol[ia] == symbol[defsym])
                                            symbol[ia] = symbol[i];

                                    /* omit the TRA if no path to it */
                                    regv[RH] = defrh;
                                    regv[RL] = defrl;
                                    break;
                                } else {
                                    ib = getword(ia);
                                    // V4
                                    l               = (ib << 2) + (l & 3);
                                    symAddr(defsym) = -l;
                                    // V4
                                    putword(ia, j >> 2);
                                    // V4
                                    j          = ((ia << 2) + (j & 3));
                                    symAddr(i) = -j;
                                }
                            }
                        }
                        if (regv[RH] == (-255)) {
                            _delete(1);
                            return true;
                        }
                    }
                }

                if (it == 3 && iop == 1) { // BUG FIX user labels all have 4
                    /* we have a TRA to the outer block... */
                    j = C_STACKHANDLING;
                    if (prsp != 0 && j != 1) {
                        if (j == 0) {
                            j    = lxis;
                            lxis = codloc + 1;
                        }
                        emit(LXI, RSP, j % 65536);
                    }
                }
                j = -symAddr(i);
                m = j / 4;

                /* connect entry into chain */
                k = codloc + 1;
                if (iop == 4)
                    k = codloc;

                /* iop = 4 if we arrived here from case table JMP */
                // V4
                symAddr(i) = -(shl(k, 2) + right(j, 2));

                /* check for single reference */
                if (INFO_ECNT(abs(symAttrib(i))) == 1) {
                    // V4
                    //        note in a do case block, an implicit goto is
                    //        generated at the end of each selective statement
                    //        in order to branch out to the next statement
                    //        at the end of block. all these gotos are
                    //        considered a single forward reference for the
                    //        the convienance of the h/l optimization
                    //        ************************************************
                    //        make sure if saved h/l is already marked no good
                    //           1b   |   1b    |   9b    |   8b
                    //        h valid | l valid | h value | l value
                    //

                    int lsym = symF3(i);

                    /* PMO simplified the code to propagate HL info also fixed a bug in the
                       original Fortran code which could incorrectly subtract the valid flag
                       value from ktotal even if it hadn't been set.
                    */
                    if (lsym != -1) {
                        int ktotal = 0;
                        if (0 <= regv[RL] && regv[RL] < 256) {
                            ktotal = regv[RL];
                            if (lsym == 0 || ((lsym & LVALID) && regv[RL] == (lsym & 0xff)))
                                ktotal |= LVALID;
                        }
                        if (0 <= regv[RH] && regv[RH] < 512) {
                            ktotal += regv[RH] << 8;
                            if (lsym == 0 || ((lsym & HVALID) && regv[RH] == ((lsym >> 8) & 0x1ff)))
                                ktotal |= HVALID;
                        }
                        symF3(i) = ktotal & (HVALID | LVALID) ? ktotal : -1;
                    }
                }

                /* TRA, TRC, PRO, AX2 (case TRA) */
                switch (iop) {
                case 1:

                    /* may be INC TRA combination IN do-loop */
                    if (lastin + 1 == codloc) {
                        /* change to jfz to top of loop */
                        emit(JMC, FAL * 32 + ZERO, m);
                        _delete(1);
                        return true;
                    } else {
                        xfrloc = codloc;
                        xfrsym = st[sp];
                        tstloc = codloc + 3;
                        emit(JMP, m, 0);
                        _delete(1);
                        regv[RH] = regv[RL] = -255; /* mark as nil */
                        return true;
                    }
                    break;
                case 2:
                    conloc = codloc;
                    emit(JMC, iop2, m);
                    _delete(2);
                    return true;
                    ;
                case 3:
                    xfrloc = codloc;
                    xfrsym = st[sp];
                    tstloc = codloc + 3;
                    emit(CAL, m, 0);

                    /* adjust the maxdepth, if necessary */
                    // V4
                    j = symIProcDepth(i) + 1;
                    /* j is number of double-byte stack elements reqd */
                    stack(j);

                    /* now returned from call so... */
                    curdep[prsp] -= j;

                    /* now fix the h AND l values upon return */
                    // V4
                    j = symF3(i);
                    if (j < 0)
                        j = 0; // will force regv values to -1
                    /* may be unchanged from call */
                    if ((j >> 19) != 3) {
                        /* compare values */
                        regv[RL] = (j & LVALID) ? j & 0xff : -1;
                        regv[RH] = (j & HVALID) ? (j >> 8) & 0x1ff : -1;
                    }
                    _delete(1);

                    /* may have to construct a returned */
                    /* value at the stack top */
                    j = INFO_PREC(symAttrib(i));
                    if (j > 0) {
                        /* set stack top to precision of procedure */
                        sp++;
                        prec[sp] = j;
                        st[sp]   = 0;
                        i        = RC;
                        if (j > 1)
                            i = REGPAIR(RB, i);
                        rasn[sp] = i;
                        regs[RA] = RC;
                        regs[RC] = sp;
                        if (j > 1)
                            regs[RB] = sp;
                        litv[sp] = -1;
                    }
                    return true;
                case 4:

                    /* came from a case vector */
                    emit(0, m % 256, 0);
                    emit(0, m / 256, 0);
                    _delete(1);
                    return true;
                }
            }
        }
    } else if (iop != 1 || i != 0) { /* could be a computed address */
        error("134: invalid program transfer (only computed jumps are allowed with a 'go to')");
        sp--;
        return true;
    }

    /* jump to computed location */
    loadv(sp, 4);
    _delete(1);
    emit(PCHL, 0, 0);

    /* pc has been moved, so mark h AND l unknown */
    // V4
    regv[RH] = regv[RL] = -1;
    return true;
}

/*
    symbol[i] format
    0-7     RL val
    8-16    RH val  (note additional bit)
    17      RL valid
    18      RH valid
    19+     tbd if 3 then merge values of h and l
*/
void updateHL(int jp) {
    xfrloc   = codloc - 1;
    xfrsym   = 0;
    tstloc   = codloc;
    int i    = prstk[jp] % 65536;
    int dsym = regv[RH];
    int l    = regv[RL];
    int j    = symbol[i]; // masking not needed as implicit in code below
    int lp, kp;

    alter = 1; // hoisted here to simplify return
    if (j < 0)
        j = 0;
    if ((j & (LNOTSET | HNOTSET)) != (LNOTSET | HNOTSET)) {
        /* otherwise merge values of h AND l */
        lp = (j & LVALID) ? j & 0xff : -1;
        kp = (j & HVALID) ? (j >> 8) & 0x1ff : -1;
    } else if (dsym == -254 && l == -254)
        return;
    else { /* h AND l have been altered IN the procedure */
        kp = dsym;
        lp = l;
    }

    /* compare k with kp AND l with lp */
    j = (l >= 0 && lp == l) ? LVALID + l : 0;
    if (dsym >= 0 && kp == dsym)
        j += HVALID + (dsym << 8);
    symbol[i] = j;
    regv[RH] = regv[RL] = -255;
}

// update condition code for 16 bit, iq == 1 if zero test
void compare16(bool icom, int flag, int iq) {
    apply(SU, SB, icom, 1);
    int ip = rasn[sp] & 0xf;        /* change to condition code */
    int j  = (rasn[sp] >> 4) & 0x7; // > 7 would cause memory access error for lock etc.
    if (iq == 1)
        emit(OR, ip, 0);

    /* get rid of high order register in the result */
    regs[RA] = ip;
    rasn[sp] = flag + ip;
    prec[sp] = 1;
    litv[sp] = -1;
    st[sp]   = 0;
    if (j != 0) {
        lock[j] = false;
        regs[j] = 0;
        regv[j] = -1;
    }
    return;
}

// genrate call to builtin function bf, result determines result reg pair to use
// modified to use bf = 0->mutliply 1->divide/mod
void builtin(int bf, int result) {
    /* clear condition code */
    if (rasn[sp] > 255)
        cvcond(sp);

    /* clear pending store */
    if (regs[RA] != 0) {
        emit(LD, regs[RA], RA);
        regs[RA] = 0;
    }

    /* lock any correctly assigned registers */
    /* ....AND store the remaining registers. */
    if (REGLOW(rasn[sp]) == RE)
        lock[RE] = true;
    if (REGHIGH(rasn[sp]) == RD)
        lock[RD] = true;
    if (REGLOW(rasn[sp - 1]) == RC)
        lock[RC] = true;
    if (REGHIGH(rasn[sp - 1]) == RB)
        lock[RB] = true;
    saver();

    /* mark register c used. */
    if (regs[RC] == 0)
        regs[RC] = -1;

    /* load top of stack into registers d AND e. */
    loadv(sp, 0);
    if (prec[sp] == 1)
        emit(LD, RD, 0);

    /* now deassign register c unless correctly loaded. */
    if (regs[RC] == (-1))
        regs[RC] = 0;

    /* load t.o.s. - 1 into registers b AND c. */
    loadv(sp - 1, 0);
    if (prec[sp - 1] == 1)
        emit(LD, RB, 0);
    _delete(2);

    /* call the built-IN function */
    emitbf(bf);

    /* requires 2 levels IN stack for BIF (call AND temp.) */
    stack(2);
    ustack();
    ustack();

    /* AND then retrieve results */
    for (int dsym = 1; dsym <= 7; dsym++)
        lock[dsym] = false;

    /* cannot predict where registers h AND l will END up */
    regv[RH] = regv[RL] = -1;
    st[++sp]            = 0;
    prec[sp]            = 2;
    litv[sp]            = -1;
    if (result == 2) {
        rasn[sp] = REGPAIR(RD, RE);
        regs[RD] = regs[RE] = sp;
    } else {
        rasn[sp] = REGPAIR(RB, RC);
        regs[RB] = regs[RC] = sp;
    }
}

/* base may be indexed by ZERO... */
void inx(int jp) {
    int i;

    if (litv[sp] == 0) /* just _delete the index AND ignore the INX operator */
        _delete(1);
    else {
        if (rasn[sp] > 255)
            cvcond(sp);
        int j  = regs[RA];
        int il = REGLOW(rasn[sp]);
        int ih = REGHIGH(rasn[sp]);
        int jl = REGLOW(rasn[sp - 1]);
        int jh = REGHIGH(rasn[sp - 1]);

        /* check for pending store to base OR index */
        if (j && (j == jh || j == jl || j == ih || j == il)) {
            emit(LD, j, RA);
            regs[RA] = 0;
        }

        /* make sure that d AND e are available */
        if (regs[RE] != 0 || regs[RD] != 0)
            if (il != RE && jl != RE) { /* mark all registers free */
                int ia, ib, ic;
                if (il != 0)
                    regs[il] = 0;
                if (jl != 0)
                    regs[jl] = 0;
                genreg(2, &ia, &ib);
                regs[ia] = 1;
                genreg(2, &ic, &ib);
                regs[ia] = 0;

                /* all regs are cleared except base AND index, if allocated. */
                if (il != 0)
                    regs[il] = sp;
                if (jl != 0)
                    regs[jl] = sp - 1;
            }

        /* if literal 1 OR -1, use INX OR DCX */
        if (litv[sp] != 1 && litv[sp] != 65535) {
            /* if the index is constant, AND the base an address variable, */
            /* double the literal value at compile time */
            if (litv[sp] >= 0 && jp != 1) {
                litv[sp] *= 2;
                jp = 1;
            }
            i = 0;
            if (litv[sp] >= 0)
                i = 3;
            loadv(sp, i);
        }

        /* if the index was already in the registers, may */
        /* have to extend precision to address. */
        il = REGLOW(rasn[sp]);
        ih = REGHIGH(rasn[sp]);
        if (il != 0 && ih == 0) {
            ih = il - 1;
            emit(LD, ih, 0);
        }
        i = DAD;
        i = litv[sp] == 1 ? INCX : litv[sp] == 65535 ? DCX : DAD;
        if (ih == 0)
            ih = RH;
        /* _delete the index.  (note that sp will then point to the base) */
        _delete(1);
        /* load the base into the h AND l registers */
        loadv(sp, 5);
        /* ADD the base AND index */
        emit(i, ih, 0);
        /* AND ADD index again if base is an address variable. */
        if (jp != 1)
            emit(i, ih, 0);
        emit(XCHG, 0, 0);
        /* note XCHG here AND remove with peephole optimization later */
        i = prec[sp];
        _delete(1);
        st[++sp] = 0;
        prec[sp] = i;
        litv[sp] = regv[RH] = regv[RL] = -1;
        rasn[sp]                       = REGPAIR(RD, RE);
        regs[RD] = regs[RE] = sp;
    }
}

#define MAXLABEL 32
void sydump() {
    /* dump the symbol table for the simulator */
    /* clear the output buffer */
    putc('\n', lstFp);
    int len;

    while ((len = getc(symFp)) > 0) { /* process next symbol table entry */
        int symNo = getc(symFp);
        symNo += getc(symFp) * 256;

        /* write symbol number, symbol */
        char label[MAXLABEL + 1];
        memset(label, '.', MAXLABEL);

        fread(label, 1, len <= MAXLABEL ? len : MAXLABEL, symFp);
        if (len > MAXLABEL) {
            error("xx: symbol too long - truncated");
            fseek(symFp, len - MAXLABEL, SEEK_CUR);
        }
        label[MAXLABEL] = '\0';
        if (symNo == 5 || symNo > 6) { /* write hex address */
            int j    = symbol[symNo];
            int type = LOWNIBBLE(symbol[j - 1]);
            int ch   = symbol[j - 1] & 0xf;
            if (ch == PROC || ch == LABEL)
                j -= 2;
            int addr = abs(symbol[j]);
            if (C_HEXFILE)
                fprintf(hexFp, "%-5d %.*s %05XH\n", symNo, len, label, addr);
            if (C_MAP)
                fprintf(lstFp, "%s %04XH\n", label, addr);
        }
    }
    putc('\n', lstFp);

    if (len < 0)
        fatal("xx: premature EOF reading symbol file");
}

void cmpuse() {
    printf("table usage in pass 2:\n");
    printf("symbol table - max=%-5d, top=%-5d, info=%-5d\n", symax, sytop, syinfo);
    printf("memory usage - low=%04XH, high=%04XH\n", offset, codloc);
}
