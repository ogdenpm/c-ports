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

bool FindInfo() {
    if (symtab[curSym].infoIdx) {
        word depth = blockDepth;
        while (depth) {
            if (FindScopedInfo(procChains[depth--]))
                return true;
        }
    }
    infoIdx = 0;
    info    = NULL;
    return false;
}

void AdvNxtInfo() {
    while (++infoIdx < infoCnt) {
        if (infotab[infoIdx].type != CONDVAR_T) {
            info = &infotab[infoIdx];
            return;
        }
    }
    info    = NULL;
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

bool FindScopedInfo(word scope) {
    offset_t prev = 0;
    for (index_t idx = symtab[curSym].infoIdx; idx; prev = idx, idx = infotab[idx].ilink) {
        info_t *inf = &infotab[idx];
        if (scope == inf->scope) {
            if (inf->type == LIT_T || inf->type == MACRO_T || !(inf->flag & F_MEMBER)) {
                if (prev) {                                       /* not at start of Chain() */
                    infotab[prev].ilink = inf->ilink;             /* Move() to head of Chain() */
                    inf->ilink          = symtab[curSym].infoIdx; /* set its link to current head */
                    symtab[curSym].infoIdx = idx;                 /* set head to found info */
                }
                SetInfo(idx);
                return true;
            }
        }
    }
    infoIdx = 0;
    info    = NULL;
    return false;
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
