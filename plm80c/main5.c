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
byte groupingChar = 0;

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";

void LoadDictionary() {
    dictCnt = maxSymLen = 0;
    infoIdx             = 0;
    AdvNxtInfo();
    while (infoIdx) {
        if (info->type < MACRO_T && info->sym != 0) {
            newDict(infoIdx);
            info->scope = 0; /* used for xref Chain() */
            curSym      = info->sym;
            if (symtab[curSym].name->len > maxSymLen)
                maxSymLen = symtab[curSym].name->len;
        }
        AdvNxtInfo();
    }
}

int CmpSym(void const *a, void const *b) {
    index_t ia = *(index_t *)a;
    index_t ib = *(index_t *)b;

    int cmp    = strcmp(symtab[infotab[ia].sym].name->str, symtab[infotab[ib].sym].name->str);
    return cmp ? cmp : ia - ib;
}

void SortDictionary() {
    qsort(dicttab, dictCnt, sizeof(index_t), CmpSym);
}

static void LoadXref() {
    byte xrefType;

    vfRewind(&xrff);
    while ((xrefType = vfRbyte(&xrff))) {
        word xrInfo = vfRword(&xrff);
        word stmt   = vfRword(&xrff);
        if (xrefType == 'B' || XREF) {
            SetInfo(xrInfo);
            xrefIdx = newXref(info->scope, xrefType == 'B' ? -stmt : stmt); /* make defn line -ve */
            info->scope = xrefIdx;
        }
    }
    vfRewind(&xrff);
}

static void XrefDef2Head() {
    word p;
    offset_t q, r;

    for (p = 0; p < dictCnt; p++) {
        SetInfo(dicttab[p]);
        xrefIdx = info->scope;
        if (xrefIdx) {
            q           = 0;
            info->scope = 0;
            while (xrefIdx) {
                r = xreftab[xrefIdx].next;
                if ((xreftab[xrefIdx].line & 0x8000))
                    q = xrefIdx; /* definition */
                else {
                    xreftab[xrefIdx].next = info->scope;
                    info->scope           = xrefIdx;
                }
                xrefIdx = r;
            }

            if (q) { /* insert definition at head */
                xrefIdx               = q;
                xreftab[xrefIdx].next = info->scope;
                info->scope           = xrefIdx;
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

static void PrintXrefs() {
    word p;

    if (!XREF) {
        NewLineLst();
        return;
    }

    xrefIdx = info->scope;
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

static void DescribeName() {
    curSym = info->sym;
    TabLst(-nameCol);
    lstStr(symtab[curSym].name->str);
    DotLst(attribCol - 2);
    lstc(' ');
}

static void DescribeAddressAndSize(word addr, word size) {
    TabLst(-addrCol);
    lstStr(hexfmt(4, addr)->str);
    if (size != 0) {
        TabLst(-sizeCol);
        lprintf("%5d", size);
    }
}

static void DescribeDefinition() {
    xrefIdx = info->scope;
    if (info->type == BUILTIN_T)
        return;
    if (xrefIdx && (xreftab[xrefIdx].line & 0x8000) != 0) {
        TabLst(-defnCol);
        lprintf("%5d", 0x10000 - xreftab[xrefIdx].line); /* defn stored as -ve */
        info->scope = xreftab[xrefIdx].next;
    } else if (!(info->flag & F_LABEL)) {
        TabLst(-defnCol);
        lstStr("-----");
    }
}


static void DescribeBased() {
    offset_t baseSym, member;
    info_t *baseInfo;

    baseInfo = &infotab[info->baseOff];
    if ((baseInfo->flag & F_MEMBER)) {
        member  = baseInfo->sym;
        baseSym = infotab[baseInfo->parent].sym;
    } else {
        baseSym = baseInfo->sym;
        member  = 0;
    }

    curSym = baseSym;
    lprintf(" BASED(%.*s", symtab[curSym].name->len, symtab[curSym].name->str);
    if (member) {
        lstc('.');
        curSym = member;
        lprintf("%.*s", symtab[curSym].name->len, symtab[curSym].name->str);
    }
    lstc(')');
}

static void DescribeMember() {
    curSym = infotab[info->parent].sym;
    lprintf(" MEMBER(%.*s)", symtab[curSym].name->len, symtab[curSym].name->str);
}

static void DescribeSimple(char const *attribute) {
    DescribeDefinition();
    DescribeName();
    lstStr(attribute);
    PrintXrefs();
}

static void DescribeProc() {
    DescribeDefinition();
    DescribeAddressAndSize(info->addr, info->codeSize);
    DescribeName();
    lstStr("PROCEDURE");
    if (info->returnType)
        lstStr(info->returnType == BYTE_T ? " BYTE" : " ADDRESS");
    if ((info->flag & F_PUBLIC))
        lstStr(" PUBLIC");

    if ((info->flag & F_EXTERNAL))
        lprintf(" EXTERNAL(%d)", info->extId);

    if ((info->flag & F_REENTRANT))
        lstStr(" REENTRANT");

    if ((info->flag & F_INTERRUPT))
        lprintf(" INTERRUPT(%d)", info->intno);
    if (!(info->flag & F_EXTERNAL))
        lprintf(" STACK=%s", hexfmt(4, info->stackUsage)->str);

    PrintXrefs();
}

static void DescribeVar(char const *str) {
    word size;

    DescribeDefinition();
    byte type = info->type;
    if (type == BYTE_T)
        size = 1;
    else if (type == ADDRESS_T)
        size = 2;
    else if (type == LABEL_T)
        size = 0;
    else
        size = info->totalSize;

    if ((info->flag & F_ARRAY))
        size *= info->dim;
    DescribeAddressAndSize(info->addr, size);
    DescribeName();
    lstStr(str);
    if ((info->flag & F_BASED))
        DescribeBased();
    if ((info->flag & F_ARRAY))
        lprintf(" ARRAY(%d)", info->dim);

    if ((info->flag & F_PUBLIC))
        lstStr(" PUBLIC");

    if ((info->flag & F_EXTERNAL))
        lprintf(" EXTERNAL(%d)", info->extId);

    if ((info->flag & F_AT))
        lstStr(" AT");
    if ((info->flag & F_DATA))
        lstStr(" DATA");
    if ((info->flag & F_INITIAL))
        lstStr(" INITIAL");

    if ((info->flag & F_MEMBER))
        DescribeMember();

    if ((info->flag & F_PARAMETER))
        lstStr(" PARAMETER");

    if ((info->flag & F_AUTOMATIC))
        lstStr(" AUTOMATIC");

    if ((info->flag & F_ABSOLUTE))
        lstStr(" ABSOLUTE");

    PrintXrefs();
}

static void DoOneXref() {
    curSym = info->sym;
    if (groupingChar != symtab[curSym].name->str[0]) {
        NewLineLst();
        groupingChar = symtab[curSym].name->str[0];
    }
    if (info->type < MACRO_T)
        switch (info->type) {
        case LIT_T:
            DescribeSimple("LITERALLY");
            break;
        case LABEL_T:
            DescribeVar("LABEL");
            break;
        case BYTE_T:
            DescribeVar("BYTE");
            break;
        case ADDRESS_T:
            DescribeVar("ADDRESS");
            break;
        case STRUCT_T:
            DescribeVar("STRUCTURE");
            break;
        case PROC_T:
            DescribeProc();
            break;
        case BUILTIN_T:
            DescribeSimple("BUILTIN");
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
        SetInfo(dicttab[p]);
        if (info->type == BUILTIN_T) {
            if (info->scope != 0)
                DoOneXref();
        } else
            DoOneXref();
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
    info   = &infotab[infoIdx = procInfo[1]];
    curSym = info->sym;
    if (curSym) { /* Write() the module info */
        pstr_t const *pstr = symtab[curSym].name;
        WrIxiByte(0xff); // module header
        WrIxiByte(22 + pstr->len);
        WrIxiByte(pstr->len);                      /* module name len */
        WrIxiBuf((uint8_t *)pstr->str, pstr->len); /* module name */
    }
    // as name may be more than 10 characters and diskette name is always dashes
    // as an extension if the name is > 10 chars allow up to 19, overwriting
    // the diskette name
    // note only the file name and not the directory are used
    char const *name = basename(srcFileTable[0].fNam);
    int nameLen      = (int)strlen(name);
    if (nameLen > 19)
        nameLen = 19;
    char ixname[] = "          ---------";
    memcpy(ixname, name, nameLen);
    WrIxiBuf((uint8_t *)ixname, 19);

    for (int p = 0; p < dictCnt; p++) {
        info      = &infotab[infoIdx = dicttab[p]];
        byte type = info->type;
        if (LABEL_T <= type && type <= PROC_T &&
            ((info->flag & (F_PUBLIC | F_EXTERNAL)) && !(info->flag & F_AT))) {
            WrIxiByte((info->flag & F_PUBLIC) ? 0 : 1);
            curSym             = info->sym;
            pstr_t const *pstr = symtab[curSym].name;
            WrIxiByte(6 + pstr->len);
            WrIxiByte(pstr->len);           /* module name len */
            WrIxiBuf(pstr->str, pstr->len); /* module name */

            WrIxiByte(type);
            if (info->type == PROC_T)
                WrIxiWord(info->returnType);
            else
                WrIxiWord((info->flag & F_ARRAY) ? info->dim : 0);
        }
    }
    CloseF(&ixiFile);
}

void ProcessXref() {
    LoadDictionary();
    SortDictionary();
    PrepXref();
    if ((XREF || SYMBOLS) && PRINT)
        PrintRefs();
    if (IXREF)
        CreateIxrefFile();
}

word Start5() {
    ProcessXref();
    if (PRINT)
        LstModuleInfo();
    EndCompile();
    Exit(programErrCnt != 0);
}
