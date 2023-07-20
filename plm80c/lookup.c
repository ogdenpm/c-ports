/****************************************************************************
 *  lookup.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

byte Hash(pstr_t *pstr) {
    byte hash = pstr->len;

    for (int i = 0; i < pstr->len; i++)
        hash = (hash << 1) + (hash >> 7) + pstr->str[i];

    return hash & 0x3F;
} /* Hash() */

void Lookup(pstr_t *pstr) {
    index_t p, q, r;
    word hval;

    hval   = Hash(pstr);
    curSym = hashTab[hval];
    p      = 0;
    while (curSym != 0) {
        if (pstrequ(symtab[curSym].name, pstr)) {
            if (p) {
                q                     = symtab[curSym].link;
                r                     = curSym;
                curSym                = p;
                symtab[curSym].link   = q;
                curSym                = r;
                symtab[curSym].link   = hashTab[hval];
                hashTab[hval]         = curSym;
            }
            return;
        }

        p      = curSym;
        curSym = symtab[curSym].link;
    }
    curSym = newSymbol(pstr);
    symtab[curSym].link = hashTab[hval];
    hashTab[hval]  = curSym;
} /* Lookup() */
