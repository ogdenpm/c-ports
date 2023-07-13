/****************************************************************************
 *  main5.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include "os.h"

byte maxSymLen;
word dictSize;
word w66D2;
word w66D4;
offset_t xrefItemP;
byte b66D8 = 0;
offset_t dictionaryP;
offset_t dictTopP;

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";
static char dots[]   = ". . . . . . . . . . . . . . . . . . . . ";
static char dashes[] = "------------------------------------";
byte b3F0B           = 0xFF; /* ixi module header */

void warn(char const *str) {
    lprintf("\n-- WARNING -- %s\n", str);
}

void LoadDictionary() {
    dictionaryP = dictTopP = botMem;
    dictSize = maxSymLen = 0;
    curInfoP             = botInfo + 2;

    while (1) {
        if (GetType() < MACRO_T && GetSymbol() != 0) {
            dictTopP = dictTopP + 2;
            if (dictTopP >= botSymbol) {
                warn("INSUFFICIENT MEMORY FOR FULL DICTIONARY LISTING");
                return;
            }
            dictSize         = dictSize + 1;
            *WordP(dictTopP) = curInfoP;
            SetScope(0); /* used for xref Chain() */
            curSymbolP = GetSymbol();
            if (PstrP(curSymbolP)->len > maxSymLen)
                maxSymLen = PstrP(curSymbolP)->len;
        }
        AdvNxtInfo();
        if (curInfoP == 0)
            return;
    }
}

byte CmpSym(offset_t dictItem1, offset_t dictItem2) {
    pointer sym1, sym2;
    pointer str1, str2;
    byte i, j;

    curInfoP = dictItem1;
    sym1     = ByteP(GetSymbol());
    str1     = sym1 + 1;
    curInfoP = dictItem2;
    sym2     = ByteP(GetSymbol());
    str2     = sym2 + 1;
    if (*sym1 < *sym2) {
        i = *sym1;
        j = 0;
    } else if (*sym1 > *sym2) {
        i = *sym2;
        j = 2;
    } else {
        i = *sym1;
        j = 1;
    }

    while (i != 0) {
        if (*str1 < *str2)
            return 0;
        if (*str1 > *str2)
            return 2;
        str1++;
        str2++;
        i--;
    }
    return j;
}

void SortDictionary() {
    word p, q, r, s, t, u, v;
    offset_t w, x;
    word y, z;

    if (dictSize == 1)
        return;
    r = dictSize / 2;
    s = r + 2;
    t = 1;
    p = 2;
    q = r;
    y = dictSize;
    u = q;
    w = WordP(dictionaryP)[u];

    while (1) {
        v = u * 2;
        if (v > y) {
            WordP(dictionaryP)[u] = w;
            if (t == 2) {
                x                     = WordP(dictionaryP)[1];
                WordP(dictionaryP)[1] = WordP(dictionaryP)[q];
                WordP(dictionaryP)[q] = x;
                if (p >= dictSize)
                    break;
                else {
                    p = p + 1;
                    q = y = z - p;
                    u     = 1;
                }
            } else if (p >= r) {
                t = 2;
                z = dictSize + 2;
                p = 2;
                q = y = dictSize;
                u     = 1;
            } else {
                p = p + 1;
                q = s - p;
                y = dictSize;
                u = q;
            }
            w = WordP(dictionaryP)[u];
        } else {
            if (v != y)
                if (CmpSym(WordP(dictionaryP)[v + 1], WordP(dictionaryP)[v]) > 1)
                    v = v + 1;
            if (CmpSym(WordP(dictionaryP)[v], w) <= 1) {
                WordP(dictionaryP)[u] = w;
                if (t == 2) {
                    x                     = WordP(dictionaryP)[1];
                    WordP(dictionaryP)[1] = WordP(dictionaryP)[q];
                    WordP(dictionaryP)[q] = x;
                    if (p >= dictSize)
                        break;
                    p = p + 1;
                    q = y = z - p;
                    u     = 1;
                } else if (p >= r) {
                    t = 2;
                    z = dictSize + 2;
                    p = 2;
                    q = y = dictSize;
                    u     = 1;
                } else {
                    p = p + 1;
                    q = s - p;
                    y = dictSize;
                    u = q;
                }
                w = WordP(dictionaryP)[u];
            } else {
                WordP(dictionaryP)[u] = WordP(dictionaryP)[v];
                u                     = v;
            }
        }
    }
}

static void LoadXref() {
    byte xrefType;

    vfRewind(&xrff);
    while ((xrefType = vfRbyte(&xrff))) {
        word infoP = vfRword(&xrff);
        word stmt  = vfRword(&xrff);
        if (xrefType == 'B' || XREF) {
            curInfoP  = infoP + botInfo;
            xrefItemP = w66D4 + 1;
            w66D4 += 4;
            if (w66D4 > botSymbol) {
                warn("INSUFFICIENT MEMORY FOR FULL XREF PROCESSING");
                break;
            }
            WordP(xrefItemP)[0] = GetScope();
            SetScope(xrefItemP);
            WordP(xrefItemP)[1] = xrefType == 'B' ? -stmt : stmt; /* make defn line -ve */
        }
    }
    vfRewind(&xrff);
}

static void XrefDef2Head() {
    word p;
    offset_t q, r;

    for (p = 1; p <= dictSize; p++) {
        curInfoP  = WordP(dictionaryP)[p];
        xrefItemP = GetScope();
        if (xrefItemP != 0) {
            q = 0;
            SetScope(0);
            while (xrefItemP != 0) {
                r = WordP(xrefItemP)[0];
                if ((WordP(xrefItemP)[1] & 0x8000) != 0)
                    q = xrefItemP; /* definition */
                else {
                    WordP(xrefItemP)[0] = GetScope();
                    SetScope(xrefItemP);
                }
                xrefItemP = r;
            }

            if (q != 0) { /* insert definition at head */
                xrefItemP           = q;
                WordP(xrefItemP)[0] = GetScope();
                SetScope(xrefItemP);
            }
        }
    }
}

void PrepXref() {

    w66D2 = dictTopP + 2;
    if (w66D2 >= botSymbol) {
        warn("INSUFFICIENT MEMORY FOR ANY XREF PROCESSING");
        return;
    }
    w66D4 = w66D2 - 1;
    LoadXref();
    XrefDef2Head();
}

// file scope due to nested procedure lift;
static byte defnCol, addrCol, sizeCol, nameCol, attribCol, refContCol;

static void Sub_480A() {
    word p;

    if (!XREF) {
        NewLineLst();
        return;
    }

    xrefItemP = GetScope();
    if (xrefItemP == 0) {
        NewLineLst();
        return;
    }
    lstStr(": ");
    p = 0;

    while (xrefItemP != 0) {
        if (p != WordP(xrefItemP)[1]) {
            if (PWIDTH < col + 5) {
                NewLineLst();
                TabLst(-refContCol);
            }
            TabLst(1);
            lprintf("%d", WordP(xrefItemP)[1]);
            p = WordP(xrefItemP)[1];
        }
        xrefItemP = WordP(xrefItemP)[0];
    }
    if (col)
        NewLineLst();
}

static void Sub_48A7() {
    curSymbolP = GetSymbol();
    TabLst(-nameCol);
    lprintf("%.*s%.*s", PstrP(curSymbolP)->len, PstrP(curSymbolP)->str, attribCol - col - 2,
            &dots[PstrP(curSymbolP)->len]);
    TabLst(1);
}

static void Sub_48E2(word arg1w, word arg2w) {
    TabLst(-addrCol);
    lstStr(hexfmt(4, arg1w)->str);
    if (arg2w != 0) {
        TabLst(-sizeCol);
        lprintf("%5d", arg2w);
    }
}

static void Sub_4921() {
    xrefItemP = GetScope();
    if (GetType() == BUILTIN_T)
        return;
    if (xrefItemP != 0 && (WordP(xrefItemP)[1] & 0x8000) != 0) {
        TabLst(-defnCol);
        lprintf("%5d", 0x10000 - WordP(xrefItemP)[1]); /* defn stored as -ve */
        SetScope(WordP(xrefItemP)[0]);
    } else if (!TestInfoFlag(F_LABEL)) {
        TabLst(-defnCol);
        lstStr("-----");
    }
}

static void Sub_499C() {
    lprintf(" EXTERNAL(%d)", GetExternId());
}

static void Sub_49BB() {
    offset_t p, q, r;

    p        = curInfoP;
    curInfoP = GetBaseOffset();
    if (TestInfoFlag(F_MEMBER)) {
        r        = GetSymbol();
        curInfoP = GetParentOffset();
        q        = GetSymbol();
    } else {
        q = GetSymbol();
        r = 0;
    }

    curSymbolP = q;
    lprintf(" BASED(%.*s", PstrP(curSymbolP)->len, PstrP(curSymbolP)->str);
    if (r) {
        lstc('.');
        curSymbolP = r;
        lprintf("%.*s", PstrP(curSymbolP)->len, PstrP(curSymbolP)->str);
    }
    lstc(')');
    curInfoP = p;
}

static void Sub_4A42() {
    offset_t p;

    p        = curInfoP;
    curInfoP = GetParentOffset();
    curSymbolP = GetSymbol();
    lprintf(" MEMBER(%.*s)", PstrP(curSymbolP)->len, PstrP(curSymbolP)->str);
    curInfoP = p;
}

static void Sub_4A78(char const *str) {
    Sub_4921();
    Sub_48A7();
    lstStr(str);
    Sub_480A();
}

static void Sub_4A92() {
    Sub_4921();
    Sub_48E2(GetLinkVal(), GetDimension2());
    Sub_48A7();
    lstStr("PROCEDURE");
    if (GetDataType() != 0)
        lstStr(GetDataType() == 2 ? " BYTE" : " ADDRESS");
    if (TestInfoFlag(F_PUBLIC))
        lstStr(" PUBLIC");

    if (TestInfoFlag(F_EXTERNAL))
        Sub_499C();

    if (TestInfoFlag(F_REENTRANT))
        lstStr(" REENTRANT");

    if (TestInfoFlag(F_INTERRUPT))
        lprintf(" INTERRUPT(%d)", GetIntrNo());
    if (!TestInfoFlag(F_EXTERNAL))
        lprintf(" STACK=%s", hexfmt(4, GetBaseVal())->str);

    Sub_480A();
}

static void Sub_4B4A(char const *str) {
    word size;

    Sub_4921();
    byte type = GetType();
    if (type == BYTE_T)
        size = 1;
    else if (type == ADDRESS_T)
        size = 2;
    else if (type == LABEL_T)
        size = 0;
    else
        size = GetParentOffset();

    if (TestInfoFlag(F_ARRAY))
        size *= GetDimension();
    Sub_48E2(GetLinkVal(), size);
    Sub_48A7();
    lstStr(str);
    if (TestInfoFlag(F_BASED))
        Sub_49BB();
    if (TestInfoFlag(F_ARRAY))
        lprintf(" ARRAY(%d)", GetDimension());

    if (TestInfoFlag(F_PUBLIC))
        lstStr(" PUBLIC");

    if (TestInfoFlag(F_EXTERNAL))
        Sub_499C();

    if (TestInfoFlag(F_AT))
        lstStr(" AT");
    if (TestInfoFlag(F_DATA))
        lstStr(" DATA");
    if (TestInfoFlag(F_INITIAL))
        lstStr(" INITIAL");

    if (TestInfoFlag(F_MEMBER))
        Sub_4A42();

    if (TestInfoFlag(F_PARAMETER))
        lstStr(" PARAMETER");

    if (TestInfoFlag(F_AUTOMATIC))
        lstStr(" AUTOMATIC");

    if (TestInfoFlag(F_ABSOLUTE))
        lstStr(" ABSOLUTE");

    Sub_480A();
}

static void Sub_4C84() {
    curSymbolP = GetSymbol();
    if (b66D8 != PstrP(curSymbolP)->str[0]) {
        NewLineLst();
        b66D8 = PstrP(curSymbolP)->str[0];
    }
    if (GetType() < MACRO_T)
        switch (GetType()) {
        case 0:
            Sub_4A78("LITERALLY");
            break;
        case 1:
            Sub_4B4A("LABEL");
            break;
        case 2:
            Sub_4B4A("BYTE");
            break;
        case 3:
            Sub_4B4A("ADDRESS");
            break;
        case 4:
            Sub_4B4A("STRUCTURE");
            break;
        case 5:
            Sub_4A92();
            break;
        case 6:
            Sub_4A78("BUILTIN");
            break;
        }
} /* Sub_4C84() */

void PrintRefs() {
    word p;

    /* PrintRefs() */
    defnCol    = 3;
    addrCol    = defnCol + 6;
    sizeCol    = addrCol + 6;
    nameCol    = sizeCol + 7;
    attribCol  = nameCol + maxSymLen + 2;
    refContCol = attribCol + 1;
    SetMarkerInfo(attribCol, '-', 3);
    EjectNext();
    if (XREF)
        lstStr("CROSS-REFERENCE LISTING\n"
               "-----------------------\n");
    else
        lstStr("SYMBOL LISTING\n"
               "--------------\n");

 
    SetSkipLst(2);
    TabLst(-defnCol);
    lstStr(" DEFN");
    TabLst(-addrCol);
    lstStr(" ADDR");
    TabLst(-sizeCol);
    lstStr(" SIZE");
    TabLst(-nameCol);
    lstStr("NAME, ATTRIBUTES, AND REFERENCES\n");
    TabLst(-defnCol);
    lstStr("-----");
    TabLst(-addrCol);
    lstStr("-----");
    TabLst(-sizeCol);
    lstStr("-----");
    TabLst(-nameCol);
    lstStr("--------------------------------\n\n");

    for (p = 1; p <= dictSize; p++) {
            curInfoP = WordP(dictionaryP)[p];
            if (GetType() == BUILTIN_T) {
                if (GetScope() != 0)
                    Sub_4C84();
            } else
                Sub_4C84();
    }
} /* PrintRefs() */

// lifted
static void WrIxiBuf(uint8_t const *buf, word cnt) {
    fwrite(buf, 1, cnt, ixiFile.fp);
}

static void WrIxiByte(byte v) {
    fputc(v, ixiFile.fp);
}

static void WrIxiWord(word v) {
    fputc(v % 256, ixiFile.fp);
    fputc(v / 256, ixiFile.fp);
}

void CreateIxrefFile() {

    OpenF(&ixiFile, "wb");
    curInfoP   = botInfo + procInfo[1];
    curSymbolP = GetSymbol();
    if (curSymbolP) { /* Write() the module info */
        pstr_t *pstr = PstrP(curSymbolP);
        WrIxiByte(b3F0B);
        WrIxiByte(22 + pstr->len);
        WrIxiByte(pstr->len);           /* module name len */
        WrIxiBuf(pstr->str, pstr->len); /* module name */
    }

    WrIxiBuf(basename(srcFileTable[0].fNam), 10); // form compatibility max 10 chars
    WrIxiBuf("---------", 9);

    for (int p = 1; p <= dictSize; p++) {
        curInfoP  = WordP(dictionaryP)[p];
        byte type = GetType();
        if (LABEL_T <= type && type <= PROC_T &&
            (TestInfoFlag(F_PUBLIC) || (TestInfoFlag(F_EXTERNAL) && !TestInfoFlag(F_AT)))) {

            WrIxiByte(TestInfoFlag(F_PUBLIC) ? 0 : 1);
            curSymbolP   = GetSymbol();
            pstr_t *pstr = PstrP(curSymbolP);
            WrIxiByte(6 + pstr->len);
            WrIxiByte(pstr->len);           /* module name len */
            WrIxiBuf(pstr->str, pstr->len); /* module name */

            WrIxiByte(type);
            if (GetType() == PROC_T)
                WrIxiWord(GetDataType());
            else
                WrIxiWord(TestInfoFlag(F_ARRAY) ? GetDimension() : 0);
        }
    }
    CloseF(&ixiFile);
}

void Sub_4EC5() {
    LoadDictionary();
    SortDictionary();
    PrepXref();
    if ((XREF || SYMBOLS) && PRINT)
        PrintRefs();
    if (IXREF)
        CreateIxrefFile();
}

word Start5() {
    MEMORY = 0x6936;
    botMem = MEMORY + 0x100;
    topSymbol += 4;

    Sub_4EC5();
    if (PRINT)
        LstModuleInfo();
    EndCompile();
    Exit(programErrCnt != 0);
}
