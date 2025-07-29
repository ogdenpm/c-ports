#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef linux
#include <unistd.h>
#endif
#include "utility.h"

#define C_ANALYZE  contrl['A' - 'A'] // action/reduction trace
#define C_BYPASS   contrl['B' - 'A'] // bypass stack dump on error
#define C_GENERATE contrl['G' - 'A'] // show intermediate code generation
#define C_LINECNT  contrl['L' - 'A'] // line count start value
#define C_MEMORY   contrl['M' - 'A'] // dump symbol table (0 = no, 1 = yes)
#define C_PRINT    contrl['P' - 'A'] // controls printing on lst file
#define C_SYMBOLS  contrl['S' - 'A'] // control symbol dump
#define C_TAB      contrl['T' - 'A'] // tab size for listing file
#define C_UPPER    contrl['U' - 'A'] // convert string text to upper case
#define C_WIDTH    contrl['W' - 'A'] // listing file line width


#define LOWBYTE(n)    ((n) & 0xff)
#define HIGHBYTE(n)   LOWBYTE((n) >> 8)
#define LOWNIBBLE(n)  ((n) & 0x0f)
#define HIGHNIBBLE(n) LOWNIBBLE((n) >> 4)

#define ASIZE(x)      (int)(sizeof(x) / sizeof((x)[0]))



/* pol types */
enum { OPR = 0, ADR, VLU, DEF, LIT, LIN, FIN};
// clang-format off
enum {
    NOP = 0,
    ADD, ADC, SUB, SBC, MUL, DIV, REM, NEG, AND, IOR,
    XOR, NOT, EQL, LSS, GTR, NEQ, LEQ, GEQ, INX, TRA,
    TRC, PRO, RET, STO, STD, XCH, DEL, DAT, LOD, BIF,
    INC, CSE, END, ENB, ENP, HAL, RTL, RTR, SFL, SFR,
    HIV, LOV, CVA, ORG, DRT, ENA, DIS, AX1, AX2, AX3
};
// clang-format on

// plm81.c
extern int errorCnt;
extern int contrl[]; // control flags
extern int curblk;   // current block number
extern bool failsf;  // not failed
extern bool compil;  // still compiling
struct _symbol {
    uint16_t hcode;          // hash code
    uint16_t next;           // next symbol in chain
    uint16_t outOfScope : 1; // non zero if out of scope
    uint16_t strLen : 15;    // symbol string length
    uint16_t strId;          // symbol string Id
    union {
        uint16_t len; // symbol info length field
        uint16_t refCnt;
        uint16_t dim;
        uint16_t iVal;
    };
    uint8_t prec;            // symbol precision
    uint8_t type;            // symbol type
    uint16_t based;          // based symbol Id
};
extern struct _symbol symbol[];


extern int symNext;
extern uint32_t initialData[];
extern uint16_t initialDataSP;
extern uint16_t intpro[];

// io.c
extern FILE *srcFp;
extern FILE *polFp;
extern FILE *symFp;
extern FILE *lstFp;

// lexer.c
extern uint16_t acclen;
extern char accum[];
extern int accumIVal;
extern int token;
extern const char *tokens[];
extern uint16_t macdefSP;

// prod.c
extern char *productions[];

// tables.c
/* syntax analyzer tables*/
#define TRI(a, b, option) (((a) << 16) + ((b) << 8) + option)


extern const uint8_t c1[][13];
extern const int c1tri[243];
extern const int prtb[];
extern const uint8_t prdtb[];
extern const uint8_t hdtb[];
extern const uint8_t prlen[];
extern const uint8_t contc[];
extern const uint8_t leftc[];
extern const uint8_t lefti[];
extern const uint8_t prind[];
enum { nt = 50 };



    // shared definitions
// clang-format off
enum {
    SEMIV = 1, DOV = 15,   EOFILE = 20, ENDV = 21,   CALLV = 30, DECL = 42, NUMBV = 45,
    STRV = 46, PROCV = 48, IDENTV = 50, GROUPV = 55, STMTV = 65, SLISTV = 82
};


enum { P_LABEL, P_BYTE, P_ADDRESS, P_INLINE };      // Data prec values
enum { OuterLabel = 3, LocalLabel, CompilerLabel }; // LABEL prec values

/* types */
enum { VARB = 1, INTR, PROC, LABEL, LITER, NUMBER };
// clang-format on

/* plm81.c */
unsigned iabs(int a);
void doHead(int slot, int type, int clause);
void setInfo(int symIdx, uint16_t len, uint8_t prec, uint8_t type);
void openfiles(char *srcFile);
void closefiles(void);
int main(int argc, char **argv);
void dumpsym(int idx);
void exitb(void);
int varHash(char *str, int len);
int lookup(const int iv);
int enter(uint16_t len, uint8_t prec, uint8_t type, bool hasHash);
void install(void);
void dumpsy(void);
void recov(void);
int intcmp(const void *a, const void *b);
bool stack(void);
bool prok(const int prd);
void reduce(void);
void cloop(void);
int getc1(int stackItem, int token);
void scan(void);
int wrdata(const int sy);
void dumpch(void);
void procHead(int plist, int rtype);
void synth(const int prod, const int curTokId);
char *readline(FILE *srcFp);
int gnc(void);
void parseOptions(char *s);
void decibp(void);
void stackc(char *fname);
void enterb(void);
void dumpin(void);
void fatal(char const *fmt, ...);
void error(char const *fmt, ...);
void showTopTokens(int start, int col);
void sdump(void);
void redpr(const int prod, const int sym);
void emit(int val, int typ);

/* lexer.c */
int newString(uint16_t len, const char *str);
bool strequ(int strId, char const *str, int len);
char *idToStr(int loc);
bool defMacro(int idLen, char *id, int valLen, char *val);
bool useMacro(int len, char *str);
void dropMacro(int newTop);
int macGetc(void);
