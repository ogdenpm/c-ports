/****************************************************************************
 *  main5.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"
#include <stdlib.h>

byte maxSymLen;
offset_t xrefIdx;
byte b66D8 = 0;

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";


void warn(char const *str) {
    lprintf("\n-- WARNING -- %s\n", str);
}

void LoadDictionary() {
    dictCnt = maxSymLen = 0;

    for (infoIdx = AdvNxtInfo(0); infoIdx != 0; infoIdx = AdvNxtInfo(infoIdx)) {
        if (GetType() < MACRO_T && GetSymbol() != 0) {
            newDict(infoIdx);
            SetScope(0); /* used for xref Chain() */
            curSym = GetSymbol();
            if (symtab[curSym].name->len > maxSymLen)
                maxSymLen = symtab[curSym].name->len;
        }
    }
}

int CmpSym(void const *a, void const *b) {
    index_t ia = *(index_t *)a;
    index_t ib = *(index_t *)b;

    int cmp = strcmp(symtab[infotab[ia].sym].name->str, symtab[infotab[ib].sym].name->str);
    return cmp ? cmp : ia - ib;
}

void SortDictionary() {

    qsort(dicttab, dictCnt, sizeof(index_t), CmpSym);
}

static void LoadXref() {
    byte xrefType;

    vfRewind(&xrff);
    while ((xrefType = vfRbyte(&xrff))) {
        word infoP = vfRword(&xrff);
        word stmt  = vfRword(&xrff);
        if (xrefType == 'B' || XREF) {
            infoIdx = infoP;
            xrefIdx = newXref(GetScope(), xrefType == 'B' ? -stmt : stmt); /* make defn line -ve */
            SetScope(xrefIdx);
        }
    }
    vfRewind(&xrff);
}

static void XrefDef2Head() {
    word p;
    offset_t q, r;

    for (p = 0; p < dictCnt; p++) {
        infoIdx = dicttab[p];
        xrefIdx = GetScope();
        if (xrefIdx) {
            q = 0;
            SetScope(0);
            while (xrefIdx) {
                r = xreftab[xrefIdx].next;
                if ((xreftab[xrefIdx].line & 0x8000))
                    q = xrefIdx; /* definition */
                else {
                    xreftab[xrefIdx].next = GetScope();
                    SetScope(xrefIdx);
                }
                xrefIdx = r;
            }

            if (q) { /* insert definition at head */
                xrefIdx               = q;
                xreftab[xrefIdx].next = GetScope();
                SetScope(xrefIdx);
            }
        }
    }
}

void PrepXref() {
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

    xrefIdx = GetScope();
    if (xrefIdx == 0) {
        NewLineLst();
        return;
    }
    lstStr(": ");
    p = 0;

    while (xrefIdx) {
        if (p != xreftab[xrefIdx].line) {
            if (PWIDTH < col + 5) {
                NewLineLst();
                TabLst(-refContCol);
            }
            TabLst(1);
            lprintf("%d", xreftab[xrefIdx].line);
            p = xreftab[xrefIdx].line;
        }
        xrefIdx = xreftab[xrefIdx].next;
    }
    if (col)
        NewLineLst();
}

static void Sub_48A7() {
    curSym = GetSymbol();
    TabLst(-nameCol);
    lstStr(symtab[curSym].name->str);
    DotLst(attribCol - 2);
    lstc(' ');
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
    xrefIdx = GetScope();
    if (GetType() == BUILTIN_T)
        return;
    if (xrefIdx != 0 && (xreftab[xrefIdx].line & 0x8000) != 0) {
        TabLst(-defnCol);
        lprintf("%5d", 0x10000 - xreftab[xrefIdx].line); /* defn stored as -ve */
        SetScope(xreftab[xrefIdx].next);
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

    p       = infoIdx;
    infoIdx = GetBaseOffset();
    if (TestInfoFlag(F_MEMBER)) {
        r       = GetSymbol();
        infoIdx = GetParentOffset();
        q       = GetSymbol();
    } else {
        q = GetSymbol();
        r = 0;
    }

    curSym = q;
    lprintf(" BASED(%.*s", symtab[curSym].name->len, symtab[curSym].name->str);
    if (r) {
        lstc('.');
        curSym = r;
        lprintf("%.*s", symtab[curSym].name->len, symtab[curSym].name->str);
    }
    lstc(')');
    infoIdx = p;
}

static void Sub_4A42() {
    offset_t p;

    p       = infoIdx;
    infoIdx = GetParentOffset();
    curSym  = GetSymbol();
    lprintf(" MEMBER(%.*s)", symtab[curSym].name->len, symtab[curSym].name->str);
    infoIdx = p;
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
    curSym = GetSymbol();
    if (b66D8 != symtab[curSym].name->str[0]) {
        NewLineLst();
        b66D8 = symtab[curSym].name->str[0];
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

    for (p = 0; p < dictCnt; p++) {
        infoIdx = dicttab[p];
        if (GetType() == BUILTIN_T) {
            if (GetScope() != 0)
                Sub_4C84();
        } else
            Sub_4C84();
    }
} /* PrintRefs() */

// lifted
static void WrIxiBuf(void const *buf, word cnt) {
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
    infoIdx = procInfo[1];
    curSym  = GetSymbol();
    if (curSym) { /* Write() the module info */
        pstr_t const *pstr = symtab[curSym].name;
        WrIxiByte(0xff);    // module header
        WrIxiByte(22 + pstr->len);
        WrIxiByte(pstr->len);           /* module name len */
        WrIxiBuf((uint8_t *)pstr->str, pstr->len); /* module name */
    }
    // as name may be more than 10 characters and diskette name is always dashes
    // as an extension if the name is > 10 chars allow up to 19, overwriting
    // the diskette name
    // note only the file name and not the directory are used
    char const *name = basename(srcFileTable[0].fNam);
    int nameLen = (int)strlen(name);
    if (nameLen > 19)
        nameLen = 19;
    char ixname[] = "          ---------";
    memcpy(ixname, name, nameLen);
    WrIxiBuf((uint8_t *)ixname, 19);


    for (int p = 0; p < dictCnt; p++) {
        infoIdx   = dicttab[p];
        byte type = GetType();
        if (LABEL_T <= type && type <= PROC_T &&
            (TestInfoFlag(F_PUBLIC) || (TestInfoFlag(F_EXTERNAL) && !TestInfoFlag(F_AT)))) {

            WrIxiByte(TestInfoFlag(F_PUBLIC) ? 0 : 1);
            curSym             = GetSymbol();
            pstr_t const *pstr = symtab[curSym].name;
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

    Sub_4EC5();
    if (PRINT)
        LstModuleInfo();
    EndCompile();
    Exit(programErrCnt != 0);
}
