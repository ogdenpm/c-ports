/****************************************************************************
 *  asxref.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
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
byte row;
int nameWidth = 6;

static int AllocLineRef(void) {
    if (cntLine >= maxLines)
#ifdef _MSC_VER
#pragma warning(suppress : 6308)
#endif
        lines = xrealloc(lines, sizeof(line_t) * (maxLines += LCHUNK));
    return cntLine++;
}

static int AllocXref(char const *name) {
    if (cntXref >= maxXrefs)
#ifdef _MSC_VER
#pragma warning(suppress : 6308)
#endif
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
    return AllocXref(name);
}

void InsertXref(bool isDef, const char *name, word lineNum) {
    bool found;
    int nXref = FindXref(name, &found);
    int nLine = AllocLineRef();
    if (isDef)
        lineNum |= 0x8000;

    lines[nLine].lineNum = lineNum;
    if (found) {
        lines[nLine].next             = lines[xrefs[nXref].head].next;
        lines[xrefs[nXref].head].next = nLine;
    } else
        lines[nLine].next = nLine;

    xrefs[nXref].head = nLine;
    if (strlen(name) > nameWidth)
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

static void PageHeader(byte pageNum) {
    if (fprintf(lstFp,
                "\f\n\n\nISIS-II ASSEMBLER SYMBOL CROSS REFERENCE, V2.1                     PAGE "
                "%3d\n\n",
                pageNum) < 0)
        IoError(lstFile, "Write error");
    row = 7;
}

static void OutputXref(void) {
    byte refsPerLine, refsCnt, pageNum;

    int nLine;
    bool moreLineRefs;

    refsPerLine = (controls.pageWidth - nameWidth) / 7;
    pageNum     = 1;
    PageHeader(1);

    for (int i = 0; i < cntXref; i++) {
        char const *label = xrefs[i].name;
        nLine             = GetLineIdx(0, i, &moreLineRefs);
        while (moreLineRefs) {
            fprintf(lstFp, "%-*s", nameWidth, label);
            refsCnt = 0;
            while (++refsCnt <= refsPerLine && moreLineRefs) {
                fprintf(lstFp, " %4d%c ", lines[nLine].lineNum & 0x7fff,
                        (lines[nLine].lineNum & 0x8000) ? '#' : ' ');
                nLine = GetLineIdx(1, nLine, &moreLineRefs);
            }
            if (putc('\n', lstFp) == EOF)
                IoError(lstFile, "Write error");
            if (controls.paging)
                if (++row == controls.pageLength - 2)
                    PageHeader(++pageNum);
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
