/****************************************************************************
 *  fi.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void FindInfo()
{
    word i;

    if (!symtab[curSym].infoIdx) {
        infoIdx = 0;
        return;
    }
    i = blockDepth;
    while (i) {
        FindScopedInfo(procChains[i--]);
        if (infoIdx)
            return;
    }
}

index_t AdvNxtInfo(index_t idx) {
    while (1) {
        if (++idx >= infoCnt)
            return 0;
        else if (infotab[idx].type != CONDVAR_T)
            return idx;
    }
}

void FindMemberInfo() {
    word tmp;

    tmp     = infoIdx;
    infoIdx = symtab[curSym].infoIdx;
    while (infoIdx != 0) {
        if (TestInfoFlag(F_MEMBER))
            if (tmp == GetParentOffset())
                return;
        infoIdx = GetLinkOffset();
    }
}


void FindScopedInfo(word scope) {
    word p;
    byte infoType;

    infoIdx = symtab[curSym].infoIdx;
    p       = 0;
    while (infoIdx) {
        if (scope == GetScope()) {
            infoType = GetType();
            if (infoType == LIT_T || infoType == MACRO_T || !TestInfoFlag(F_MEMBER)) {
                if (p) {                                   /* not at start of Chain() */
                    infotab[p].ilink = GetLinkOffset();    /* Move() to head of Chain() */
                    SetLinkOffset(symtab[curSym].infoIdx); /* set its link to current head */
                    symtab[curSym].infoIdx = infoIdx;      /* set head to found info */
                }
                return;
            }
        }
        p       = infoIdx;
        infoIdx = GetLinkOffset();
    }
}


void CreateInfo(word scope, byte type) {
    infoIdx = newInfo(type);
    if (curSym) {
        SetLinkOffset(symtab[curSym].infoIdx);
        symtab[curSym].infoIdx = infoIdx;
    }
    SetScope(scope);
    SetSymbol(curSym);
} /* CreateInfo() */