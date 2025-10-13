
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
uint16_t ActSym;
uint16_t l11c5;
uint16_t XFerAdr; // Transfer address
uint8_t ASC_len;
uint8_t l11ca;
uint8_t l11cb;
uint8_t l11cc;
uint8_t IF_level;         // IF level
uint8_t IF_Array[IF_Dep]; // IF level
uint16_t l11d6;
uint16_t l11d8;
uint8_t l11da;
uint8_t supp_0;  // Leading zero flag
uint16_t val_16; // Current 16 bit value
uint16_t LocCnt; // Local count
uint16_t l11e0;
uint16_t l11e2;

uint8_t prevCh;
uint8_t ValBase;
uint16_t l1613;
uint8_t l1616;
uint8_t ChrCnt;
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
    uint8_t l2ea4;
    uint16_t l2eb4;
    uint16_t l2ed4;
    uint16_t l2ef4;
    uint8_t l2f14;
    uint16_t l2f24;
    uint8_t l2f44; // not used?
    uint8_t l2f54;
} Stack[16];

// overlapped data area for symbol sorting
uint8_t l2ea4;
uint8_t l2ea5;
uint8_t l2ea6;
uint8_t l2ea7;
uint16_t l2eaa;
uint16_t l2eac[28];
uint8_t l2f64;
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
uint16_t TopPtr;
uint8_t PassNr;
uint16_t CurHEX;
uint16_t LocCtr;

uint16_t SymBot;
uint16_t SymHshp;
uint16_t CurSym;

uint8_t l305a;
uint8_t curCh;
uint8_t l305c;
bool InLIB; // true if from LIB
uint8_t S_opt = _SPENA;
uint8_t M_opt = _SPENA;
uint16_t l3060;
uint16_t Titleptr;
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
char const *symFile;
FILE *fpHEX;
char const *hexFile;

// Cold entry

uint8_t mem[0x10000];

char const help[] =
    "Usage: %s [options] sourceFile[.asm]\n"
    "Where options are\n"
    "  -p file[.prn]  override listing file. Default sourceFile.prn\n"
    "  -s file[.sym]  override symbol file. Default sourcefile.sym\n"
    "  -h file[.hex]  override hex file. Default sourcfile.hex\n"
    "  -c controls+   set mac control options\n"
    "  -t [=n]        expand tabs to every nth column. If n missing then 8 is used\n"
    "Notes:\n"
    " * file is the filename or - for standard output or . to disable output\n"
    "   If no extent is specified then the relevant extent is added.\n"
    " * controls are as per MAC (+|-|*)(1|L|M|Q|R|S). lowercase letters are also supported\n"
    "   Due to OS limitations, controls may need to be quoted to allow * and spaces\n";

int main(int argc, char **argv) {
    PassNr = 0;
    l305a  = 0;
    IniMAC(argc, argv);
    IniField_1();
    l11d6       = 0;

    uint8_t ret = PASSLOOP;
    while (ret != FINISH) {
        IniField_2();
        Balance        = 0;
        Stack[0].l2ea4 = 0;
        Stack[0].l2f24 = TopPtr;
        l17f1();
        IniLine();
        ActSym  = 0;
        XFerAdr = LocCtr = CurHEX = R_opt ? TPA : 0;
        IF_level                  = 0;
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
        if (tokType == _digit)
            return LINELOOP;
        if ((ret = MAC_sub()) != STMTLOOP)
            return ret;
    }
}

uint8_t l02d6() {
    l11c2               = getParamLen();
    uint16_t saveTopPtr = TopPtr;
    if (l11c2 != 0) {
        while (1) {
            uint8_t next = 0;
            uint8_t a;
            if (tokType == _any) {
                a = token.str[0];
                if (testEndInst(a))
                    break;
                if (a == '%') {
                    val_16 = GetOpr1();
                    if (tokType != _any) {
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
                if (tokType != _any) {
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
    Stack[0].l2f14 = 0;
    NewPage();
    Stack[0].l2f54 = IF_level;
    nest();
    Stack[0].l2ef4 = tmp;
    Stack[0].l2f24 = saveTopPtr;
    Stack[0].l2f14 = 0;
    Stack[0].l2ea4 = 1;
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
    return tokType != _error && (pMnem = GetMnemo(token.str, token.len)) &&
           pMnem->type == _pseudo && pMnem->code == _macro;
}

bool testEndInst(uint8_t ch) {
    return ch == ';' || ch == '\r' || testEndStmt(ch);
}

bool testEndStmt(uint8_t ch) {
    return ch == '\n' || ch == eof || ch == '!';
}

void NewPage() {

    if (Balance)
        OutLine[5] = '+';
    emitLine();
    OutLen = _ASCbyt;
}

void l03c1() {
    while (Balance) {
        if (Stack[0].l2ea4 >= 3 || Stack[0].l2ea4 == 1)
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
    if (tokType == _any) {
        if (token.str[0] == '$') {
            if (IsActSym())
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
    } else if (tokType != _label)
        return EndLine('S');
    if ((pMnem = GetMnemo(token.str, token.len)))
        return MAC_Mnemo();
    if (!SymSrc()) { // new symbol
        PostSym();
        if (PassNr)
            SetErr('P');            // e39
    } else if (GetSymType() == 6) { // 3e9
        l11e0 = 0;
        if (PassNr) {
            while (GetSymVal() > SymHshp) { // 23c
                l11e0 = SymHshp;
                lookup((pstr_t *)&token, WORD(SymHshp));
                if (SymHshp) {
                    if (GetSymType() != 0)
                        return EndLine('P');
                } else {
                    l1afc();
                    if (!testMacro() || l11e0 != l11d8)
                        return EndLine('P');
                    ActSym = l11d8;
                    return macro();
                }
            }
        }
        StoreSymbol();
        l1afc();
        if (!testMacro()) {
            uint16_t tmp = SymHshp; // l02c1
            CheckSymbol();
            if (M_opt == _DISA)
                StWord(LocCtr);
            SymHshp = tmp;
            return l02d6();
        }
        if (ActSym)
            SetErr('S');
        if (PassNr) {
            if (l11e0 != l11d8)
                return EndLine('P');
            ActSym = l11d8;
        } else {
            LoadSymbol();
            GetHashIdx((pstr_t *)&token);
            PostSym();
            ActSym = SymHshp;
        }
        return macro();
    }
    if (ActSym)
        SetErr('L');
    ActSym = SymHshp;
    GetToken();
    if (tokType != _any || token.str[0] != ':')
        return STMTLOOP;
    return LINELOOP;
}

bool IsMacro(uint8_t type) {
    return type == _macro || type == _rept || type == _irp || type == _irpc;
}

bool l0722(uint8_t a) {
    l305c = a;
    while (1) {
        if (tokType == _any) {
            if ((token.str[0] == '\r' || token.str[0] == '!'))
                break;
            else if (token.str[0] == eof)
                return false;
        }
        SetErr('S');
        GetToken();
    }

    l3060 = CurSym;
    l305a = 1;
    GetToken();

    while (1) {
        l11e0 = CurSym;
        GetToken();
        if (tokType == _any && token.str[0] == eof)
            return false;
        if (tokType == _label && (pMnem = GetMnemo(token.str, token.len))) {
            if (l305c == 1 && PassNr == 0 && token.len != 1 && pMnem->tokId) {
                CurSym = l11e0;
                PutSym(pMnem->tokId | 0x80); // replace token with its id
                if (curCh)
                    PutSym(curCh);
            }
            if (pMnem->type == _pseudo) {
                if (IsMacro(pMnem->code)) {
                    if (l305a++ == 0xff)
                        return false;
                } else if (pMnem->code == _endm && --l305a == 0)
                    break;
            }
        }
    }
    if (l305c == 1) {
        ActSym = 0;
        if (InLIB)
            PutSymVal(0);
        else {
            PutSymVal(l11d6);
            l11d6 = SymHshp;
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
    if (IF_level >= IF_Dep)
        SetErr('O');
    else
        IF_Array[IF_level++] = a;
}
uint8_t GetIF() {
    if (IF_level == 0) {
        SetErr('B');
        return 0;
    } else
        return IF_Array[--IF_level];
}

void SET_EQU(uint8_t a) {
    uint16_t tmp = LocCtr;
    StWord(LocCtr = GetOpr1());
    LocCtr     = tmp;
    OutLine[6] = a;
    return;
}

uint8_t macro() {
    if (!IsActSym()) {
        SetErr('L');
        return EndLine(0);
    }
    if (PassNr != 0) {
        if (SymHshp != l11d8)
            SetErr('P');
        else
            l11d8 = GetSymVal();
    } else
        SetSymType(6);
    l11da = 0;
    if (PassNr == 0)
        setParamLen(0);
    while (GetToken(), tokType == _label) {
        if (PassNr == 0)
            l2065();
        l11da++;
        GetToken();
        if (tokType != _any || token.str[0] != ',')
            break;
    }
    if (!l0722(1))
        return EndFile(true);
    if (PassNr == 0)
        setParamLen(l11da);
    GetToken();
    return EndLine(0);
}

uint8_t l0552() {
    uint16_t ptr;
    if (Stack[0].l2ea4 == _endm) {
        uint16_t hl = Stack[0].l2eb4;
        if ((ptr = WORD(hl)) != 0) {
            WORD(hl)       = ptr - 1;
            OutLen         = _ASCbyt;
            Stack[0].l2ef4 = Stack[0].l2ed4;
            curCh          = 0;
            return LINELOOP;
        }
    } else {
        uint16_t hl = Stack[0].l2eb4;
        ptr         = WORD(hl);
        if (BYTE(ptr) != '\r') {
            if (BYTE(ptr) != 0) {
                if (Stack[0].l2ea4 != 3) {
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
                            ptr = l11e2;
                            GetToken();
                            if (token.str[0] != ',')
                                SetErr('S');
                            l11ee(ptr); // 5f0
                            PrnLine.len = 0;
                        }
                    }
                    WORD(Stack[0].l2eb4) = ptr; // 5f7
                    curCh                = saveCh;
                    lookAhead            = saveLookAhead;
                } else {
                    WORD(hl)     = ptr + 1;
                    token.len    = 1;
                    token.str[0] = BYTE(ptr);
                    l1f87();
                }
                CurSym = Stack[0].l2eb4 + 1; // 606
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
    NewPage();               // 616
    TopPtr = Stack[0].l2f24; // Restore top
    unnest();
    IF_level = Stack[0].l2f54;
    if ((curCh = Stack[0].l2f14))
        StChar(curCh);
    return LINELOOP;
}

uint8_t l0a7e() {
    NewPage();
    GetToken();
    Stack[0].l2f54 = IF_level;
    Stack[0].l2f14 = curCh != '\n' ? curCh : 0;
    nest();
    Stack[0].l2f24 = TopPtr;
    while (CurSym >= l11c5)
        BYTE(--TopPtr) = BYTE(CurSym--);
    SymTop         = CurSym + 1;
    Stack[0].l2eb4 = TopPtr;
    if (l305c != 6)
        WORD(TopPtr) = TopPtr + BYTE(TopPtr);
    Stack[0].l2ed4 = TopPtr + (SymHshp - l11c5);
    Stack[0].l2ea4 = l305c;
    return l0552();
}

uint8_t _ENDM_() {
    CheckSymbol();
    l03c1();
    OutLine[5] = '+';
    if (Stack[0].l2ea4 < 3)
        PopParam();
    else {
        uint16_t tmp   = Stack[0].l2f24;
        Stack[0].l2f24 = Stack[0].l2eb4;
        PopParam();
        Stack[0].l2f24 = tmp;
        if (pMnem->code == _endm)
            return l0552();
    }
    NewPage();
    TopPtr = Stack[0].l2f24; // Restore top
    unnest();
    IF_level = Stack[0].l2f54;
    if ((curCh = Stack[0].l2f14))
        StChar(curCh);
    return LINELOOP;
}

uint8_t __FALSE(uint8_t l11ca) {
    l11cc = l11cb = 0;
    while (1) {
        while (1) {
            if (tokType == _any) {
                if (token.str[0] == '\r') {
                    GetToken(); // eat '\n'
                    break;
                } else if (token.str[0] == '!')
                    break;
                else if (token.str[0] == eof)
                    return EndFile(true);
            }
            GetToken();
        }
        GetToken();
        if (tokType == _digit)
            GetToken();
        if (tokType != _label)
            continue;
        if (!(pMnem = GetMnemo(token.str, token.len))) {
            GetToken();
            if (tokType == _any) {
                if (token.str[0] != ':')
                    continue;
                GetToken();
            }
            if (tokType != _label || !(pMnem = GetMnemo(token.str, token.len)))
                continue;
        }
        if (pMnem->type != _pseudo)
            continue;
        if (pMnem->code == _if) {
            if (l11cb++ == 0xff)
                SetErr('O');
            continue;
        } else if (pMnem->code == _else) {
            if (l11cb)
                continue;
            if (l11ca == 2)
                SetErr('B');
            PutIF(2);
            GetToken();
        } else if (pMnem->code == _endif) {
            if (l11cb--)
                continue;
            if (l11cc)
                SetErr('B');
            GetToken();
        } else if (IsMacro(pMnem->code)) {
            if (l11cc++ == 0xff)
                SetErr('O');
            continue;
        } else if (pMnem->code != _endm || l11cc--)
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
            CheckSymbol();
            do {
                GetToken();
                if (tokType == _strg && token.len != 1) {
                    for (uint8_t i = 0; i < token.len; i++)
                        PutCode(token.str[i]);
                    GetToken();
                } else {
                    Expression();
                    PutCode(CheckByteOper(ExprVal));
                }
                SetLocCtr();
            } while (GetDatDelim() == ',');
            break;
        case _ds:
            CheckSymbol();
            StWord(LocCtr);
            CurHEX = LocCtr += GetOpr1();
            break;
        case _dw:
            CheckSymbol();
            do {
                uint16_t hl = GetOpr1();
                PutCode(hl & 0xff);
                PutCode(hl >> 8);
                SetLocCtr();
            } while (GetDatDelim() == ',');
            break;
        case _end:
            CheckSymbol();
            StWord(LocCtr);
            if (OutLine[0] != ' ')
                EndLine(0);
            uint16_t hl = GetOpr1();
            if (OutLine[0] == ' ')
                XFerAdr = hl;
            OutLine[0] = ' '; // clear error
            if (IF_level)
                SetErr('B');
            GetToken();
            if (tokType != _any || token.str[0] != '\n')
                return EndLine('S');
            return EndFile(Balance != 0);
        case _endif:
            CheckSymbol();
            GetIF();
            GetToken();
            break;
        case _endm:
        case _exitm:
            return _ENDM_();
        case _equ:
            if (!IsActSym())
                EndLine('S');
            SET_EQU('=');
            uint16_t tmp = LocCtr;
            LocCtr       = ExprVal;
            CheckSymbol();
            LocCtr = tmp;
            break;
        case _if:
            CheckSymbol();
            uint16_t opr1 = GetOpr1();
            if (OutLine[0] != ' ')
                return __FALSE(OutLine[0]);
            if (!(opr1 & 1))
                return __FALSE(1);
            PutIF(1);
            break;
        case _macro:
            return macro();
        case _org:
            uint16_t val = GetOpr1();
            if (OutLine[0] == ' ') {
                if (R_opt)
                    val += 0x100;
                LocCtr = val;
                CurHEX = val;
                CheckSymbol();
                StWord(LocCtr);
            }
            break;
        case _set:
            if (!IsActSym())
                return EndLine('S');
            uint8_t type;

            if ((type = GetSymType()) && type != 5)
                SetErr('L');
            SetSymType(5);
            SET_EQU('#');
            IsActSym();
            PutSymVal(ExprVal);
            ActSym = 0;
            break;
        case _title:
            CheckSymbol();
            GetToken();
            if (tokType != _strg || l305a)
                return EndLine('S');
            if (PassNr == 0) {
                Titleptr = SymTop;
                CurSym   = SymTop - 1;
                for (uint8_t i = 0; i < token.len; i++)
                    PutSym(token.str[i]);
                PutSym(0);
            }
            GetToken();
            break;
        case _else:
            CheckSymbol();
            if (GetIF() != 1) {
                SetErr('B');
                GetToken();
                break;
            }
            return __FALSE(2);
        case _irp:
        case _irpc:
            l305c = pMnem->code == _irp ? _irp_ : _irpc_;
            CheckSymbol();
            GetToken();
            if (tokType == _label) {
                l11c5  = SymTop;
                CurSym = SymTop - 1;
                PutSym(3 + min(token.len, SymLen));
                PutSym(0);
                l2065();
                GetToken();
                if (tokType == _any && token.str[0] == ',') {
                    l1afc();
                    if (token.len == 0)
                        GetToken();
                    else if (tokType != _any || !IsDelim(token.str[0])) {
                        for (uint8_t i = 0; i < token.len; i++)
                            PutSym(token.str[i]);
                        PutSym('\r');
                        GetToken();
                    }
                    PutSym(0);
                    SymHshp = SymTop;
                    if (!l0722(l305c))
                        return EndFile(true);
                    return l0a7e();
                }
            } else {
                SetErr('S');
                l0722(l305c);
                GetToken();
            }
            break;
        case _rept:
            val    = GetOpr1(); // get repetition
            l11c5  = SymTop;
            CurSym = SymTop - 1;
            PutSym(val & 0xff);
            PutSym(val >> 8);
            SymHshp = SymTop;
            if (!l0722(6))
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
            CheckSymbol();
            GetToken();
            if (tokType != _any || !IsDelim(token.str[0])) {
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
                    if (tokType != _label)
                        break;
                    tmp    = SymTop;
                    CurSym = SymTop - 1;
                    l2065();
                    if (++LocCnt > 9999)
                        SetErr('O');
                    token.len = sprintf(token.str, "??%04d", LocCnt % 10000);
                    l1f87();
                    SymTop = tmp;
                    CurSym = SymTop - 1;
                    GetSymbol();
                    l1fa5();
                    GetToken();
                    if (tokType == _any && IsDelim(token.str[0]))
                        return EndLine(0);
                } while (tokType == _any && token.str[0] == ',');
            }
            SetErr('S');
            break;
        case _maclib:
            CheckSymbol();
            if (l11d6 || InLIB || !GetLibName()) {
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
            while (curCh != '\r' && curCh != eof)
                GetStrChr();
            if (curCh == '\r')
                GetStrChr(); // accept cr
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
        CheckSymbol();
        SetLocCtr();
    }
    return EndLine(0);
}

uint8_t GetDatDelim() {
    if (tokType != _any || (token.str[0] != ',' && token.str[0] != ';' && token.str[0] != '\r'))
        SetErr('D');
    return token.str[0];
}

uint16_t GetOpr1() {
    GetToken();
    Expression();
    return ExprVal;
}

uint8_t GetByteOper() {
    return CheckByteOper(GetOpr1());
}

uint8_t CheckByteOper(uint16_t val) {
    if ((val & ~0xff) && (val & ~0xff) != 0xff00)
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
    PutWord(GetOpr1());
}

void GetComma() {
    if (tokType != _any || token.str[0] != ',')
        SetErr('C');
}

uint8_t EndLine(uint8_t err) {
    if (err)
        SetErr(err);
    if (err != 'S') {
        CheckSymbol();
        if (tokType != _any)
            SetErr('S');
        else if (token.str[0] == '\r') {
            GetToken(); // eat '\n'
            return LINELOOP;
        } else if (token.str[0] == ';')
            CheckSymbol();
        else if (token.str[0] == '!')
            return LINELOOP;
        else if (token.str[0] == eof)
            return EndFile(Balance != 0);
        else
            SetErr('S');
    }
    while (1) {
        GetToken();
        if (tokType == _any) {
            if (token.str[0] == eof)
                return EndFile(Balance != 0);
            if (token.str[0] == '\n' || token.str[0] == '!')
                return LINELOOP;
        }
    }
}

uint8_t EndFile(bool err) {
    if (err)
        SetErr('B');
    l305a = 0;
    if (PassNr++ == 0) {
        l11d8 = -1;
        while (l11d6) {
            SymHshp     = l11d6;
            uint16_t hl = GetSymVal();
            PutSymVal(l11d8);
            l11d8 = l11d6;
            l11d6 = hl;
        }
        return PASSLOOP;
    }
    return FINISH;
}

void finish() {
    GetToken();
    StWord(LocCtr);
    strcpy(&OutLine[5], "\n");
    fputs(OutLine + 1, stdout);
    if (S_opt) {
        l2ea4 = 1;
        PrepSYM();
        WriteSymbols();
    }
    StWord((SymTop - SymBot) / ((TopPtr - SymBot) >> 8));
    strcpy(OutLine + 5, "H Use Factor\n");
    fputs(OutLine + 2, stdout);
    CurHEX = XFerAdr;
    FClose();
}

void SetLocCtr() {
    LocCtr = CurHEX;
}

bool IsActSym() {
    return (SymHshp = ActSym) != 0;
}

void CheckSymbol() {
    if (!IsActSym())
        return;
    ActSym = 0;
    if (PassNr == 0) {
        uint8_t type;
        if ((type = GetSymType()) & TYPMASK)
            SetErr('L');
        SetSymType(type | 1);
        PutSymVal(LocCtr);
    } else if ((GetSymType() & TYPMASK) == 0 || GetSymVal() != LocCtr)
        SetErr('P');
}

void PutCode(uint8_t val) {
    if (PassNr)
        putHEX(val);
    if (OutLine[1] == ' ')
        StWord(LocCtr);
    if (ASC_len < _ASCbyt)
        StByte(val);
    CurHEX++;
}

void PutWord(uint16_t val) {
    PutCode(val & 0xff);
    PutCode(val >> 8);
}

char hexDigits[16] = { "0123456789ABCDEF" };

void StByte(uint8_t val) {
    OutLine[ASC_len++] = hexDigits[val >> 4];
    OutLine[ASC_len++] = hexDigits[val & 0xf];
}

void StWord(uint16_t val) {
    ASC_len     = 1;
    StByte(val >> 8);
    StByte(val & 0xff);
    ASC_len++;
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

uint16_t l2eac[0x1c];

void WriteSymbols() {
    l2ea5   = 0;
    ASC_len = 0;
    SymHshp = SymBot;
    // sort the symbols
    while (SymHshp < SymTop) {
        CurSym    = Titleptr - 1;
        bool skip = true;
        uint8_t type;
        if (Titleptr != SymHshp) {
            if ((type = GetSymType()) == 6)
                for (uint8_t i = getParamLen(); i; i--)
                    GetSymbol();
            else
                skip = false;
        }
        if (skip) {
            while (SymbolByte() != 0)
                ;
            SymHshp = CurSym + 1;
            continue;
        }
        if (type == l2ea4) {  // l1090
            l2eaa  = SymHshp; /// overlay data !!
            CurSym = SymHshp + 1;
            GetSymbol();
            if (Q_opt || token.len < 2 || token.str[0] != '?' || token.str[1] != '?') {
                sym_t *chain = (sym_t *)&l2eac[Ch2Tok(token.str[0])];
                while ((SymHshp = chain->link) != 0) {
                    sym_t *sym   = SYMP(SymHshp);
                    uint8_t slen = (sym->typeSize & LOMASK) + 1;
                    int8_t cmp   = memcmp(sym->name, token.str, min(slen, token.len));
                    if (cmp > 0 || (cmp == 0 && slen > token.len))
                        break;
                    chain = sym;
                }
                uint16_t tmp        = SymHshp;
                SymHshp             = l2eaa;
                SYMP(SymHshp)->link = tmp;
                chain->link         = SymHshp;
            }
        }
        SymHshp += SymSize() + 5; // SymSize adds 1 anyway
    }

    for (uint8_t i = 0; i < 0x1c; i++) {
        SymHshp = l2eac[i];
        while (SymHshp) {
            uint8_t b = l2ea6 = (BYTE(SymHshp + 2) & 0xf) + 1;
            CurSym            = SymHshp + 2;
            if (l2ea5) {
                StASC('\t');
                l2ea5 = (l2ea5 & 0xf8) + 8;
                if (l2ea5 & 0xf) {
                    l2ea5 += 8;
                    StASC('\t');
                }
            }
            if (l2ea5 + b + 5 >= 80) {
                while (OutLine[--ASC_len] == '\t')
                    ;
                OutLen  = ASC_len;
                ASC_len = 0;
                emitLine();
                l2ea5 = 0;
            }
            uint16_t hl = GetSymVal();
            StByte(hl >> 8);
            StByte(hl & 0xff);
            StASC(' ');
            l2ea5 += 5;
            for (uint8_t i = l2ea6; i; i--) {
                StASC(SymbolByte());
                l2ea5++;
            }
            SymHshp = WORD(SymHshp);
        }
    }
    OutLen = ASC_len;
    emitLine();
}

void StASC(uint8_t ch) {
    OutLine[ASC_len++] = ch;
}

void l11e4(uint16_t de) {
    if (de == l11e2)
        BYTE(l11e2) = 0;
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
        result = PopExpr();
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
    return tokType == _any && (token.str[0] == '\r' || token.str[0] == ';' || token.str[0] == '!');
}

bool IsExprDelOrComma() {
    return IsExprDel() || token.str[0] == ',';
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
            if (tokType == _strg) {
                if (token.len == 0 || token.len > 2)
                    SetErr('E');
                uint16_t val = (token.len == 0) ? 0 : token.str[0];
                if (token.len > 1)
                    val += token.str[1] << 8;
                PushGood(val);
            } else if (tokType == _digit)
                PushGood(Value);
            else if ((pMnem = GetMnemo(token.str, token.len))) { // Expr_Mnemo?
                uint8_t type = pMnem->type;
                uint8_t pri  = pMnem->code;
                if (type < _regs) {
                    if (type == _nul) {
                        l1afc();
                        if (!IsExprDel() &&
                            (tokType != _strg || token.len || (GetToken(), !IsExprDelOrComma()))) {
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
            } else if (tokType == _any) {
                if (token.str[0] != '$') {
                    SetErr('E');
                    PushGood(0);
                } else
                    PushGood(LocCtr);
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
                l11e2 = Stack[0].l2ef4;
            break;
        }
        if (Stack[0].l2ea4 != 2) {
            if (F_CHR++ != 0xff)
                return 0;
            SetErr('B');
            emitLine();
        }
        unnest();
        ch = Stack[0].l2f14;
        if (ch)
            return ch;
    }
    if (Balance == 0)
        ch = mgetc();
    F_CHR = ch;
    return tokType == _strg ? ch : toupper(ch);
}

uint8_t StChar(uint8_t ch) {
    if (ch != '\r' && ch != '\n' && OutLen < LINLEN)
        OutLine[OutLen++] = ch;
    return ch;
}

bool GetId() {
    PrnLine.len = 0;
    l2f64       = 0;
    lookAhead   = RdSrc();
    if (tokType == _cmnt)
        return false;
    if (lookAhead >= 0x80) {
        char *p     = getMnemStr(lookAhead);
        PrnLine.len = (uint8_t)strlen(p);
        memcpy(PrnLine.str, p, PrnLine.len);
        lookAhead = RdSrc();
    } else if (!IsLabCh(lookAhead))
        return false;
    while (IsLabel(lookAhead)) {
        if (PrnLine.len >= SymLen - 1)
            return false;
        PrnLine.str[PrnLine.len++] = lookAhead;
        lookAhead                  = RdSrc();
    }
    PrnLine.str[PrnLine.len] = '\0'; // make C string
    return true;
}

bool l16f3() {
    bool result;
    l1613 = SymHshp;
    SymMArg();
    if (!(result = l1e47()))
        SymHshp = l1613;
    return result;
}

uint8_t RdChar() {
    ChrCnt = 0;
    while (1) {
        if (ChrCnt++ == 255) {
            SetErr('O');
            PrnLine.len    = 0;
            Stack[0].l2ef4 = 0;
        }
        if (PrnLine.len) {
            PrnLine.len--;
            return StChar(PrnLine.str[l2f64++]);
        }
        uint8_t ch = lookAhead;
        if (Balance == 0) {
            lookAhead = 0;
            return StChar(ch ? ch : RdSrc());
        } else if (lookAhead) {
            if (lookAhead == '^') {
                if (!GetId() && lookAhead == '&') {
                    PrnLine.len++;
                    PrnLine.str[0] = '&';
                    lookAhead      = 0;
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
        } else if (!GetId() || (lookAhead != '&' && tokType == _strg) || !l16f3())
            continue;

        Stack[0].l2f14 = lookAhead == '&' ? 0x7f : lookAhead;
        lookAhead      = 0;

        nest();
        Stack[0].l2ea4 = 2;
        Stack[0].l2f24 = TopPtr;
        Stack[0].l2ef4 = GetSymValAddr();
        PrnLine.len    = 0;
        SymHshp        = l1613;
        GetId();
    }
}

void l17f1() {
    IniEval();
    PrnLine.len = 0;
    lookAhead   = 0;
    curCh       = 0;
    OutLen      = 0;
    prevCh      = '\n';
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
    if (ch < ' ' && ch != '\t' && ch != '\r' && ch != '\n' && ch != eof)
        SetErr('I');
    return ch;
}

uint8_t GetStrChr() {
    curCh = IsValid(RdChar());
    if (l305a && (l305c != 1 || PassNr == 0))
        PutSym(curCh);
    return curCh;
}

void skipEOL() {
    while (!IsEOL(curCh))
        GetStrChr();
}

bool GetLibName() {
    char name[256];
    uint16_t len = 0;
    tokType      = _strg; // make sure chars are not mapped to upper case
    while (curCh == ' ' || curCh == '\t' || curCh == 0)
        GetStrChr();
    while (curCh >= ' ' && curCh != ';' && curCh != eof) {
        if (len < 255)
            name[len] = curCh;
        len++;
        GetStrChr();
    }
    tokType = 0;
    if (len >= 255) {
        SetErr('V');
        return false;
    }
    while (len && name[len - 1] == ' ')
        len--;
    name[len] = '\0';
    if (*name) {
        free((void *)libFile); // incase previously used
        libFile = safeStrdup(makeFilename(name, ".lib", false));
        return true;
    }
    return false;
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
            if (l305a && (l305c != 1 || PassNr == 0) && curCh == ';') {
                for (CurSym -= 2; CurSym != l3060; CurSym--) {
                    if (BYTE(CurSym) == '\n' || BYTE(CurSym) >= '!')
                        break;
                }
                if (CurSym != l3060 && BYTE(CurSym) == '\n')
                    CurSym -= 2;
                uint8_t tmp = l305a;
                l305a       = 0;
                skipEOL();
                PutSym(curCh);
                l305a = tmp;
            } else
                skipEOL();
            break;
        } else if (curCh != '*' ? (curCh && curCh != ' ') : (prevCh && prevCh != ' '))
            break;
        GetStrChr();
    } // of forever

    tokType = 0;
    if (IsLabCh(curCh))
        tokType = _label;
    else if (isdigit(curCh))
        tokType = _digit;
    else if (curCh == '\'') {
        curCh   = 0;
        tokType = _strg;
    } else {
        if (curCh == '\n') {
            if (Balance)
                OutLine[5] = '+';
            emitLine();
            OutLine[0] = ' ';
            OutLen     = _ASCbyt;
        }
        tokType = _any;
    }
    for (;;) {
        if ((prevCh = curCh))
            addCh();
        GetStrChr();
        if (tokType == _any) {
            if (l305a) // Test relation
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
            tokType = _label;
            return;
        } else if (tokType == _label) {
            if (curCh == '$')
                curCh = 0;
            else if (!IsLabel(curCh))
                return;
        } else if (tokType == _digit) {
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
    return curCh == ',' || curCh == ';' || curCh == '%' || IsEOL(curCh);
}

bool IsEOL(uint8_t ch) {
    return ch == '\r' || ch == eof || ch == '!';
}

bool IsDelim(uint8_t ch) {
    return ch == ';' || ch == ' ' || ch == '\t' || ch == ',';
}

void l1afc() {
    IniEval();
    tokType = 0;
    l1616   = 0;

    while (IsSpace(curCh))
        GetStrChr();
    if (IsDelimiter()) {
        tokType = _any;
        addCh();
        prevCh = curCh; // 1b20
        GetStrChr();
        if (tokType == _any)
            return;
    }
    while (1) { // 1b20

        while (1) { // 1b2f
            if (IsEOL(curCh)) {
                if (tokType == _strg || l1616 != 0)
                    SetErr('V');
                tokType = _error;
                return;
            }
            if (tokType == _strg) {
                if (curCh == '\'') {
                    addCh();
                    GetStrChr();
                    if (curCh == '\'')
                        break;
                    tokType = 0;
                }
            } else if (curCh == '\'')
                tokType = _strg;
            else if (curCh == '^') {
                GetStrChr();
                if (curCh != '\t' && curCh >= ' ') {
                    SetErr('I');
                    tokType = _error;
                    return;
                }
            } else if (curCh == '<') {
                if (l1616++ == 0)
                    break;
            } else if (curCh == '>') {
                if (l1616 && --l1616 == 0)
                    break;
            } else if (l1616 == 0 && IsDelim(curCh)) {
                tokType = _error;
                return;
            }
            addCh(); // 1bc9
            break;
        }
        prevCh = curCh; // 1b20
        GetStrChr();
        if (tokType == _any)
            return;
    }
}

uint16_t symHashTab[FLen];
uint8_t HashIdx;
uint16_t *curHashTab = symHashTab;

void IniField_1() {
    memset(symHashTab, 0, sizeof(symHashTab));
    SymHshp = NIL;
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
    SYMP(SymHshp)->typesize &= 0xf0;
    SYMP(SymHshp)->typesize |= len;
}
#endif

uint8_t SymSize() {
    return (SYMP(SymHshp)->typeSize & 0xf) + 1;
}

bool l1e47() {
    if (SymHshp == 0)
        return false;
    uint8_t bc = 0;
    if (Stack[0].l2ea4 != 1) {
        for (bc = Balance; bc; bc--) {
            if (Stack[bc].l2ea4 == 1)
                break;
        }
        if (bc == 0)
            return false;
    }
    if (Stack[bc].l2f24 > SymHshp)
        return true;
    SymHshp = 0;
    return false;
}

bool SymAdr() {
    return SymHshp != 0;
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
    for (SymHshp = head; SymHshp; SymHshp = WORD(SymHshp)) {
        if (ps->len == SymSize() && memcmp(ps->str, &BYTE(SymHshp + 3), ps->len) == 0)
            return true;
    }
    return false;
}

void PostSym() {
    SymHshp = SymTop;
    SymTop += token.len + 5; // packed sizeof(sym_t)
    if (SymTop >= TopPtr)
        fatal("Symbol Table Overflow");
    curHashTab = symHashTab;
    l1f31();
    WORD(SymTop - 2) = 0; // clear value
}

void l1f31() {
    sym_t *ps           = SYMP(SymHshp);

    uint16_t curLink    = curHashTab[HashIdx];
    curHashTab[HashIdx] = SymHshp;
    ps->link            = curLink;
    uint8_t len         = min(SymLen, token.len);
    ps->typeSize        = len - 1;
    memcpy(ps->name, token.str, len);
}

void alloc(uint16_t bc) {
    SymHshp = TopPtr - (bc + token.len);
    if (SymHshp < SymTop)
        fatal("Symbol table overflow");
    TopPtr = SymHshp;
}

void l1f87() {
    alloc(1);
    memcpy(&BYTE(TopPtr), token.str, token.len);
    BYTE(TopPtr + token.len) = 0;
}

void l1fa5() {
    alloc(3);
    curHashTab = macHashTab;
    l1f31();
    SetSymType(HashIdx);
}

void PopParam() {
    while (TopPtr < Stack[0].l2f24) {
        SymHshp                  = TopPtr;
        macHashTab[GetSymType()] = SYMP(SymHshp)->link;
        uint16_t hl              = GetSymValAddr(); // point to param value
        while (BYTE(hl++))
            ;
        TopPtr = hl;
    }
}

// Byte 0,1  Link address - if any
// Byte  2   High: Type - Low: Length
// Byte 3..n-1   Symbol
// Byte n,n+1    Symbol value

void SetSymType(uint8_t type) {
    sym_t *ps    = SYMP(SymHshp);
    ps->typeSize = (ps->typeSize & 0xf) | (type << 4);
}

uint8_t GetSymType() {
    sym_t *ps = SYMP(SymHshp);
    return ps->typeSize >> 4;
}

uint16_t GetSymValAddr() {
    return SymHshp + SymSize() + 3;
}

void PutSymVal(uint16_t val) {
    WORD(GetSymValAddr()) = val;
}

uint16_t GetSymVal() {
    return WORD(GetSymValAddr());
}

void NextSym() {
    CurSym = GetSymValAddr() + 2;
}

void setParamLen(uint8_t val) {
    NextSym();
    BYTE(CurSym) = val;
}

uint8_t getParamLen() {
    NextSym();
    return BYTE(CurSym);
}

void l2065() {
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
    if (CurSym + 1 >= TopPtr)
        fatal("Symbol Table Overflow");
    else {
        BYTE(++CurSym) = ch;
        SymTop         = CurSym + 1;
    }
}
