#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>


#define BYTE(addr) (mem[addr])
#define WORD(addr) (*(uint16_t *)&mem[addr])
#define SYMP(addr)  ((sym_t *)&mem[addr])
#define _ENA       3 // Enable option
#define _DISA      0 // Disable option
#define _SPENA     1 // Special enable option
#define _PART      7 // Enable option partially
#define FALSE       0
#define TRUE        ~FALSE

#define NIL         0

#define TPA         0x100

#define eof         0x1a

#define LOMASK      0xf
#define COLMASK     0x7
#define NOMSB       0x7f
#define MSB         0x80
#define DSTmask     0x38
#define DSTxmsk     0x30
#define TYPMASK     0x7
#define HIMASK      0xf0
#define LENMASK     0xf

#define ArgMax      64
#define MaxBal      15
#define ExprDep     8 // stored as words
#define ExprCod     10
#define _ASCbyt     16
#define SymLen      16

#define _H          4
#define _A          7

#define SLen        16
#define FLen        128

#define IF_Dep      8 // Max IF levels

#define _irpc_      3
#define _irp_       5

#define LINLEN      120
#define PAGLEN      56


#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
    // String states

enum { _label = 1, _digit, _strg, _any, _error, _cmnt };

enum {
    _mul = 0, // *
    _div,     // /
    _mod,     // MOD
    _shl,     // SHL
    _shr,     // SHR
    _plus,    // +
    _minus,   // -
    _uminus,  // Unary -
    _eq,      // EQ
    _lt,      // LT
    _le,      // LE
    _gt,      // GT
    _ge,      // GE
    _ne,      // NE
    _not,     // NOT
    _and,     // AND
    _or,      // OR
    _xor,     // XOR
    _high,    // HIGH
    _low,     // LOW
    _lpar,    // (
    _rpar,    // )
    _comma,
    _cr,
    _nul,          // NUL
    _regs,         // Registers
    _pseudo,       // Pseudo codes
    _1byte = 0x1c, // One byte opcode
    _rp16,         // Register pair with 16 bit
    _rp,           // Register pair
    _pshp,         // Push and pop
    _jpcl,         // Jump and call
    _mov,          // Move
    _mvi,          // Move immediate
    _im8,          // Immediate 8 bit
    _lsax,         // Load and store by reg pair
    _lsd,          // Load and store direct
    _alur,         // Logical register
    _rop,          // Register operand
    _rpop,         // Register pair operand
    _rst,          // RST
    _io            // Input output
};
// Pseudo opcode - prefix 1ah ("_pseudo")

enum {
    _db = 1,
    _ds,
    _dw,
    _end,
    _endif,
    _endm,
    _equ,
    _if,
    _macro,
    _org,
    _set,
    _title,
    _else,
    _irp,
    _irpc,
    _rept,
    _aseg,
    _cseg,
    _dseg,
    _name,
    _page,
    _exitm,
    _extrn,
    _local,
    _inpage,
    _maclib,
    _public,
    _stkln
};

typedef struct mnemonic {
    char name[7];
    uint8_t tokId;
    uint8_t type;
    uint8_t code;
} mnemonic_t;

extern mnemonic_t *pMnem;

typedef struct {
    uint16_t de;
    uint16_t hl;
} pair_t;

typedef struct {
    uint8_t len;
    char str[1];
} pstr_t;


extern FILE *fpLIB;
extern char const *libFile;
extern FILE *fpSRC;
extern char const *srcFile;
extern FILE *fpPRN;
extern char const *prnFile;
extern char const *symFile;
extern FILE *fpHEX;
extern char const *hexFile;
extern bool InLIB; // true if from LIB
extern uint8_t curCh;
extern uint16_t SymTop;
extern uint16_t SymBot;
extern uint16_t TopPtr;
extern uint8_t Q_opt;
extern uint8_t L_opt;
extern uint8_t P_opt;
extern uint8_t R_opt;
extern uint8_t expandTab;
extern uint8_t S_opt;
extern uint8_t M_opt;
extern uint8_t _1_opt;
extern uint8_t PassNr;
extern char OutLine[120];
extern uint8_t OutLen;
extern uint8_t Balance;
extern uint16_t Titleptr;
extern uint16_t CurHEX;
extern uint8_t mem[];

/* mac.c */
int main(int argc, char **argv);
uint8_t MAC_loop(void);
uint8_t l02d6(void);
void StoreSymbol(void);
void LoadSymbol(void);
bool testMacro(void);
bool testEndInst(uint8_t ch);
bool testEndStmt(uint8_t ch);
void NewPage(void);
void l03c1(void);
void l03d7(void);
void l03db(void);
uint8_t MAC_sub(void);
bool IsMacro(uint8_t type);
bool l0722(uint8_t a);
void PutIF(uint8_t a);
uint8_t GetIF(void);
void SET_EQU(uint8_t a);
uint8_t macro(void);
uint8_t l0552(void);
uint8_t l0a7e(void);
uint8_t _ENDM_(void);
uint8_t __FALSE(uint8_t);
uint8_t MAC_Mnemo(void);
uint8_t GetDatDelim(void);
uint16_t GetOpr1(void);
uint8_t GetByteOper(void);
uint8_t CheckByteOper(uint16_t val);
uint8_t GetSrcReg(void);
uint8_t GetDstReg(void);
void GetRegPair(uint8_t code);
void GetImByte(void);
void GetImWord(void);
void GetComma(void);
uint8_t EndLine(uint8_t err);
uint8_t EndFile(bool err);
void finish(void);
void SetLocCtr(void);
bool IsActSym(void);
void CheckSymbol(void);
void PutCode(uint8_t val);
void PutWord(uint16_t val);
void StByte(uint8_t val);
void StWord(uint16_t val);
uint8_t Ch2Tok(uint8_t ch);
void WriteSymbols(void);
void StASC(uint8_t ch);
void l11e4(uint16_t de);
void l11ee(uint16_t hl);
void PushExpr(uint16_t val);
void St_Code_Prio(uint8_t code, uint8_t priority);
uint16_t PopExpr(void);
pair_t PopTwoOper(void);
void Term(uint8_t op);
bool IsExprDel(void);
bool IsExprDelOrComma(void);
void Expression(void);
void PushGood(uint16_t val);
void PushGood(uint16_t val);
uint8_t StChar(uint8_t ch);
bool GetId(void);
bool l16f3(void);
uint8_t RdChar(void);
void l17f1(void);
void IniEval(void);
void addCh(void);
bool IsLabCh(uint8_t ch);
bool IsLabel(uint8_t ch);
uint8_t IsValid(uint8_t ch);
uint8_t GetStrChr(void);
void skipEOL(void);
bool GetLibName(void);
void GetToken(void);
bool IsSpace(uint8_t ch);
bool IsDelimiter(void);
bool IsEOL(uint8_t ch);
bool IsDelim(uint8_t ch);
void l1afc(void);
void IniField_1(void);
void IniField_2(void);
void nest(void);
void unnest(void);
uint8_t GetHashIdx(pstr_t *p);
void l1e31(uint8_t len);
uint8_t SymSize(void);
bool l1e47(void);
bool SymAdr(void);
bool SymMArg(void);
bool SymSrc(void);
bool lookup(pstr_t *ps, uint16_t head);
void PostSym(void);
void l1f31(void);
void alloc(uint16_t bc);
void l1f87(void);
void l1fa5(void);
void PopParam(void);
void SetSymType(uint8_t type);
uint8_t GetSymType(void);
uint16_t GetSymValAddr(void);
void PutSymVal(uint16_t val);
uint16_t GetSymVal(void);
void NextSym(void);
void setParamLen(uint8_t val);
uint8_t getParamLen(void);
void l2065(void);
void GetSymbol(void);
uint8_t SymbolByte(void);
void PutSym(uint8_t ch);
void OpenLib(void);
void PrepLib(void);
void IniMAC(int argc, char **argv);
void IniLine(void);
int mgetc(void);
void fput_p(uint8_t ch);
void fput_x(uint8_t ch);
void Header(void);
void LstPage(uint8_t len);
void lstChar(char ch);
void emitLine(void);
void SetErr(uint8_t err);
void CloseLst(void);
void PrepSYM(void);
void FClose(void);
void putHEX(uint8_t ch);
void PutHexRecord(void);

/* macin.c */
struct mnemonic *GetMnemo(register const char *str, register size_t len);
char *getMnemStr(uint8_t code);
