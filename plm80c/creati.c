/****************************************************************************
 *  creati.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void CreateInfo(word scope, byte type)
{
    infoIdx = newInfo(type);
    if (curSym) {
        SetLinkOffset(symtab[curSym].infoIdx);
        symtab[curSym].infoIdx = infoIdx;
    }
    SetScope(scope);
    SetSymbol(curSym);
} /* CreateInfo() */
