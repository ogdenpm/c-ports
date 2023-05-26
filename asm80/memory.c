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

/* simple macro file replacement
   for now still uses 128 byte blocks
*/
#define MCHUNK  100  // allocate 100 blocks at a time ~12k
typedef struct _mfile {
    struct _mfile *next;
    byte blocks[MCHUNK][128];
} mfile_t;


mfile_t mfile;
word maxMacroBlk = 0;

static byte *blk2Buf(word blk) {
    mfile_t *p = &mfile;
    while (blk >= MCHUNK) {
        if (!p->next) {
            p->next = xmalloc(sizeof(mfile_t));
            p->next->next = NULL;
        }
        p       = p->next;
        blk -= MCHUNK;
    }
    return p->blocks[blk];
}

/* read in macro from virtual disk - located at given block */
void ReadM(word blk) {
    if (blk >= maxMacroBlk) // does not exist
        macroBuf[0] = MACROEOB;
    else if (blk == curMacroBlk) // already correct
        return;
    else { // load the buffer
        memcpy(macroBuf, blk2Buf(blk), 128);
        macroBuf[128] = MACROEOB;
    }
    curMacro.blk = curMacroBlk = blk;      // set relevant trackers

}


/* write the macro to virtual disk */
void WriteM(pointer buf) {
    if (phase == 1)          // only needs writing on pass 1
        memcpy(blk2Buf(maxMacroBlk++), buf, 128); // seek to end and update marker to account for this block
    macroBlkCnt++; // update the buffer count for this macro
}

