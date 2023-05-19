#include "asm80.h"
#include <stdint.h>

#define SCHUNK 4096
/* functions to support string allocation */
typedef struct _str {
    struct _str *next;
    uint16_t used;
    char str[SCHUNK];
} strspace_t;

strspace_t *strings;
strspace_t *tmpStrings;

void resetTmpStrings() {
    for (strspace_t *p = tmpStrings; p; p = p->next)
        p->used = 0;
}

static char *AllocSpc(uint16_t spc, bool isTemp) {
    strspace_t *p;

    for (p = isTemp ? tmpStrings : strings; p; p = p->next) {
        if (SCHUNK - p->used >= spc) {
            p->used += spc;
            return p->str + p->used - spc;
        }
    }
    p = xmalloc(sizeof(strspace_t));
    if (isTemp) {
        p->next    = tmpStrings;
        tmpStrings = p;
    } else {
        p->next = strings;
        strings = p;
    }
    p->used = spc;
    return p->str;
}

char const *AllocStr(char const *s, bool isTemp) {
    char *t = AllocSpc((uint16_t)strlen(s) + 1, isTemp);
    return strcpy(t, s);
}

void *xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p)
        FatalError("Out of memory");
    return p;
}

void *xrealloc(void *p, size_t size) {
#ifdef _MSC_VER
#pragma warning(suppress: 6308)
#endif
    p = realloc(p, size);

    if (!p)
        FatalError("Out of memory");
    return p;
}