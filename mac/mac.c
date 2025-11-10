
#include "mac.h"
#include "utility.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STMTLOOP 1
#define LINELOOP 2
#define PASSLOOP 3
#define FINISH   4

uint8_t l11c2;
uint16_t label;
uint16_t l11c5;
uint16_t XFerAdr; // Transfer address
uint8_t outCol;
uint8_t l11ca;
uint8_t ifNesting;
uint8_t macroNesting;
uint8_t ifLevel;         // IF level
uint8_t ifStack[IF_Dep]; // IF level
uint16_t macroChain;
uint16_t l11d8;
uint8_t supp_0;    // Leading zero flag
uint16_t val_16;   // Current 16 bit value
uint16_t localCnt; // Local count
uint16_t afterComma;

uint8_t prevCh;
uint8_t ValBase;
uint16_t l1613;
uint8_t chevronCnt;

uint8_t F_CHR;

// Structure of symbol table
typedef struct {
    uint16_t link;
    uint8_t typeSize;
    uint8_t name[1];
    // rest of name
    // uint16_t val;
} sym_t; // sizeof symbol = 5 + len(name)

uint8_t _1_opt;

uint16_t macHashTab[SLen];
uint8_t Balance;

struct {
    uint8_t collectMode;
    uint16_t topPtr;
    uint16_t l2ed4;
    uint16_t l2ef4;
    uint8_t curCh;
    uint16_t prevTopPtr;
    uint8_t l2f44; // not used?
    uint8_t ifLevel;
} Stack[16];

// overlapped data area for symbol sorting
uint8_t l2ea4;
uint8_t l2ea6;
uint8_t l2ea7;
uint16_t symList[28];
uint8_t expandChIdx;
uint8_t lookAhead;
struct {
    uint8_t len;
    uint8_t str[37 + 1]; // extra byte for zero termination
} PrnLine;


char OutLine[120];

uint8_t OutLen;
uint8_t tokType;
uint16_t Value;
struct {
    uint8_t len;
    char str[ArgMax + 1]; // extra byte for zero termination
} token;

uint16_t ExprVal; // Expression value;

uint16_t SymTop;
uint16_t topPtr;
uint8_t PassNr;
uint16_t CurHEX;
uint16_t LocCtr;

uint16_t SymBot;
uint16_t curSym;
uint16_t CurSym;

uint8_t collectDepth;
uint8_t curCh;
uint8_t collectMode;
bool InLIB; // true if from LIB
uint8_t S_opt = _SPENA;
uint8_t M_opt = _SPENA;
uint16_t startMacroBody;
char const *Titleptr;
uint8_t Q_opt;
uint8_t L_opt;
uint8_t P_opt;
uint8_t R_opt;
uint8_t expandTab;

mnemonic_t *pMnem;

FILE *fpLIB;
char const *libFile;
FILE *fpSRC;
char const *srcFile;
FILE *fpPRN;
char const *prnFile;
FILE *fpHEX;
char const *hexFile;
char const *symFile;

// Cold entry

uint8_t mem[0x10000];

char const help[] =
    "Usage: %s [options] sourceFile[.asm]\n"
    "Where options are\n"
    "  -p file[.prn]  override listing file. Default sourceFile.prn\n"
    "  -s file[.sym]  override symbol file. Default sourcefile.sym\n"
    "  -h file[.hex]  override hex file. Default sourcfile.hex\n"
    "  -c controls+   set mac control options\n"
    "  -t[=n]         expand tabs to every nth column. If n missing then 8 is used\n"
    "Notes:\n"
    " * file is the filename or - for standard output or . to disable output\n"
    "   If no extent is specified then the relevant extent is added.\n"
    " * controls are as per MAC (+|-|*|x)(1|L|M|Q|R|S). lowercase letters are also supported\n"
    "   Due to OS shell processing, controls may need to be quoted to allow * and spaces\n"
    "   Note x is an alias for *. If quotes are not used this will avoid shell wild card "
    "expansion\n";

int main(int argc, char **argv) {
    PassNr       = 0;
    collectDepth = 0;
    IniMAC(argc, argv);
    IniField_1();
    macroChain  = 0;

    uint8_t ret = PASSLOOP;
    while (ret != FINISH) {
        IniField_2();
        Balance              = 0;
        Stack[0].collectMode = 0;
        Stack[0].prevTopPtr  = topPtr;
        l17f1();
        IniPass();
        localCnt = label   = 0;
        XFerAdr = LocCtr = CurHEX = R_opt ? TPA : 0;
        ifLevel                   = 0;
        // MAC_Mainloop:
        do {
            GetToken();
        } while ((ret = MAC_loop()) == LINELOOP);
    }
    finish();
    exit(0); // all done
}

uint8_t MAC_loop() {
    while (1) {
        uint8_t ret;
        if (tokType == _number)
            return LINELOOP;
        if ((ret = MAC_sub()) != STMTLOOP)
            return ret;
    }
}

uint8_t l02d6() {
    l11c2               = getParamLen();
    uint16_t saveTopPtr = topPtr;
    if (l11c2 != 0) {
        while (1) {
            uint8_t next = 0;
            uint8_t a;
            if (tokType == _punct) {
                a = token.str[0];
                if (testEndInst(a))
                    break;
                if (a == '%') {
                    val_16 = getWordValue();
                    if (tokType != _punct) {
                        SetErr('S');
                        break;
                    }
                    a         = token.str[0];
                    token.len = sprintf(token.str, "%d", val_16);
                    l03db();
                    next = 1;
                } else if (token.str[0] == ',') {
                    l03d7();
                    next = 2;
                }
            }
            if (next == 0) {
                l03db();
                GetToken();
                if (tokType != _punct) {
                    SetErr('S');
                    break;
                }
                a = token.str[0];
            }
            if (next <= 1 && a != ',') {
                if (!testEndInst(a))
                    SetErr('S');
                break;
            }
            if (l11c2 == 0)
                break;
            l1afc();
        }
    }

    while (l11c2 != 0)
        l03d7();
    uint16_t tmp = CurSym + 1;

    while (!testEndStmt(curCh))
        GetToken();
    curCh          = 0;
    Stack[0].curCh = 0;
    NewPage();
    Stack[0].ifLevel = ifLevel;
    nest();
    Stack[0].l2ef4       = tmp;
    Stack[0].prevTopPtr  = saveTopPtr;
    Stack[0].curCh       = 0;
    Stack[0].collectMode = 1;
    return LINELOOP;
}

void StoreSymbol() {
    uint16_t tmp = SymTop;
    CurSym       = SymTop;
    for (uint8_t i = 0; i < token.len; i++)
        PutSym(token.str[i]);
    BYTE(SymTop = tmp) = token.len;
}

void LoadSymbol() {
    token.len = BYTE(SymTop);
    CurSym    = SymTop;
    for (uint8_t i = 0; i < token.len; i++)
        token.str[i] = SymbolByte();
}

bool testMacro() {
    return tokType == _error && (pMnem = GetMnemo(token.str, token.len)) &&
           pMnem->type == _pseudo && pMnem->code == _macro;
}

bool testEndInst(uint8_t ch) {
    return ch == ';' || ch == '\r' || testEndStmt(ch);
}

bool testEndStmt(uint8_t ch) {
    return ch == '\n' || ch == cpmEOF || ch == '!';
}

void NewPage() {

    if (Balance)
        OutLine[5] = '+';
    emitLine();
    OutLen = _ASCbyt;
}

void l03c1() {
    while (Balance) {
        if (Stack[0].collectMode >= _irpc_ || Stack[0].collectMode == _macro_)
            return;
        unnest();
    }
    SetErr('B');
}

void l03d7() {
    token.len = 0;
    l03db();
}

void l03db() {
    l1f87();
    GetSymbol();
    l1fa5();
    l11c2--;
}

uint8_t MAC_sub() {
    uint8_t cval;
    if (tokType == _punct) {
        if (token.str[0] == '$') {
            if (hasLabel())
                return EndLine('S');
            if (curCh == '-')
                cval = _DISA;
            else if (curCh == '+')
                cval = _ENA;
            else if (curCh == '*')
                cval = _PART;
            else
                return EndLine('S');
            GetToken();
            if (curCh == 'M')
                M_opt = cval;
            else if (curCh == 'P')
                P_opt = cval;
            else
                return EndLine('S');
            GetToken();
        }
        return EndLine(0);
    } else if (tokType != _id)
        return EndLine('S');
    if ((pMnem = GetMnemo(token.str, token.len)))
        return MAC_Mnemo();
    if (!SymSrc()) { // new symbol
        PostSym();
        if (PassNr)
            SetErr('P');                  // e39
    } else if (GetSymType() == T_MACRO) { // 3e9
        uint16_t l11e0 = 0;
        if (PassNr) {
            while (GetSymVal() > curSym) { // 23c
                l11e0 = curSym;
                lookup((pstr_t *)&token, WORD(curSym));
                if (curSym) {
                    if (GetSymType() != T_MACRO)
                        return EndLine('P');
                } else {
                    l1afc();
                    if (!testMacro() || l11e0 != l11d8)
                        return EndLine('P');
                    label = l11d8;
                    return macro();
                }
            }
        }
        StoreSymbol();
        l1afc();
        if (!testMacro()) {
            uint16_t tmp = curSym; // l02c1
            CheckLabel(LocCtr);
            if (M_opt == _DISA)
                StWord(LocCtr);
            curSym = tmp;
            return l02d6();
        }
        if (label)
            SetErr('S');
        if (PassNr) {
            if (l11e0 != l11d8)
                return EndLine('P');
            label = l11d8;
        } else {
            LoadSymbol();
            GetHashIdx((pstr_t *)&token);
            PostSym();
            label = curSym;
        }
        return macro();
    }
    if (label)
        SetErr('L');
    label = curSym;
    GetToken();
    if (tokType != _punct || token.str[0] != ':')
        return STMTLOOP;
    return LINELOOP;
}

bool IsMacro(uint8_t type) {
    return type == _macro || type == _rept || type == _irp || type == _irpc;
}

bool collectMaroBody(uint8_t mode) {
    collectMode = mode;
    while (1) {
        if (tokType == _punct) {
            if ((token.str[0] == '\r' || token.str[0] == '!'))
                break;
            else if (token.str[0] == cpmEOF)
                return false;
        }
        SetErr('S');
        GetToken();
    }

    startMacroBody = CurSym;
    collectDepth   = 1;
    GetToken();

    while (1) {
        uint16_t tokStart = CurSym;
        GetToken();
        if (tokType == _punct && token.str[0] == cpmEOF)
            return false;
        if (tokType == _id && (pMnem = GetMnemo(token.str, token.len))) {
            if (collectMode == _macro_ && PassNr == 0 && token.len != 1 && pMnem->tokId) {
                CurSym = tokStart;
                PutSym(pMnem->tokId | 0x80); // replace token with its id
                if (curCh)                   // append prepread character
                    PutSym(curCh);
            }
            if (pMnem->type == _pseudo) {
                if (IsMacro(pMnem->code)) {
                    if (collectDepth++ == 0xff)
                        return false;
                } else if (pMnem->code == _endm && --collectDepth == 0)
                    break;
            }
        }
    }
    if (collectMode == _macro_) {
        label = 0;
        if (InLIB)
            PutSymVal(0);
        else {
            PutSymVal(macroChain);
            macroChain = curSym;
        }
        if (PassNr)
            return true;
    }
    if (BYTE(CurSym) != '\r')
        SetErr('S');
    BYTE(CurSym) = '\r';
    PutSym(0);
    return true;
}

void PutIF(uint8_t a) {
    if (ifLevel >= IF_Dep)
        SetErr('O');
    else
        ifStack[ifLevel++] = a;
}
uint8_t GetIF() {
    if (ifLevel == 0) {
        SetErr('B');
        return 0;
    } else
        return ifStack[--ifLevel];
}

void SET_EQU(uint8_t a) {
    uint16_t tmp = LocCtr;
    StWord(LocCtr = getWordValue());
    LocCtr     = tmp;
    OutLine[6] = a;
    return;
}

uint8_t macro() {
    if (!hasLabel()) {
        SetErr('L');
        return EndLine(0);
    }
    if (PassNr != 0) {
        if (curSym != l11d8)
            SetErr('P');
        else
            l11d8 = GetSymVal();
    } else
        SetSymType(T_MACRO);
    uint8_t paramterCnt = 0;
    if (PassNr == 0)
        setParameterCnt(0);
    while (GetToken(), tokType == _id) {
        if (PassNr == 0)
            addParameter();
        paramterCnt++;
        GetToken();
        if (tokType != _punct || token.str[0] != ',')
            break;
    }
    if (!collectMaroBody(_macro_))
        return EndFile(true);
    if (PassNr == 0)
        setParameterCnt(paramterCnt);
    GetToken();
    return EndLine(0);
}

uint8_t l0552() {
    uint16_t ptr;
    if (Stack[0].collectMode == _endm) {
        uint16_t hl = Stack[0].topPtr;
        if ((ptr = WORD(hl)) != 0) {
            WORD(hl)       = ptr - 1;
            OutLen         = _ASCbyt;
            Stack[0].l2ef4 = Stack[0].l2ed4;
            curCh          = 0;
            return LINELOOP;
        }
    } else {
        uint16_t hl = Stack[0].topPtr;
        ptr         = WORD(hl);
        if (BYTE(ptr) != '\r') {
            if (BYTE(ptr) != 0) {
                if (Stack[0].collectMode != _irpc_) {
                    uint8_t saveLookAhead = lookAhead;
                    lookAhead             = 0;
                    uint8_t saveCh        = curCh;
                    curCh                 = 0;
                    Stack[0].l2ef4        = ptr;
                    if (BYTE(ptr) == ',') {
                        ptr++;
                        token.len = 0;
                        l1f87();
                        l11ee(ptr); // 5f0
                        PrnLine.len = 0;
                    } else {
                        l1afc();
                        l11e4(ptr);
                        l1f87(); // 5d0
                        ptr = Stack[0].l2ef4;
                        if (BYTE(ptr) == 0)
                            BYTE(ptr) = '\r';
                        else {
                            ptr = afterComma;
                            GetToken();
                            if (token.str[0] != ',')
                                SetErr('S');
                            l11ee(ptr); // 5f0
                            PrnLine.len = 0;
                        }
                    }
                    WORD(Stack[0].topPtr) = ptr; // 5f7
                    curCh                 = saveCh;
                    lookAhead             = saveLookAhead;
                } else {
                    WORD(hl)     = ptr + 1;
                    token.len    = 1;
                    token.str[0] = BYTE(ptr);
                    l1f87();
                }
                CurSym = Stack[0].topPtr + 1; // 606
                GetSymbol();
                l1fa5();
            } else {
                BYTE(ptr) = '\r';
                token.len = 0;
                l1f87();
            }
            OutLen         = _ASCbyt; // 635
            Stack[0].l2ef4 = Stack[0].l2ed4;
            curCh          = 0;
            return LINELOOP;
        }
    }
    NewPage();                    // 616
    topPtr = Stack[0].prevTopPtr; // Restore top
    unnest();
    ifLevel = Stack[0].ifLevel;
    if ((curCh = Stack[0].curCh))
        StChar(curCh);
    return LINELOOP;
}

uint8_t l0a7e() {
    NewPage();
    GetToken();
    Stack[0].ifLevel = ifLevel;
    Stack[0].curCh   = curCh != '\n' ? curCh : 0;
    nest();
    Stack[0].prevTopPtr = topPtr;
    while (CurSym >= l11c5)
        BYTE(--topPtr) = BYTE(CurSym--);
    SymTop          = CurSym + 1;
    Stack[0].topPtr = topPtr;
    if (collectMode != _rept_)
        WORD(topPtr) = topPtr + BYTE(topPtr);
    Stack[0].l2ed4       = topPtr + (curSym - l11c5);
    Stack[0].collectMode = collectMode;
    return l0552();
}

uint8_t _ENDM_() {
    CheckLabel(LocCtr);
    l03c1();
    OutLine[5] = '+';
    if (Stack[0].collectMode < _irpc_) // macro
        PopParam();
    else {
        uint16_t tmp        = Stack[0].prevTopPtr;
        Stack[0].prevTopPtr = Stack[0].topPtr;
        PopParam();
        Stack[0].prevTopPtr = tmp;
        if (pMnem->code == _endm)
            return l0552();
    }
    NewPage();
    topPtr = Stack[0].prevTopPtr; // Restore top
    unnest();
    ifLevel = Stack[0].ifLevel;
    if ((curCh = Stack[0].curCh))
        StChar(curCh);
    return LINELOOP;
}

uint8_t __FALSE(uint8_t l11ca) {
    macroNesting = ifNesting = 0;
    while (1) {
        while (1) {
            if (tokType == _punct) {
                if (token.str[0] == '\r') {
                    GetToken(); // eat '\n'
                    break;
                } else if (token.str[0] == '!')
                    break;
                else if (token.str[0] == cpmEOF)
                    return EndFile(true);
            }
            GetToken();
        }
        GetToken();
        if (tokType == _number) // leading line number, so ignore
            GetToken();
        if (tokType != _id)
            continue;
        if (!(pMnem = GetMnemo(token.str, token.len))) {
            GetToken();
            if (tokType == _punct) {
                if (token.str[0] != ':') // not a label definition
                    continue;
                GetToken();
            }
            if (tokType != _id || !(pMnem = GetMnemo(token.str, token.len)))
                continue;
        }
        if (pMnem->type != _pseudo)
            continue;
        if (pMnem->code == _if) {
            if (ifNesting++ == 255) // way too deep
                SetErr('O');
            continue;
        } else if (pMnem->code == _else) {
            if (ifNesting)
                continue;
            if (l11ca == _else_)
                SetErr('B');
            PutIF(_else_);
            GetToken();
        } else if (pMnem->code == _endif) {
            if (ifNesting--)
                continue;
            if (macroNesting)
                SetErr('B');
            GetToken();
        } else if (IsMacro(pMnem->code)) {
            if (macroNesting++ == 0xff)
                SetErr('O');
            continue;
        } else if (pMnem->code != _endm || macroNesting--)
            continue;
        else
            return _ENDM_();
        return EndLine(0);
    }
}

uint8_t MAC_Mnemo() {
    if (pMnem->type == _pseudo) {
        switch (pMnem->code) {
        case _db:
            CheckLabel(LocCtr);
            do {
                GetToken();
                if (tokType == _string && token.len != 1) {
                    for (uint8_t i = 0; i < token.len; i++)
                        PutCode(token.str[i]);
                    GetToken();
                } else {
                    Expression();
                    PutCode(CheckByteOper(ExprVal));
                }
                LocCtr = CurHEX;
            } while (haveComma());
            break;
        case _ds:
            CheckLabel(LocCtr);
            StWord(LocCtr);
            CurHEX = (LocCtr += getWordValue());
            break;
        case _dw:
            CheckLabel(LocCtr);
            do {
                uint16_t hl = getWordValue();
                PutCode(hl & 0xff);
                PutCode(hl >> 8);
                LocCtr = CurHEX;
            } while (haveComma());
            break;
        case _end:
            CheckLabel(LocCtr);
            StWord(LocCtr);
            if (OutLine[0] != ' ')
                EndLine(0);
            uint16_t hl = getWordValue();
            if (OutLine[0] == ' ')
                XFerAdr = hl;
            OutLine[0] = ' '; // clear error
            if (ifLevel)
                SetErr('B');
            GetToken();
            if (tokType != _punct || token.str[0] != '\n')
                return EndLine('S');
            return EndFile(Balance != 0);
        case _endif:
            CheckLabel(LocCtr);
            GetIF();
            GetToken();
            break;
        case _endm:
        case _exitm:
            return _ENDM_();
        case _equ:
            if (!hasLabel())
                EndLine('S');
            SET_EQU('=');
            CheckLabel(ExprVal);
            break;
        case _if:
            CheckLabel(LocCtr);
            uint16_t opr1 = getWordValue();
            if (OutLine[0] != ' ')
                return __FALSE(OutLine[0]);
            if (!(opr1 & 1))
                return __FALSE(_if_);
            PutIF(_if_);
            break;
        case _macro:
            return macro();
        case _org:
            uint16_t val = getWordValue();
            if (OutLine[0] == ' ') {
                if (R_opt)
                    val += 0x100;
                LocCtr = CurHEX = val;
                CheckLabel(LocCtr);
                StWord(LocCtr);
            }
            break;
        case _set:
            if (!hasLabel())
                return EndLine('S');
            uint8_t type;

            if ((type = GetSymType()) && type != T_SET)
                SetErr('L');
            SetSymType(T_SET);
            SET_EQU('#');
            hasLabel();
            PutSymVal(ExprVal);
            label = 0;
            break;
        case _title:
            CheckLabel(LocCtr);
            GetToken();
            if (tokType != _string || collectDepth)
                return EndLine('S');
            // note test added in case of multiple titles
            // suspect original code would have problems
            // with multiple titles as it would confuse
            // the writing of symbols
            // here allow multiple titles
            free((void *)Titleptr);
            Titleptr = safeStrdup(token.str);
            GetToken();
            break;
        case _else:
            CheckLabel(LocCtr);
            if (GetIF() != _if_) {
                SetErr('B');
                GetToken();
                break;
            }
            return __FALSE(_else_);
        case _irp:
        case _irpc:
            collectMode = pMnem->code == _irp ? _irp_ : _irpc_;
            CheckLabel(LocCtr);
            GetToken();
            if (tokType == _id) {
                l11c5  = SymTop;
                CurSym = SymTop - 1;
                PutSym(3 + min(token.len, SymLen));
                PutSym(0);
                addParameter();
                GetToken();
                if (tokType == _punct && token.str[0] == ',') {
                    l1afc();
                    if (token.len == 0)
                        GetToken();
                    else if (tokType != _punct || !IsTerminator(token.str[0])) {
                        for (uint8_t i = 0; i < token.len; i++)
                            PutSym(token.str[i]);
                        PutSym('\r');
                        GetToken();
                    }
                    PutSym(0);
                    curSym = SymTop;
                    if (!collectMaroBody(collectMode))
                        return EndFile(true);
                    return l0a7e();
                }
            } else {
                SetErr('S');
                collectMaroBody(collectMode);
                GetToken();
            }
            break;
        case _rept:
            val    = getWordValue(); // get repetition
            l11c5  = SymTop;
            CurSym = SymTop - 1;
            PutSym(val & 0xff);
            PutSym(val >> 8);
            curSym = SymTop;
            if (!collectMaroBody(_rept_))
                return EndFile(true);
            return l0a7e();
        case _aseg:
        case _cseg:
        case _dseg:
        case _name:
        case _extrn:
        case _inpage:
        case _public:
        case _stkln:
            SetErr('N');
            GetToken();
            break;
        case _page:
            CheckLabel(LocCtr);
            GetToken();
            if (tokType != _punct || !IsTerminator(token.str[0])) {
                Expression();
                if (OutLine[0] == ' ')
                    LstPage((uint8_t)ExprVal);
            } else {
                NewPage();
                if (PassNr)
                    Header();
            }
            break;
        case _local:
            if (Balance != 0) {
                do {
                    GetToken();
                    if (tokType != _id)
                        break;
                    uint16_t tmp = SymTop;
                    CurSym       = SymTop - 1;
                    addParameter();
                    if (++localCnt > 9999)
                        SetErr('O');
                    token.len = sprintf(token.str, "??%04d", localCnt % 10000);
                    l1f87();
                    SymTop = tmp;
                    CurSym = SymTop - 1;
                    GetSymbol();
                    l1fa5();
                    GetToken();
                    if (tokType == _punct && IsTerminator(token.str[0]))
                        return EndLine(0);
                } while (tokType == _punct && token.str[0] == ',');
            }
            SetErr('S');
            break;
        case _maclib:
            CheckLabel(LocCtr);
            if (macroChain || InLIB || !GetLibName()) {
                SetErr('S');
                break;
            }
            if (PassNr) {
                GetToken();
                break;
            }

            OpenLib();
            if (L_opt)
                Header();
            while (curCh != '\n' && curCh != cpmEOF)
                GetStrChr();
            NewPage();
            PrepLib();
            return LINELOOP;
        }

    } else {
        uint8_t code = pMnem->code;
        switch (pMnem->type) {
        default:
            return EndLine('S');
            break;
        case _1byte:
            PutCode(code);
            GetToken();
            break;
        case _rp16:
            GetRegPair(code);
            GetComma();
            GetImWord();
            break;
        case _rp:
            GetRegPair(code);
            break;
        case _pshp:
            uint8_t dst = GetDstReg();
            if (dst != (_A << 3) && (dst & (1 << 3)))
                SetErr('R'); // C, E or L not valid
            PutCode(code | (dst & DSTxmsk));
            break;
        case _jpcl:
            PutCode(code);
            GetImWord();
            break;
        case _mov:
            code |= GetDstReg();
            GetComma();
            PutCode(code | GetSrcReg());
            break;
        case _mvi:
            code |= GetDstReg();
            PutCode(code);
            GetComma();
            GetImByte();
            break;
        case _im8:
            PutCode(code);
            GetImByte();
            break;
        case _lsax:
            dst = GetDstReg();
            if (dst & ((_H + 1) << 3)) // Allow B and D only
                SetErr('R');
            PutCode(code | (dst & (2 << 3)));
            break;
        case _lsd:
            PutCode(code);
            GetImWord();
            break;
        case _alur:
            PutCode(code | GetSrcReg());
            break;
        case _rop:
            PutCode(code | GetDstReg());
            break;
        case _rpop:
            dst = GetDstReg();
            if (dst & (1 << 3)) // no A, C, E, L
                SetErr('R');
            PutCode(code | (dst & DSTxmsk));
            break;
        case _rst:
            PutCode(code | GetDstReg());
            break;
        case _io:
            PutCode(code);
            GetImByte();
            break;
        }
        CheckLabel(LocCtr);
        LocCtr = CurHEX;
    }
    return EndLine(0);
}

bool haveComma() {
    if (tokType == _punct && token.str[0] == ',')
        return true;

    if (tokType != _punct || (token.str[0] != ';' && token.str[0] != '\r'))
        SetErr('D');
    return false;
}

uint16_t getWordValue() {
    GetToken();
    Expression();
    return ExprVal;
}

uint8_t GetByteOper() {
    return CheckByteOper(getWordValue());
}

uint8_t CheckByteOper(uint16_t val) {
    if ((val & ~0xff) != 0 && (val & ~0xff) != 0xff00)
        SetErr('V');
    return val & 0xff;
}

uint8_t GetSrcReg() {
    uint8_t reg = GetByteOper();
    if (reg > _A)
        SetErr('V');
    return reg & _A;
}

uint8_t GetDstReg() {
    return GetSrcReg() << 3;
}

void GetRegPair(uint8_t code) {
    uint8_t reg = GetDstReg();
    if (reg & (1 << 3)) // no Accu, C, E or L
        SetErr('R');
    PutCode(code | (reg & ~(1 << 3))); // PMO make valid reg
}

void GetImByte() {
    PutCode(GetByteOper());
}

void GetImWord() {
    PutWord(getWordValue());
}

void GetComma() {
    if (tokType != _punct || token.str[0] != ',')
        SetErr('C');
}

uint8_t EndLine(uint8_t err) {
    if (err)
        SetErr(err);
    if (err != 'S') {
        CheckLabel(LocCtr);
        if (tokType != _punct)
            SetErr('S');
        else if (token.str[0] == '\r') {
            GetToken(); // eat '\n'
            return LINELOOP;
        } else if (token.str[0] == ';')
            CheckLabel(LocCtr);
        else if (token.str[0] == '!')
            return LINELOOP;
        else if (token.str[0] == cpmEOF)
            return EndFile(Balance != 0);
        else
            SetErr('S');
    }
    while (1) {
        GetToken();
        if (tokType == _punct) {
            if (token.str[0] == cpmEOF)
                return EndFile(Balance != 0);
            if (token.str[0] == '\n' || token.str[0] == '!')
                return LINELOOP;
        }
    }
}

uint8_t EndFile(bool err) {
    if (err)
        SetErr('B');
    collectDepth = 0;
    if (PassNr++ == 0) {
        l11d8 = 0xffff;
        while (macroChain) { // invert macro chain
            curSym      = macroChain;
            uint16_t hl = GetSymVal();
            PutSymVal(l11d8);
            l11d8      = macroChain;
            macroChain = hl;
        }
        return PASSLOOP;
    }
    return FINISH;
}

void finish() {
    GetToken();
    printf("End: %04XH\n", LocCtr);
    if (S_opt) {
        l2ea4 = 1;
        PrepSYM();
        WriteSymbols();
    }
    printf("Use Factor %2.1f%%\n", (double)(SymTop - SymBot) / (double)(topPtr - SymBot) * 100.0);

    CurHEX = XFerAdr;
    Exit(0);
}

bool hasLabel() {
    return (curSym = label) != 0;
}

void CheckLabel(uint16_t loc) {
    if (hasLabel()) {
        label = 0;
        if (PassNr == 0) {
            uint8_t type;
            if ((type = GetSymType()) & TYPMASK)
                SetErr('L');
            SetSymType(type | T_DEFINED);
            PutSymVal(loc);
        } else if ((GetSymType() & TYPMASK) == 0 || GetSymVal() != loc)
            SetErr('P');
    }
}

void PutCode(uint8_t val) {
    if (PassNr)
        putHEX(val);
    if (OutLine[1] == ' ') // add address if not already present
        StWord(LocCtr);
    if (outCol < _ASCbyt) // truncate if too many bytes
        StByte(val);
    CurHEX++;
}

void PutWord(uint16_t val) {
    PutCode(val & 0xff);
    PutCode(val >> 8);
}

char hexDigits[16] = { "0123456789ABCDEF" };

void StByte(uint8_t val) {
    OutLine[outCol++] = hexDigits[val >> 4];
    OutLine[outCol++] = hexDigits[val & 0xf];
}

void StWord(uint16_t val) {
    outCol = 1;
    StByte(val >> 8);
    StByte(val & 0xff);
    outCol++;
}

// Convert character to token
//  0x00 .. 0x19 : A .. Z
//  0x1A : ?
//  0x1B : Else

uint8_t Ch2Tok(uint8_t ch) {
    if (isalpha(ch))
        return ch - 'A';
    if (ch == '?')
        return 0x1a;
    return 0x1b;
}

uint16_t symList[0x1c];

void WriteSymbols() {

    curSym = SymBot;
    // sort the symbols
    while (curSym < SymTop) {
        CurSym = SymBot;
        uint8_t type;
        if ((type = GetSymType()) == 6) {
            getParamLen();            // advance to macro params
            while (SymbolByte() != 0) // skip to end of names
                ;
            curSym = CurSym + 1; // next symbol
        } else {
            if (type == l2ea4) { // l1090
                uint16_t reference = curSym;
                CurSym             = curSym + 1;
                GetSymbol();
                if (Q_opt || token.len < 2 || token.str[0] != '?' || token.str[1] != '?') {
                    sym_t *chain = (sym_t *)&symList[Ch2Tok(token.str[0])];
                    while ((curSym = chain->link) != 0) {
                        sym_t *sym   = SYMP(curSym);
                        uint8_t slen = (sym->typeSize & LOMASK) + 1;
                        int8_t cmp   = memcmp(sym->name, token.str, min(slen, token.len));
                        if (cmp > 0 || (cmp == 0 && slen > token.len))
                            break;
                        chain = sym;
                    }
                    uint16_t tmp       = curSym;
                    curSym             = reference;
                    SYMP(curSym)->link = tmp;
                    chain->link        = curSym;
                }
            }
            curSym += SymSize() + 5; // SymSize adds 1 anyway
        }
    }

    uint8_t symCol = 0;
    outCol         = 0;
    for (uint8_t i = 0; i < 0x1c; i++) {
        curSym = symList[i];
        while (curSym) {
            uint8_t symWidth = (BYTE(curSym + 2) & 0xf) + 1;
            CurSym           = curSym + 2;
            if (symCol) {
                StASC('\t');
                symCol = (symCol & 0xf8) + 8;
                if (symCol & 0xf) {
                    symCol += 8;
                    StASC('\t');
                }
            }
            if (symCol + symWidth + 5 >= 80) {
                OutLen = outCol;
                emitLine();
                symCol = outCol = 0;
            }
            uint16_t hl = GetSymVal();
            StByte(hl >> 8);
            StByte(hl & 0xff);
            StASC(' ');
            symCol += 5;
            for (uint8_t i = symWidth; i; i--) {
                StASC(SymbolByte());
                symCol++;
            }
            curSym = WORD(curSym);
        }
    }
    if ((OutLen = outCol))
        emitLine();
}

void StASC(uint8_t ch) {
    OutLine[outCol++] = ch;
}

void l11e4(uint16_t de) {
    if (de == afterComma)
        BYTE(afterComma) = 0;
}

void l11ee(uint16_t hl) {
    Stack[0].l2ef4 = hl;
    if (BYTE(hl) == '\r')
        BYTE(hl) = 0;
}

bool Parenth;
uint8_t ExprCode[ExprCod];
uint8_t ExprPrio[ExprCod];
uint16_t ExprStk[ExprDep];
uint8_t ExprDepth;
uint8_t ExprLen;

// Put operand to expression Stack
void PushExpr(uint16_t val) {
    if (ExprLen >= ExprDep) {
        SetErr('E');
        ExprLen = 0;
    } else
        ExprStk[ExprLen++] = val;
}

// Store expression code and priority
void St_Code_Prio(uint8_t code, uint8_t priority) {
    if (ExprDepth >= ExprCod) {
        ExprDepth = 0;
        SetErr('E');
    } else {
        ExprCode[ExprDepth]   = code;
        ExprPrio[ExprDepth++] = priority;
    }
}

uint16_t PopExpr() {
    if (ExprLen == 0) {
        SetErr('E');
        return 0;
    }
    return ExprStk[--ExprLen];
}

pair_t PopTwoOper() {
    pair_t pair;
    pair.de = PopExpr();
    pair.hl = PopExpr();
    return pair;
}

void Term(uint8_t op) {
    pair_t operand;
    uint16_t result;
    switch (op) {
    case _mul:
        operand = PopTwoOper();
        result  = operand.hl * operand.de;
        break;
    case _div:
        operand = PopTwoOper();
        if (operand.de)
            result = operand.hl / operand.de;
        else {
            SetErr('Z');
            result = 0xffff;
        }
        break;
    case _mod:
        operand = PopTwoOper();
        if (operand.de)
            result = operand.hl % operand.de;
        else {
            SetErr('Z');
            result = operand.hl;
        }
        break;
    case _shl:
        operand = PopTwoOper();
        if (operand.de > 16) {
            SetErr('E');
            result = 0;
        } else
            result = operand.hl << operand.de;
        break;
    case _shr:
        operand = PopTwoOper();
        if (operand.de > 16) {
            SetErr('E');
            result = 0;
        } else
            result = operand.hl >> operand.de;
        break;
    case _plus:
        operand = PopTwoOper();
        result  = operand.hl + operand.de;
        break;
    case _minus:
        operand = PopTwoOper();
        result  = operand.hl - operand.de;
        break;
    case _uminus:
        result = -PopExpr();
        break;
    case _eq:
        operand = PopTwoOper();
        result  = operand.hl == operand.de ? TRUE : FALSE;
        break;
    case _lt:
        operand = PopTwoOper();
        result  = operand.hl < operand.de ? TRUE : FALSE;
        break;
    case _le:
        operand = PopTwoOper();
        result  = operand.hl <= operand.de ? TRUE : FALSE;
        break;
    case _gt:
        operand = PopTwoOper();
        result  = operand.hl > operand.de ? TRUE : FALSE;
        break;
    case _ge:
        operand = PopTwoOper();
        result  = operand.hl >= operand.de ? TRUE : FALSE;
        break;
    case _ne:
        operand = PopTwoOper();
        result  = operand.hl != operand.de ? TRUE : FALSE;
        break;
    case _not:
        result = ~PopExpr();
        break;
    case _and:
        operand = PopTwoOper();
        result  = operand.hl & operand.de;
        break;
    case _or:
        operand = PopTwoOper();
        result  = operand.hl | operand.de;
        break;
    case _xor:
        operand = PopTwoOper();
        result  = operand.hl ^ operand.de;
        break;
    case _high:
        result = PopExpr() >> 8;
        break;
    case _low:
        result = PopExpr() & 0xff;
        break;
    default:
        SetErr('E');
        return;
    }
    PushExpr(result);
}

bool IsExprDel() {
    return tokType == _punct && strchr("\r;!", token.str[0]);
}

bool IsExprDelOrComma() {
    return tokType == _punct && strchr(",\r;!", token.str[0]);
}

void Expression() {
    ExprDepth = 0;
    ExprLen   = 0;
    Parenth   = true;
    ExprVal   = 0;
    while (1) {
        if (IsExprDelOrComma()) {
            while (ExprDepth)
                Term(ExprCode[--ExprDepth]);
            if (ExprLen != 1)
                SetErr('E');
            if (OutLine[0] == ' ')
                ExprVal = ExprStk[0];
            return;
        }
        if (OutLine[0] == ' ') {
            if (tokType == _string) {
                if (token.len == 0 || token.len > 2)
                    SetErr('E');
                // expression relies on string being null terminated
                PushGood(token.len == 0 ? 0 : token.str[0] + (token.str[1] << 8));
            } else if (tokType == _number)
                PushGood(Value);
            else if ((pMnem = GetMnemo(token.str, token.len))) { // Expr_Mnemo?
                uint8_t type = pMnem->type;
                uint8_t pri  = pMnem->code;
                if (type < _regs) {
                    if (type == _nul) {
                        l1afc();
                        if (!IsExprDel() && (tokType != _string || token.len ||
                                             (GetToken(), !IsExprDelOrComma()))) {
                            do {
                                l1afc();
                            } while (!IsExprDel());
                            PushGood(FALSE);
                        } else
                            PushGood(TRUE);
                        continue;
                    } else if (type == _lpar) { // Expr_Expr?
                        if (!Parenth)
                            SetErr('E');
                        St_Code_Prio(type, pri);
                        Parenth = true;

                    } else if (Parenth && type == _plus)
                        ;
                    else {
                        if (Parenth) {
                            if (type == _minus)
                                type++; // convert to unary minus
                            else if (type != _not && type != _high && type != _low)
                                SetErr('E');
                        }
                        while (ExprDepth && ExprPrio[ExprDepth - 1] >= pri)
                            Term(ExprCode[--ExprDepth]);
                        if (type == _rpar) { // 1530
                            if (ExprDepth == 0 || ExprCode[--ExprDepth] != _lpar)
                                SetErr('E');
                            Parenth = false;
                        } else {
                            St_Code_Prio(type, pri);
                            Parenth = true;
                        }
                    }
                } else {
                    if (type == _maclib)
                        SetErr('E');
                    PushGood(pri);
                }
            } else if (tokType == _punct) {
                if (token.str[0] == '$')
                    PushGood(LocCtr);
                else {
                    SetErr('E');
                    PushGood(0);
                }
            } else {
                if (!SymSrc()) {
                    SetErr('P');
                    PostSym();
                } else if ((GetSymType() & TYPMASK) == 0)
                    SetErr('U');
                PushGood(GetSymVal());
            }
        }
        GetToken();
    }
}

void PushGood(uint16_t val) {
    if (!Parenth)
        SetErr('E');
    Parenth = false;
    PushExpr(val);
}

uint8_t RdSrc() {
    uint8_t ch;

    while (Balance) {
        if ((ch = BYTE(Stack[0].l2ef4))) {
            Stack[0].l2ef4++;
            if (ch == ',')
                afterComma = Stack[0].l2ef4;
            break;
        }
        if (Stack[0].collectMode != 2) {
            if (F_CHR++ != 0xff)
                return 0;
            SetErr('B');
            emitLine();
        }
        unnest();
        ch = Stack[0].curCh;
        if (ch)
            return ch;
    }
    if (Balance == 0)
        ch = mgetc();
    F_CHR = ch;
    return tokType == _string ? ch : toupper(ch);
}

uint8_t StChar(uint8_t ch) {
    if (ch != '\r' && ch != '\n' && OutLen < LINLEN)
        OutLine[OutLen++] = ch;
    return ch;
}

bool GetId() {
    PrnLine.len = 0;
    expandChIdx  = 0;
    lookAhead    = RdSrc();
    if (tokType == _cmnt)
        return false;
    if (lookAhead >= 0x80) {
        char *p      = getMnemStr(lookAhead);
        PrnLine.len = (uint8_t)strlen(p);
        memcpy(PrnLine.str, p, PrnLine.len);
        lookAhead = RdSrc();
    } else if (!IsLabCh(lookAhead))
        return false;
    while (IsLabel(lookAhead)) {
        if (PrnLine.len >= SymLen - 1)
            return false;
        PrnLine.str[PrnLine.len++] = lookAhead;
        lookAhead                    = RdSrc();
    }
    PrnLine.str[PrnLine.len] = '\0'; // make C string
    return true;
}

bool l16f3() {
    bool result;
    l1613 = curSym;
    SymMArg();
    if (!(result = l1e47()))
        curSym = l1613;
    return result;
}

uint8_t RdChar() {
    uint8_t expandCnt = 0;
    while (1) {
        if (expandCnt++ == 255) { // too many expansions
            SetErr('O');
            PrnLine.len   = 0;
            Stack[0].l2ef4 = 0; // original was PrnLine
            // here 0 will access mem[0] which is always 0
        }
        if (PrnLine.len) { // pending char
            PrnLine.len--;
            return StChar(PrnLine.str[expandChIdx++]);
        }
        uint8_t ch = lookAhead;
        if (Balance == 0) { // simple read
            if (lookAhead)
                lookAhead = 0;
            else
                ch = RdSrc();
            return StChar(ch);
        } else if (lookAhead) {
            if (lookAhead == '^') {
                if (!GetId() && lookAhead == '&') {
                    PrnLine.len++;
                    PrnLine.str[0] = '&';
                    lookAhead       = 0;
                    return StChar('&');
                }
                return StChar('^');
            } else if (lookAhead == '&') { // 176c
                if (!GetId() || !l16f3())  // 179e
                    return StChar('&');
            } else if (lookAhead != 0x7f) {
                lookAhead = 0;
                return StChar(ch);
            } else if (!GetId() || !l16f3())
                continue;
        } else if (!GetId() || (lookAhead != '&' && tokType == _string) || !l16f3())
            continue;

        Stack[0].curCh = lookAhead == '&' ? 0x7f : lookAhead;
        lookAhead      = 0;

        nest();
        Stack[0].collectMode = 2;
        Stack[0].prevTopPtr  = topPtr;
        Stack[0].l2ef4       = GetSymValAddr();
        PrnLine.len         = 0;
        curSym               = l1613;
        GetId();
    }
}

void l17f1() {
    IniEval();
    PrnLine.len = 0;
    lookAhead    = 0;
    curCh        = 0;
    OutLen       = 0;
    prevCh       = '\n';
    emitLine();
    OutLen = _ASCbyt;
}

void IniEval() {
    token.len = 0;
    ValBase   = 0;
}

void addCh() {
    if (token.len >= ArgMax) {
        token.len = 0;
        SetErr('O');
    }
    token.str[token.len++] = curCh;
    token.str[token.len]   = 0; // make sure it's always terminated
}

bool IsLabCh(uint8_t ch) {
    return isalpha(ch) || ch == '?' || ch == '@';
}

bool IsLabel(uint8_t ch) {
    return isalnum(ch) || ch == '?' || ch == '@';
}

uint8_t IsValid(uint8_t ch) {
    if (ch < ' ' && ch != '\t' && ch != '\r' && ch != '\n' && ch != cpmEOF)
        SetErr('I');
    return ch;
}

uint8_t GetStrChr() {
    curCh = IsValid(RdChar());
    if (collectDepth && (collectMode != _macro_ || PassNr == 0))
        PutSym(curCh);
    return curCh;
}

void skipEOL() {
    while (!IsEOS(curCh))
        GetStrChr();
}

bool GetLibName() {
    char name[256];
    uint16_t len = 0;
    tokType      = _string; // make sure chars are not mapped to upper case

    free((void *)libFile); // incase previously used
    libFile = NULL;

    while (curCh == ' ' || curCh == '\t' || curCh == 0)
        GetStrChr();
    bool instring = false;
    if (curCh == '\'') { // collect string to allow any char >= ' '
        instring = true;
        while ((curCh = GetStrChr()) >= ' ') {
            if (curCh == '\'' && (curCh = GetStrChr()) != '\'') {
                instring = false;
                break;
            }
            if (len < 255)
                name[len++] = curCh;
        }
    } else {
        while (curCh > ' ' && curCh != ';') {
            if (len < 255)
                name[len++] = curCh;
            GetStrChr();
        }
    }
    tokType = 0;
    if (instring || len == 0 || len >= 255) {
        SetErr('V');
        return false;
    }

    name[len] = '\0';
    libFile   = makeFilename(name, ".lib", false);
    return true;
}

void GetToken() {
    IniEval();
    for (;;) {
        tokType = 0;
        if (curCh == '\t')
            ;
        else if (curCh == ';') {
            tokType = _cmnt;
            GetStrChr();
            if (collectDepth && (collectMode != _macro_ || PassNr == 0) && curCh == ';') {
                for (CurSym -= 2; CurSym != startMacroBody; CurSym--) {
                    if (BYTE(CurSym) == '\n' || BYTE(CurSym) >= '!')
                        break;
                }
                if (CurSym != startMacroBody && BYTE(CurSym) == '\n')
                    CurSym -= 2;
                uint8_t tmp  = collectDepth;
                collectDepth = 0;
                skipEOL();
                PutSym(curCh);
                collectDepth = tmp;
            } else
                skipEOL();
            break;
        } else if (curCh != '*' ? (curCh && curCh != ' ') : (prevCh && prevCh != ' '))
            break;
        GetStrChr();
    } // of forever

    tokType = 0;
    if (IsLabCh(curCh))
        tokType = _id;
    else if (isdigit(curCh))
        tokType = _number;
    else if (curCh == '\'') {
        curCh   = 0;
        tokType = _string;
    } else {
        if (curCh == '\n') {
            if (Balance)
                OutLine[5] = '+';
            emitLine();
            OutLine[0] = ' ';
            OutLen     = _ASCbyt;
        }
        tokType = _punct;
    }
    for (;;) {
        if ((prevCh = curCh))
            addCh();
        GetStrChr();
        if (tokType == _punct) {
            if (collectDepth) // Test relation
                return;
            char *rel;
            if (token.str[0] == '=')
                rel = "EQ";
            else if (token.str[0] == '<') {
                if (curCh == '=') {
                    curCh = 0;
                    rel   = "LE";
                } else if (curCh == '>') { // PMO added missing <>
                    curCh = 0;
                    rel   = "NE";
                } else
                    rel = "LT";
            } else if (token.str[0] == '>') {
                if (curCh == '=') {
                    curCh = 0;
                    rel   = "GE";
                } else
                    rel = "GT";
            } else
                return;
            token.len = 2;
            strcpy(token.str, rel);
            tokType = _id;
            return;
        } else if (tokType == _id) {
            if (curCh == '$')
                curCh = 0;
            else if (!IsLabel(curCh))
                return;
        } else if (tokType == _number) {
            if (curCh == '$')
                curCh = 0;
            else if (!isxdigit(curCh)) {
                if (curCh == 'O' || curCh == 'Q' || curCh == 'H') {
                    ValBase = curCh == 'H' ? 16 : 8;
                    curCh   = 0;
                } else if (prevCh == 'B' || prevCh == 'D') {
                    ValBase = prevCh == 'D' ? 10 : 2;
                    token.len--;
                } else
                    ValBase = 10;
                uint32_t val = 0;
                for (uint8_t i = 0; i < token.len; i++) {
                    uint8_t digit = token.str[i];
                    digit -= isdigit(digit) ? '0' : ('A' - 10);
                    if (digit >= ValBase)
                        SetErr('V');
                    if ((val = val * ValBase + digit) >= 0x10000)
                        SetErr('V');
                }
                Value = (uint16_t)val;
                return;
            }
        } else { // string
            if (curCh == '\r') {
                SetErr('O');
                return;
            } else if (curCh == '\'' && GetStrChr() != '\'')
                return;
        }
    }
}

bool IsSpace(uint8_t ch) {
    return ch == '\0' || ch == ' ' || ch == '\t';
}

bool IsDelimiter() {
    return curCh == ',' || curCh == ';' || curCh == '%' || IsEOS(curCh);
}

bool IsEOS(uint8_t ch) {
    return ch == '\r' || ch == cpmEOF || ch == '!';
}

bool IsTerminator(uint8_t ch) {
    return ch == '\r' || ch == '!' || ch == ';';
}

bool IsDelim(uint8_t ch) {
    return ch == ';' || ch == ' ' || ch == '\t' || ch == ',';
}

void l1afc() {
    IniEval();
    tokType            = 0;
    uint8_t chevronCnt = 0;
    bool setPrev       = false;
    while (IsSpace(curCh))
        GetStrChr();
    if (IsDelimiter()) {
        tokType = _punct;
        addCh();
        setPrev = true;
    }
    while (1) { // 1b20
        if (setPrev) {
            prevCh = curCh; // 1b20
            GetStrChr();
            if (tokType == _punct)
                return;
        } else
            setPrev = true;
        while (1) { // 1b2f
            if (IsEOS(curCh)) {
                if (tokType == _string || chevronCnt != 0)
                    SetErr('V');
                tokType = _error;
                return;
            }
            if (tokType == _string) {
                if (curCh == '\'') {
                    addCh();
                    GetStrChr();
                    if (curCh == '\'')
                        break;
                    tokType = 0;
                    continue;
                }
            } else if (curCh == '\'')
                tokType = _string;
            else if (curCh == '^') {
                GetStrChr();
                if (curCh != '\t' && curCh < ' ') {
                    SetErr('I');
                    tokType = _error;
                    return;
                }
            } else if (curCh == '<') {
                if (chevronCnt++ == 0)
                    break;
            } else if (curCh == '>') {
                if (chevronCnt && --chevronCnt == 0)
                    break;
            } else if (chevronCnt == 0 && IsDelim(curCh)) {
                tokType = _error;
                return;
            }
            addCh(); // 1bc9
            break;
        }
    }
}

uint16_t symHashTab[FLen];
uint8_t HashIdx;
uint16_t *curHashTab = symHashTab;

void IniField_1() {
    memset(symHashTab, 0, sizeof(symHashTab));
    curSym = NIL;
}

void IniField_2() {
    memset(macHashTab, 0, sizeof(macHashTab));
}

void nest() {
    if (Balance >= MaxBal)
        SetErr('B');
    else
        Stack[++Balance] = Stack[0];
}

void unnest() {
    if (Balance == 0)
        SetErr('B');
    else
        Stack[0] = Stack[Balance--];
}

uint8_t GetHashIdx(pstr_t *p) {
    uint8_t hash = 0;
    for (uint8_t i = 0; i < p->len; i++)
        hash += p->str[i];
    return HashIdx = hash & 0x7f;
}

#if 0
void l1e31(uint8_t len) {
    SYMP(curSym)->typesize &= 0xf0;
    SYMP(curSym)->typesize |= len;
}
#endif

uint8_t SymSize() {
    return (SYMP(curSym)->typeSize & 0xf) + 1;
}

bool l1e47() {
    if (curSym == 0)
        return false;
    uint8_t bc;
    if (Stack[0].collectMode != _macro_) {
        for (bc = Balance; bc; bc--) {
            if (Stack[bc].collectMode == _macro_)
                break;
        }
        if (bc == 0)
            return false;
    } else
        bc = 0;
    if (Stack[bc].prevTopPtr > curSym)
        return true;
    curSym = 0;
    return false;
}

bool SymAdr() {
    return curSym != 0;
}

bool SymMArg() {
    if (PrnLine.len > SymLen)
        PrnLine.len = SymLen;
    return lookup((pstr_t *)&PrnLine, macHashTab[GetHashIdx((pstr_t *)&PrnLine) & 0xf]);
}

bool SymSrc() {
    if (token.len > SymLen)
        token.len = SymLen;
    return lookup((pstr_t *)&token, symHashTab[GetHashIdx((pstr_t *)&token)]);
}

bool lookup(pstr_t *ps, uint16_t head) {
    for (curSym = head; curSym; curSym = WORD(curSym)) {
        if (ps->len == SymSize() && memcmp(ps->str, &BYTE(curSym + 3), ps->len) == 0)
            return true;
    }
    return false;
}

void PostSym() {
    curSym = SymTop;
    SymTop += token.len + 5; // packed sizeof(sym_t)
    if (SymTop >= topPtr)
        fatal("Symbol Table Overflow");
    curHashTab = symHashTab;
    l1f31();
    WORD(SymTop - 2) = 0; // clear value
}

void l1f31() {
    sym_t *ps           = SYMP(curSym);

    uint16_t curLink    = curHashTab[HashIdx];
    curHashTab[HashIdx] = curSym;
    ps->link            = curLink;
    uint8_t len         = min(SymLen, token.len);
    ps->typeSize        = len - 1;
    memcpy(ps->name, token.str, len);
}

void alloc(uint16_t bc) {
    curSym = topPtr - (bc + token.len);
    if (curSym < SymTop)
        fatal("Symbol table overflow");
    topPtr = curSym;
}

void l1f87() {
    alloc(1);
    memcpy(&BYTE(topPtr), token.str, token.len);
    BYTE(topPtr + token.len) = 0;
}

void l1fa5() {
    alloc(3);
    curHashTab = macHashTab;
    l1f31();
    SetSymType(HashIdx);
}

void PopParam() {
    while (topPtr < Stack[0].prevTopPtr) {
        curSym                   = topPtr;
        macHashTab[GetSymType()] = SYMP(curSym)->link; // remove from macro hash table
        uint16_t hl              = GetSymValAddr();    // point to param value
        while (BYTE(hl++))                             // skip the parameters
            ;
        topPtr = hl; // update to reflect removal
    }
}

// Byte 0,1  Link address - or zero
// Byte  2   High: Type - Low: Length
// Byte 3..n-1   Symbol
// Byte n,n+1    Symbol value

void SetSymType(uint8_t type) {
    sym_t *ps    = SYMP(curSym);
    ps->typeSize = (ps->typeSize & 0xf) | (type << 4);
}

uint8_t GetSymType() {
    sym_t *ps = SYMP(curSym);
    return ps->typeSize >> 4;
}

uint16_t GetSymValAddr() {
    return curSym + SymSize() + 3;
}

void PutSymVal(uint16_t val) {
    WORD(GetSymValAddr()) = val;
}

uint16_t GetSymVal() {
    return WORD(GetSymValAddr());
}

void SeekMacroParam() {
    CurSym = GetSymValAddr() + 2;
}

void setParameterCnt(uint8_t val) {
    SeekMacroParam();
    BYTE(CurSym) = val;
}

uint8_t getParamLen() {
    SeekMacroParam();
    return BYTE(CurSym);
}

void addParameter() {
    uint8_t type = GetHashIdx((pstr_t *)&token) << 4;
    if (token.len > SymLen)
        token.len = SymLen;
    PutSym(type | (token.len - 1));
    for (uint8_t i = 0; i < token.len; i++)
        PutSym(token.str[i]);
}

void GetSymbol() {
    uint8_t b = SymbolByte();
    HashIdx   = b >> 4;
    token.len = (b & 0xf) + 1;
    for (int i = 0; i < token.len; i++)
        token.str[i] = SymbolByte();
}

uint8_t SymbolByte() {
    return BYTE(++CurSym);
}

void PutSym(uint8_t ch) {
    if (CurSym + 1 >= topPtr)
        fatal("Symbol Table Overflow");
    else {
        BYTE(++CurSym) = ch;
        SymTop         = CurSym + 1;
    }
}
