#include "utility.h"
#include <assert.h>
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

/* types */
enum { VARB = 1, INTR, PROC, LABEL, LITER, NUMBER };

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


/* ilcod */
// clang-format off
enum { OPR = 0, ADR, VLU, DEF, LIT, LIN, FIN };
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
// flag values stored  in rasn bits 8-12
#define ZFLAG             ((16 + 2) << 8)
#define NZFLAG            (2 << 8)
#define CFLAG             ((16 + 1) << 8)
#define NCFLAG            (1 << 8)

#define HIGHNIBBLE(n)     (((n) >> 4) & 0xf)
#define LOWNIBBLE(n)      ((n) & 0xf)
#define REGPAIR(h, shift) (((h) << 4) + (shift))
#define REGLOW(hl)        ((hl) & 0x7)
#define REGHIGH(hl)       (((hl) >> 4) & 0x7)
#define HIGH(n)           (((n) >> 8) & 0xff)
#define LOW(n)            ((n) & 0xff)
#define HIGHWORD(n)       (((n) >> 16) & 0xffff)
#define LOWWORD(n)        ((n) & 0xffff)

// HL tracking uses the following flags
// the LVALUE is in bits 0-7, the HVALUE is in bits 8-16 (9 bits wide)
#define LVALID            0x20000
#define HVALID            0x40000
#define LNOTSET           0x80000
#define HNOTSET           0x100000

extern int outloc;
extern int firsti;
extern int casjmp;

/* rgmapp */
extern int const regmap[9 + 1];

extern int errorCnt;
extern bool errflg;
extern int C_ANALYSIS;
extern int C_COUNT;   // line counter
extern bool C_FINISH; // dump code at finish
extern int C_GENERATE;
extern int C_LOAD; // load address of program
extern bool C_MAP;
extern bool C_NUMERIC;
extern bool v4Opt;
extern int C_SYMBOLS;
extern int C_VARIABLES;
extern int C_WIDTH;
extern bool C_HEXFILE;
extern int C_STACKHANDLING; // 0->compiler handles stack pointer
                            // 1 - programmer handles stack pointer
                            // > 1 n is stack size

extern uint16_t intpro[8]; // interrupt procedure names
extern int symbol[MAXSYM + 1];
extern int symax;
extern int sytop;
extern int syinfo;
extern int lmem;
extern int memtop;
extern uint8_t mem[MAXMEM]; // upto max memory of 8080
extern int codeBase;
extern int entry; // entry point for hex trailer
extern int preamb;
extern int codloc;
extern bool alter;
extern int lapol; // current position in pol file
extern int sp;    // stack pointer for register allocation
typedef struct {
    int32_t st;
    int32_t litv;
    uint16_t rasn;
    uint8_t prec;
} regAlloc_t;
extern regAlloc_t regAlloc[16 + 1];





extern FILE *hexFp;
extern FILE *outFp;
extern FILE *polFp;
extern FILE *inFp;
extern FILE *lstFp;
extern FILE *symFp;
char *src;

extern uint8_t multiply[];
extern uint8_t divide[];

typedef struct {
    uint8_t extra;
    uint8_t opcode[7];
} ctran_t;

extern ctran_t ctran[];



/* plm82.c */
void closefiles(void);
void openfiles(char *srcFile);
int parseNum(char const *s);
int main(int argc, char **argv);
uint8_t get(uint32_t ip);
uint16_t getword(int ip);
void put(uint32_t ip, uint8_t val);
void putword(uint32_t ip, uint16_t val);
void error(char const *fmt, ...);
void fatal(char const *msg, ...);
int shr(const int i, const int j);
int shl(const int i, const int j);
int right(const int i, const int j);
void pop(int n);
void apply(int op, int op2, bool com, int cyflag);
void genreg(const int np, int *ia, int *ib);
int32_t getSym32(void);
uint16_t getSym16(void);
void loadsy(void);
void loadv(int s, int typ);
void setadr(const int val);
void ustack(void);
int chain(const int sy, const int loc);
void gensto(const int keep);
void litadd(const int s);
void dump(int lp, const int u, bool symbolic);
void emit(int opr, int opa, int opb);
void puncod(int start, const int end);
void puntrailer(void);
void cvcond(const int s);
void saver(void);
void reloc(void);
void put2(int ch1, int ch2, int prec);
void put1(int ch, int prec);
int loadin(void);
void emitbf(int bf);
int32_t getNextPol(void);
void inldat(void);
void unary(const int val);
void exch(void);
void stack(const int n);
void readcd(void);
bool doBuiltin(int val);
bool operat(int val);
void updateHL(int jp);
void compare16(bool icom, int flag, int iq);
void builtin(int bf, int result);
void inx(int jp);
void sydump(void);
void cmpuse(void);
