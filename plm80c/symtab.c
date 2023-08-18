#include "os.h"
#include "plm.h"

#define STCHUNK 2048

index_t symtabSize;
index_t symCnt = 1; // reserve 0
sym_t *symtab;

pstr_t const *pstrdup(pstr_t const *ps) {
    pstr_t *p = xmalloc(ps->len + 2);
    memcpy(p, ps, ps->len + 1); // copy pstring
    p->str[p->len] = '\0';      // make str also C compatible
    return p;
}

bool pstrequ(pstr_t const *ps, pstr_t const *pt) {
    return ps->len == pt->len && strncmp(ps->str, pt->str, ps->len) == 0;
}

index_t newSymbol(pstr_t const *ps) {
    if (symCnt >= symtabSize) {
        if (symtabSize + STCHUNK >= 0x10000)
            FatalError("Out of symbol space");
        else
            symtab = xrealloc(symtab, (symtabSize += STCHUNK) * sizeof(sym_t));
    }
    symtab[symCnt].link    = 0;
    symtab[symCnt].infoIdx = 0;
    symtab[symCnt].name    = pstrdup(ps);

    return symCnt++;
}

#define INCHUNK 2048
index_t infotabSize;
index_t infoCnt = 1;
info_t *infotab;

void newInfo(byte type) {
    if (infoCnt >= infotabSize) {
        if (infotabSize + INCHUNK >= 0x10000)
            FatalError("Out of info space");
        else
            infotab = xrealloc(infotab, (infotabSize += INCHUNK) * sizeof(info_t));
    }
    SetInfo(infoCnt++);
    memset(info, 0, sizeof(info_t));
    info->type = type;

}

#define DICHUNK 1024
index_t dictSize;
index_t dictCnt;
index_t *dicttab;

index_t newDict(index_t idx) {
    if (dictCnt >= dictSize) {
        if (dictSize + DICHUNK >= 0x10000)
            FatalError("Out of dictionary space");
        else
            dicttab = xrealloc(dicttab, (dictSize += DICHUNK) * sizeof(index_t));
    }
    dicttab[dictCnt] = idx;

    return dictCnt++;
}

#define CACHUNK 1024
index_t caseSize;
index_t caseCnt;
index_t *casetab;

index_t newCase(word val) {
    if (caseCnt >= caseSize) {
        if (caseSize + DICHUNK >= 0x10000)
            FatalError("Too many case statements");
        else
            casetab = xrealloc(casetab, (caseSize += DICHUNK) * sizeof(word));
    }
    casetab[caseCnt] = val;

    return caseCnt++;
}

#define XRCHUNK 1024
index_t xrefSize;
index_t xrefCnt = 1;
xref_t *xreftab;

index_t newXref(index_t scope, word line) {
    if (xrefCnt >= xrefSize) {
        if (xrefSize + XRCHUNK >= 0x10000)
            FatalError("Out of xref space");
        else
            xreftab = xrealloc(xreftab, (xrefSize += XRCHUNK) * sizeof(xref_t));
    }
    xreftab[xrefCnt].next = scope;
    xreftab[xrefCnt].line = line;

    return xrefCnt++;
}

#define ICHUNK  20
int newInclude(char const *fname) {
    for (int i = 0; i < includeCnt; i++) {
        if (strcmp(includes[i], fname) == 0)
            return i;
    
    }
    if (includeCnt % ICHUNK == 0)
        includes = xrealloc(includes, (includeCnt + ICHUNK) * sizeof(char *));
    includes[includeCnt] = fname;
    return includeCnt++;
    

}