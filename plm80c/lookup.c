/****************************************************************************
 *  lookup.c: part of the C port of Intel's ISIS-II plm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
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
    for (sym_t *prevSym = NULL; curSym; prevSym = curSym, curSym = curSym->link) {
        if (pstrequ(curSym->name, pstr)) {
            if (prevSym) {
                // move to front of list if not there already
                prevSym->link = curSym->link;
                curSym->link  = hashTab[hval];
                hashTab[hval]        = curSym;
            }
            return;
        }
    }
    curSym              = newSymbol(pstr);
    curSym->link = hashTab[hval];
    hashTab[hval]       = curSym;
} /* Lookup() */
