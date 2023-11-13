/****************************************************************************
 *  asxref.c: part of the C port of Intel's ISIS-II asm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"

// static const char cpyrite[] = "[C] 1976, 1977,1979 INTEL CORP\x2\x1";

typedef struct {
    char const *name;
    word head; // offset to line ref chain
} xref_t;

typedef struct {
    word lineNum;
    word next;
} line_t;

xref_t *xrefs;
int maxXrefs;
int cntXref;
#define XCHUNK 1024

line_t *lines;
int maxLines;
int cntLine;

#define LCHUNK 2048

int startMarker;
int nameWidth = 6;

static int NewLineRef(void) {
    if (cntLine >= maxLines)
        lines = xrealloc(lines, sizeof(line_t) * (maxLines += LCHUNK));
    return cntLine++;
}

static int NewXref(char const *name) {
    if (cntXref >= maxXrefs)
        xrefs = xrealloc(xrefs, sizeof(xref_t) * (maxXrefs += XCHUNK));
    xrefs[cntXref].name = AllocStr(name, false);
    return cntXref++;
}

static int FindXref(const char *name, bool *pFound) {
    for (int i = 0; i < cntXref; i++) {
        if (strcmp(name, xrefs[i].name) == 0) {
            *pFound = true;
            return i;
        }
    }
    *pFound = false;
    return NewXref(name);
}

void InsertXref(bool isDef, const char *name, word lineNum) {
    bool found;
    int nXref = FindXref(name, &found);
    int nLine = NewLineRef();
    if (isDef)
        lineNum |= 0x8000;

    lines[nLine].lineNum = lineNum;
    if (found) {
        lines[nLine].next             = lines[xrefs[nXref].head].next;
        lines[xrefs[nXref].head].next = nLine;
    } else
        lines[nLine].next = nLine;

    xrefs[nXref].head = nLine;
    if ((int)strlen(name) > nameWidth)
        nameWidth = (word)strlen(name);
}

static int GetLineIdx(byte from, int n, bool *pMoreLineRefs) {
    if (from == 0) {
        *pMoreLineRefs     = true;
        return startMarker = lines[xrefs[n].head].next;
    } else { /* ptr -> LineRef() */
        *pMoreLineRefs = startMarker != lines[n].next;
        return lines[n].next;
    }
}

static void OutputXref(void) {
    int refsPerLine, refsCnt;

    int nLine;
    bool moreLineRefs;

    refsPerLine = (pageWidth - nameWidth) / 7;
    subHeadIdx  = 1; // prevent (cont) for initial line
    PrintChar(FF);   // force new page
    subHeadIdx = 5;  // normal (cont) processing

    for (int i = 0; i < cntXref; i++) {
        char const *label = xrefs[i].name;
        nLine             = GetLineIdx(0, i, &moreLineRefs);
        while (moreLineRefs) {
            Printf("%-*s", nameWidth, label);
            refsCnt = 0;
            while (++refsCnt <= refsPerLine && moreLineRefs) {
                Printf(" %4d%c ", lines[nLine].lineNum & 0x7fff,
                       (lines[nLine].lineNum & 0x8000) ? '#' : ' ');
                nLine = GetLineIdx(1, nLine, &moreLineRefs);
            }
            PrintChar('\n'); // force paging check
            if (moreLineRefs)
                label = "";
        }
    }
}

int cmpName(void const *a, void const *b) {
    return strcmp(((xref_t const *)a)->name, ((xref_t const *)b)->name);
}

void GenAsxref(void) {
    qsort(xrefs, cntXref, sizeof(xref_t), cmpName);
    OutputXref();
    fputs("\nCROSS REFERENCE COMPLETE\n", lstFp);
}