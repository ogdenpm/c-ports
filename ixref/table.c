/****************************************************************************
 *  table.c: part of the C port of Intel's ISIS-II ixref                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "ixref.h"
#include "os.h"
#include <stdlib.h>
#include <string.h>
#define MCHUNK 1024

int modtabSize;
int modCnt;
mod_t *modtab;

pstr_t const *pstrdup(pstr_t const *ps) {
    pstr_t *p = safeMalloc(ps->len + 2);
    memcpy(p, ps, ps->len + 1); // copy pstring
    p->str[p->len] = '\0';      // make str also C compatible
    return p;
}
char const *mkCStr(int len, char const *str) {
    char *s = safeMalloc(len + 1);
    memcpy(s, str, len);
    s[len] = '\0';
    return s;
}

bool pstrequ(pstr_t const *ps, pstr_t const *pt) {
    return ps->len == pt->len && strncmp(ps->str, pt->str, ps->len) == 0;
}

char const *newmod(record_t const *rec) {
    if (modCnt >= modtabSize)
        modtab = safeRealloc(modtab, (modtabSize += MCHUNK) * sizeof(mod_t));
    modtab[modCnt].name = mkCStr(rec->data[0], (char *)rec->data + 1);  // convert the module name
    modtab[modCnt].fname = mkCStr(rec->len - rec->data[0] - 3, (char *)rec->data + rec->data[0] + 1);
    return modtab[modCnt++].name;
}

void updateFname(FILE *fp) {
    int c1       = getc(fp);
    int c2 = getc(fp);
    if (c1 == EOF || c2 == EOF)
        return;
    int nameLen = c1 + 256 * c2;
    char *fname = safeMalloc(nameLen + 2); // allow for ' ' prefix and trailing '\0'
    if (fread(fname + 1, 1, nameLen, fp) != nameLen) {
        free(fname);
        return;
    }
    fname[0] = ' ';
    fname[nameLen + 1] = '\0';
    free((char *)(modtab[modCnt - 1].fname));
    modtab[modCnt - 1].fname = fname;
}