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

void FindInfo() {
    word i;

    if (!symtab[curSym].infoIdx) {
        infoIdx = 0;
        info    = NULL;
        return;
    }
    i = blockDepth;
    while (i) {
        FindScopedInfo(procChains[i--]);
        if (infoIdx)
            return;
    }
}

void AdvNxtInfo() {
    while (++infoIdx < infoCnt) {
        if (infotab[infoIdx].type != CONDVAR_T) {
            info = &infotab[infoIdx];
            return;
        }
    }
    info = NULL;
    infoIdx = 0;
    return;
}

void FindMemberInfo() {
    offset_t parent;

    parent = infoIdx;
    for (infoIdx = symtab[curSym].infoIdx; infoIdx; infoIdx = infotab[infoIdx].ilink)
        if ((infotab[infoIdx].flag & F_MEMBER) && parent == infotab[infoIdx].parent) {
            info = &infotab[infoIdx];
            return;
        }
    info = NULL;
}

void FindScopedInfo(word scope) {
    offset_t p = 0;
    for (infoIdx = symtab[curSym].infoIdx; infoIdx; p = infoIdx, infoIdx = infotab[infoIdx].ilink) {
        info = &infotab[infoIdx];
        if (scope == info->scope) {
            if (info->type == LIT_T || info->type == MACRO_T || !(info->flag & F_MEMBER)) {
                if (p) {                                       /* not at start of Chain() */
                    infotab[p].ilink = info->ilink;            /* Move() to head of Chain() */
                    info->ilink      = symtab[curSym].infoIdx; /* set its link to current head */
                    symtab[curSym].infoIdx = infoIdx;          /* set head to found info */
                }
                return;
            }
        }
    }
}

void CreateInfo(word scope, byte type, index_t sym) {
    newInfo(type);
    if (sym) {
        info->ilink         = symtab[sym].infoIdx;
        symtab[sym].infoIdx = infoIdx;
    }
    info->scope = scope;
    info->sym   = sym;
} /* CreateInfo() */