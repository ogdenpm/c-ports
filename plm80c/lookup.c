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
    byte hval = Hash(pstr);

    curSym    = hashTab[hval]; // find start of list in hash table
    for (index_t prevSym = 0; curSym; prevSym = curSym, curSym = symtab[curSym].link) {
        if (pstrequ(symtab[curSym].name, pstr)) {
            if (prevSym) {
                // move to front of list if not there already
                symtab[prevSym].link = symtab[curSym].link;
                symtab[curSym].link  = hashTab[hval];
                hashTab[hval]        = curSym;
            }
            return;
        }
    }
    curSym              = newSymbol(pstr);
    symtab[curSym].link = hashTab[hval];
    hashTab[hval]       = curSym;
} /* Lookup() */
