/****************************************************************************
 *  symtab.c: part of the C port of Intel's ISIS-II plm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"



sym_t symtab[MAXSYM];
sym_t *topSym = &symtab[1]; // 0 is reserved

pstr_t const *pstrdup(pstr_t const *ps) {
    pstr_t *p = safeMalloc(ps->len + 2);
    memcpy(p, ps, ps->len + 1); // copy pstring
    p->str[p->len] = '\0';      // make str also C compatible
    return p;
}

bool pstrequ(pstr_t const *ps, pstr_t const *pt) {
    return ps->len == pt->len && strncmp(ps->str, pt->str, ps->len) == 0;
}

sym_t *newSymbol(pstr_t const *ps) {
    if (topSym >= &symtab[MAXSYM])
            fatal("Out of symbol space");
    topSym->link           = 0;
    topSym->infoChain        = NULL;
    topSym->name           = pstrdup(ps);

    return topSym++;
}

info_t infotab[MAXINFO];
info_t *topInfo = &infotab[1];  // 0 reserved

void newInfo(byte type) {
    if (topInfo >= &infotab[MAXINFO])
            fatal("Out of info space");
    topInfo->type = type;
    info = topInfo++;
}


info_t *dicttab[MAXSYM];
info_t **topDict = dicttab;

void newDict(info_t *info) {
    if (topDict >= dicttab + MAXSYM)
            fatal("Out of dictionary space");

    *topDict++ = info;
}


index_t casetab[MAXCASE];
index_t topCase = 0;

index_t newCase(index_t val) {
    if (topCase >= MAXCASE)
            fatal("Too many case statements");
    casetab[topCase] = val;
    return topCase++;
}



xref_t xreftab[MAXXREF];
xref_t *topXref = xreftab;

xref_t *newXref(xref_t *xrefNext, word line) {
    if (topXref >= xreftab + MAXXREF)
            fatal("Out of cross reference space");
    topXref->next = xrefNext;
    topXref->line = line;

    return topXref++;
}

char const *includes[MAXINCLUDES];
uint16_t includeCnt;


int newInclude(char const *fname) {
    for (int i = 0; i < includeCnt; i++)
        if (strcmp(includes[i], fname) == 0)    // already known
            return i;
    if (includeCnt == MAXINCLUDES)
        fatal("Too many include files");
    includes[includeCnt] = fname;
    return includeCnt++;
}
