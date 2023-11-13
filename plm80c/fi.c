/****************************************************************************
 *  fi.c: part of the C port of Intel's ISIS-II plm80                       *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
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
    info    = NULL;
    return false;
}

void AdvNxtInfo() {
    while (++info < topInfo) {
        if (info->type != CONDVAR_T)
            return;
    }
    info    = NULL;
    return;
}

void FindMemberInfo() {
    info_t *parent;

    parent = info;
    for (info = curSym->infoChain; info; info = info->ilink)
        if ((info->flag & F_MEMBER) && parent == info->parent)
            return;
}

bool FindScopedInfo(word scope) {
    info_t *prev = 0;
    for (info = curSym->infoChain; info; prev = info, info = info->ilink) {
        if (scope == info->scope) {
            if (info->type == LIT_T || info->type == MACRO_T || !(info->flag & F_MEMBER)) {
                if (prev) {                                       /* not at start of Chain() */
                    prev->ilink = info->ilink;             /* Move() to head of Chain() */
                    info->ilink          = curSym->infoChain; /* set its link to current head */
                    curSym->infoChain = info;                 /* set head to found info */
                }
                return true;
            }
        }
    }
    return false;
}

void CreateInfo(word scope, byte type, sym_t *sym) {
    newInfo(type);
    if (sym) {
        info->ilink         = sym->infoChain;
        sym->infoChain = info;
    }
    info->scope = scope;
    info->sym   = sym;
} /* CreateInfo() */
