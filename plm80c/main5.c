/****************************************************************************
 *  main5.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "../shared/os.h"
#include "plm.h"
#include <stdlib.h>

byte maxSymLen;
byte groupingChar = 0;

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";

void LoadDictionary() {
    maxSymLen = 0;
    info      = infotab;
    AdvNxtInfo();
    while (info) {
        if (info->type < MACRO_T && info->sym) {
            newDict(info);
            if (info->sym->name->len > maxSymLen)
                maxSymLen = info->sym->name->len;
        }
        AdvNxtInfo();
    }
}

int CmpSym(void const *a, void const *b) {
    int cmp = strcmp((*(info_t **)a)->sym->name->str, (*(info_t **)b)->sym->name->str);
    return cmp ? cmp : (int)((uint8_t *)a - (uint8_t *)b);
}

void SortDictionary() {
    qsort(dicttab, topDict - dicttab, sizeof(info_t *), CmpSym);
}

// file scope due to nested procedure lift;
static byte defnCol, addrCol, sizeCol, nameCol, attribCol, refContCol;

static void PrintXrefs() {

    if (!XREF) {
        NewLineLst();
        return;
    }

    if (!info->xref) {
        NewLineLst();
        return;
    }
    lstStr(": ");
    word line = 0;
    for (xref_t *xref = info->xref; xref; xref = xref->next) {
        if (line != xref->line && !(xref->line & 0x8000)) { // ignore def & duplicates
            if (PWIDTH < col + 5) {
                NewLineLst();
                TabLst(-refContCol);
            }
            TabLst(1);
            lprintf("%d", xref->line);
            line = xref->line;
        }
    }
    if (col)
        NewLineLst();
}

static void DescribeName() {
    curSym = info->sym;
    TabLst(-nameCol);
    lstStr(curSym->name->str);
    DotLst(attribCol - 2);
    lstc(' ');
}

static void DescribeAddressAndSize(word addr, word size) {
    TabLst(-addrCol);
    lstStr(hexfmt(4, addr));
    if (size != 0) {
        TabLst(-sizeCol);
        lprintf("%5d", size);
    }
}

static void DescribeDefinition() {
    if (info->type == BUILTIN_T)
        return;
    xref_t *definition;
    // find any definition
    for (definition = info->xref; definition && !(definition->line & 0x8000);
         definition = definition->next)
        ;
    if (definition) {
        TabLst(-defnCol);
        lprintf("%5d", 0x10000 - definition->line); /* defn stored as -ve */
    } else if (!(info->flag & F_LABEL)) {
        TabLst(-defnCol);
        lstStr("-----");
    }
}

static void DescribeBased() {
    sym_t *baseSym, *member;
    info_t *baseInfo;

    baseInfo = info->baseInfo;
    if ((baseInfo->flag & F_MEMBER)) {
        member  = baseInfo->sym;
        baseSym = baseInfo->parent->sym;
    } else {
        baseSym = baseInfo->sym;
        member  = 0;
    }

    curSym = baseSym;
    lprintf(" BASED(%.*s", curSym->name->len, curSym->name->str);
    if (member) {
        lstc('.');
        curSym = member;
        lprintf("%.*s", curSym->name->len, curSym->name->str);
    }
    lstc(')');
}

static void DescribeMember() {
    curSym = info->parent->sym;
    lprintf(" MEMBER(%.*s)", curSym->name->len, curSym->name->str);
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
        lprintf(" STACK=%s", hexfmt(4, info->stackUsage));

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
    if (groupingChar != curSym->name->str[0]) {
        NewLineLst();
        groupingChar = curSym->name->str[0];
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

    /* PrintRefs() */
    defnCol    = 3;
    addrCol    = defnCol + 6;
    sizeCol    = addrCol + 6;
    nameCol    = sizeCol + 7;
    attribCol  = nameCol + maxSymLen + 2;
    refContCol = attribCol + 1;
    SetMarkerInfo(attribCol, 3);
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

    for (info_t **p = dicttab; p < topDict; p++) {
        info = *p;
        if (info->type != BUILTIN_T || info->xref)
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



#define IX_MOD      0xFF
#define IX_LONGNAME 0xFE
#define IX_PUBLIC   0
#define IX_EXTERNAL 1

void CreateIxrefFile() {
    curSym = procInfo[1]->sym;
    if (curSym) { /* Write() the module info */
        pstr_t const *pstr = curSym->name;
        WrIxiByte(IX_MOD); // module header
        WrIxiByte(22 + pstr->len);
        WrIxiByte(pstr->len);                      /* module name len */
        WrIxiBuf((uint8_t *)pstr->str, pstr->len); /* module name */
    }
    // For IXREF, the PL/M compilers write the file name padded with spaces
    // for a total of 10 chars.
    // Pre V4.0 this would be followed by the volume label, for V4.0
    // it is ten dashes
    // As this version supports longer filenames and is based on V4.0
    // If the file name has <= 10 chars, then the format is as per V4.0
    // 
    // 
    // For longer names up to the first 16 characters are written in a
    // padded with spaces if needed. This is then followed by a 0 to stop
    // the dot in the diskette name and two more spaces.
    // 
    // By doing this old versions of IXREF will show up to 16 char filenames
    // although there will be a couple of spaces following the first 10 chars
    // with the remaining chars being shown under diskette name
    // 
    // Additionally a new record type is written at the end of the
    // ixi file that contains the full filename.
    // Older versions of IXREF will report an unknown record and ignore it
    // the newer version will use it as the full filename
    // 
    // note only the file name and not the directory is used
    char const *name = basename(srcFileTable[0].fNam);
    uint16_t nameLen = (uint16_t)strlen(name);
    char nameRec[20];
    if (nameLen <= 10)  // original format
        sprintf(nameRec, "%-10s---------", name);
    else {
        sprintf(nameRec, "%-16.16s   ", name);
        nameRec[16] = '\0';     // prevent . in diskette name field
    }
    WrIxiBuf(nameRec, 19);

    for (info_t **p = dicttab; p < topDict; p++) {
        info      = *p;
        byte type = info->type;
        if (LABEL_T <= type && type <= PROC_T &&
            ((info->flag & (F_PUBLIC | F_EXTERNAL)) && !(info->flag & F_AT))) {
            WrIxiByte((info->flag & F_PUBLIC) ? IX_PUBLIC : IX_EXTERNAL);
            curSym             = info->sym;
            pstr_t const *pstr = curSym->name;
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
    // for long names i.e. > 10 chars append a special record that contains
    // the full name
    // older versions of IXREF will report an invalid record and will terminate
    // processing the file. As it is at the end it is safe to ignore

    if (nameLen > 10) {
        WrIxiByte(IX_LONGNAME);
        WrIxiWord(nameLen);
        WrIxiBuf((uint8_t *)name, nameLen);    
    }
    CloseF(&ixiFile);
}

void ProcessXref() {
    LoadDictionary();
    SortDictionary();
    //    XrefDef2Head();
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
