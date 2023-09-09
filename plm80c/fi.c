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
    if (curSym->infoChain) {
        word depth = scopeSP;
        while (depth) {
            if (FindScopedInfo(scopeChains[depth--]))
                return true;
        }
    }
    infoIdx = 0;
    info    = NULL;
    return false;
}

void AdvNxtInfo() {
    while (++info < topInfo) {
        if (info->type != CONDVAR_T) {
            infoIdx = ToIdx(info);
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
    for (infoIdx = curSym->infoChain; infoIdx; infoIdx = infotab[infoIdx].ilink)
        if ((infotab[infoIdx].flag & F_MEMBER) && parent == infotab[infoIdx].parent) {
            info = &infotab[infoIdx];
            return;
        }
    info = NULL;
}

bool FindScopedInfo(word scope) {
    offset_t prev = 0;
    for (index_t idx = curSym->infoChain; idx; prev = idx, idx = infotab[idx].ilink) {
        info_t *inf = FromIdx(idx);
        if (scope == inf->scope) {
            if (inf->type == LIT_T || inf->type == MACRO_T || !(inf->flag & F_MEMBER)) {
                if (prev) {                                       /* not at start of Chain() */
                    infotab[prev].ilink = inf->ilink;             /* Move() to head of Chain() */
                    inf->ilink          = curSym->infoChain; /* set its link to current head */
                    curSym->infoChain = idx;                 /* set head to found info */
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

void CreateInfo(word scope, byte type, sym_t *sym) {
    newInfo(type);
    if (sym) {
        info->ilink         = sym->infoChain;
        sym->infoChain = infoIdx;
    }
    info->scope = scope;
    info->sym   = sym;
} /* CreateInfo() */
