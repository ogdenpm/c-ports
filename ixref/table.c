#include "ixref.h"
#include "os.h"
#include <stdlib.h>
#include <string.h>
#define MCHUNK 1024

int modtabSize;
int modCnt;
mod_t *modtab;

pstr_t const *pstrdup(pstr_t const *ps) {
    pstr_t *p = xmalloc(ps->len + 2);
    memcpy(p, ps, ps->len + 1); // copy pstring
    p->str[p->len] = '\0';      // make str also C compatible
    return p;
}

bool pstrequ(pstr_t const *ps, pstr_t const *pt) {
    return ps->len == pt->len && strncmp(ps->str, pt->str, ps->len) == 0;
}

char const *newmod(record_t const *rec) {
    if (modCnt >= modtabSize)
        modtab = xrealloc(modtab, (modtabSize += MCHUNK) * sizeof(mod_t));
    char *name = xmalloc(rec->len - 1);
    // copy module name to a C string
    memcpy(name, rec->data + 1, rec->data[0]);
    name[rec->data[0]]      = '\0';
    modtab[modCnt].name  = name;
    name += rec->data[0] + 2;  // append filename
    memcpy(name, &rec->data[rec->data[0] + 1], 19);
    // trim the filename;
    int padCh = '-';
    char *s   = name + 19;
    while (s != name && s[-1] == padCh)
        if (--s == name + 10)
            padCh = ' ';
    *s = '\0';
    modtab[modCnt].fname = name;
    return modtab[modCnt++].name;
}


