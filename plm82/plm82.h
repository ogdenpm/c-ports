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


#define Addr(n)                        (n)
#define Attrib(n)                      ((n) - 1)
#define Ref(n)                         ((n) - 2)
#define TrackHL(n)                     ((n) - 3)
#define IProcDepth(n)                  ((n) - 4)

#define symAddr(ch)                    symbol[Addr(symbol[ch])] // < 0 abs address >= 0 relocatable address
#define symAttrib(ch)                  symbol[Attrib(symbol[ch])]
#define symRef(ch)                     symbol[Ref(symbol[ch])]
#define symTrackHL(ch)                 symbol[TrackHL(symbol[ch])]
#define symIProcDepth(ch)              symbol[IProcDepth(symbol[ch])]
// VARB e..e ssss 0001   e..e - number of elements, ssss size of element
#define INFO_TYPE(a)                   (abs(a) & 0xf)
#define INFO_PREC(a)                   ((abs(a) >> 4) & 0xf)
#define INFO_ECNT(a)                   (abs(a) >> 8)
#define PACK_ATTRIB(extra, prec, type) ((extra) * 256 + (prec) * 16 + type)
#define MAXSYM                         16000
#define MAXMEM                         0x10000

/* types */
enum { VARB = 1, INTR, PROC, LABEL, LITER, NUMBER };

// clang-format off
enum {  // ops
    LD = 1, IN,   DC,   AD,  AC,   SU,   SB,   ND,  XR,  OR,
    CP,     ROT,  JMP,  JMC, CAL,  CLC,  RTN,  RTC, RST, INP,
    OUT,    HALT, STA,  LDA, XCHG, SPHL, PCHL, CMA, STC, CMC,
    DAA,    SHLD, LHLD, EI,  DI,   LXI,  PUSH, POP, DAD, STAX,
    LDAX,   INCX, DCX
};
enum { RA = 1, RB = 2, RC = 3, RD = 4, RE = 5, RH = 6, RL = 7, RSP = 9, ME = 8 };
enum { 
    // modified to match bits used in ROT group
    LFT   = 0,  RGT  = 8, CY = 0, ACC = 16,
    // modified to match the PLM81 values
    CARRY = 1, ZERO = 2, SIGN = 3, PARITY = 4,
    // TRU matches conditional bit
    TRU = 0x8, FAL = 0
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
    B_ROL = 0, B_ROR,     B_SHL,    B_SHR,     B_SCL,  B_SCR,  B_TIME,   B_HIGH,
    B_LOW,     B_INPUT,   B_OUTPUT,  B_LENGTH, B_LAST, B_MOVE, B_DOUBLE, B_DEC
};


// predefined symbol numbers
enum {
    S_CARRY = 1, S_ZERO, S_SIGN, S_PARITY, S_MEMORY, S_STACKPTR, S_ROL, S_ROR,
    S_SHL, S_SHR, S_SCL, S_SCR, S_TIME, S_HIGH, S_LOW, S_INPUT, S_OUTPUT,
    S_LENGTH, S_LAST, S_MOVE, S_DOUBLE, S_DEC
};

enum { OuterLabel = 3, LocalLabel, CompilerLabel }; // LABEL prec values


// clang-format on
// flag values stored  in rasn bits 8-12
#define ZFLAG                ((TRU + ZERO) << 8)
#define NZFLAG               ((FAL + ZERO) << 8)
#define CFLAG                ((TRU + CARRY) << 8)
#define NCFLAG               ((FAL + CARRY) << 8)

#define HIGHNIBBLE(n)        (((n) >> 4) & 0xf)
#define LOWNIBBLE(n)         ((n) & 0xf)
#define ASSIGN(cond, rh, rl) (((cond) << 8) + ((rh) << 4) + (rl))
#define REGPAIR(rh, rl)      ASSIGN(0, rh, rl)
#define REGLOW(hl)           ((hl) & 0x7)
#define REGHIGH(hl)          (((hl) >> 4) & 0x7)
#define COND(assignment)     HIGH(assignment)
#define HIGH(n)              (((n) >> 8) & 0xff)
#define LOW(n)               ((n) & 0xff)
#define HIGHWORD(n)          (((n) >> 16) & 0xffff)
#define LOWWORD(n)           ((n) & 0xffff)

// HL tracking uses the following flags
// the LVALUE is in bits 0-7, the HVALUE is in bits 8-16 (9 bits wide)
#define LVALID               0x20000
#define HVALID               0x40000
#define LNOTSET              0x80000
#define HNOTSET              0x100000

extern int casjmp;

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
extern uint8_t mem[MAXMEM]; // upto max memory of 8080
extern int codeBase;
extern int entry; // entry point for hex trailer
extern int preamb;
extern int codloc;
extern bool alter;
extern int lapol; // current position in pol file
extern int8_t sp;    // stack pointer for register allocation

typedef struct {
    uint16_t assignment; // (cond << 8) | (rh << 4) | rl
    uint8_t prec;        // precision of variable
                         // for based ((based var precision) << 2) + 2
    int16_t st;          // < 0 symbol value ref, > 0 symbol address ref
    int32_t litv;        // 0-0xffff -> literal value, < 0 not a literal
                         // >= 0x10000 relocatable value

} regAlloc_t;

extern regAlloc_t parseStk[16 + 1];
regAlloc_t stkItem(uint16_t assignment, uint8_t prec, int16_t st, int32_t litv);

extern FILE *hexFp;
extern FILE *outFp;
extern FILE *polFp;
extern FILE *inFp;
extern FILE *lstFp;
extern FILE *symFp;
extern char *src;

typedef struct {
    uint8_t extra;
    uint8_t opcode[7];
} ctran_t;

extern ctran_t const ctran[];
extern int polCnt;

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
void Fatal(char const *msg, ...);
int right(int i, int j);
void pop(int n);
void apply(int op, int op2, bool com, bool cyflag);
uint16_t genReg(int8_t np);
void genreg(int np, int *ia, int *ib);
int32_t getSym32(void);
uint16_t getSym16(void);
void loadsy(void);
void loadv(int s, int typ);
void setadr(int val);
void ustack(void);
int chain(uint16_t sy, int loc);
void genStore(bool keep);
void litadd(int s);
void dump(int lp, int u, bool symbolic);
void emit(int opr, int opa, int opb);
void puncod(int start, int end);
void puntrailer(void);
void cvcond(int s);
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
void compare16(bool icom, int flag, bool zeroTest);
void builtin(int bf, int targetReg);
void indexOp(int jp);
void sydump(void);
void cmpuse(void);
