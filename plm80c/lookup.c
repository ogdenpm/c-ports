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

byte Hash(pstr_t *pstr)
{
    pointer p = (pointer)pstr;
    byte len = *p;
    byte hash = 0;

    while (len != 0) {
        hash = (hash << 1) + (hash & 0x80 ? 1 : 0) + *p++;
        len = len - 1;
    }
    return hash & 0x3F;
} /* Hash() */



void Lookup(pstr_t *pstr)
{
	offset_t p, q, r;
    word hval;
	byte cmp;

	hval = Hash(pstr);
	curSymbolP = WordP(hashChainsP)[hval];
	p = 0;
	while (curSymbolP != 0) {
		if (SymbolP(curSymbolP)->name.len == pstr->len) {
			cmp = Strncmp(SymbolP(curSymbolP)->name.str, pstr->str, pstr->len);
			if (cmp == 0) {
				if (p != 0 ) {
					q = SymbolP(curSymbolP)->link;
					r = curSymbolP;
					curSymbolP = p;
					SymbolP(curSymbolP)->link = q;
					curSymbolP = r;
					SymbolP(curSymbolP)->link = WordP(hashChainsP)[hval];
					WordP(hashChainsP)[hval] = curSymbolP;
				}
				return;
			}
		}
		p = curSymbolP;
		curSymbolP = SymbolP(curSymbolP)->link;
	}
	Alloc(0, pstr->len + 1);
	curSymbolP = AllocSymbol(sizeof(sym_t) + pstr->len);
	memmove(&SymbolP(curSymbolP)->name, pstr, pstr->len + 1);
	SymbolP(curSymbolP)->infoP = 0;
	SymbolP(curSymbolP)->link = WordP(hashChainsP)[hval];
	WordP(hashChainsP)[hval] = curSymbolP;
} /* Lookup() */
