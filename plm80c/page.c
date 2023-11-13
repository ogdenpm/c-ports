/****************************************************************************
 *  page.c: part of the C port of Intel's ISIS-II plm80                     *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

void NewPgl() {
    byte pad = 0;
    if (!PAGING)
        return;
    linLft         = PAGELEN;
    int dateWidth  = DATELEN;     // will be > 10 for auto generated date
    if (dateWidth > 10 && PWIDTH - 30 - TITLELEN < dateWidth)   // remove time if it doesn't fit
        dateWidth = 10;
    int titleWidth = PWIDTH - 30 - dateWidth;
    int titleLen   = TITLELEN <= titleWidth ? TITLELEN : titleWidth;
    if (titleWidth > titleLen)
        pad = titleWidth - titleLen;
    fprintf(lstFile.fp, "\fPL/M-80 COMPILER  %*s%.*s%*s  %*s  PAGE %3d\n\n\n", pad / 2, "",
            titleLen, TITLE, pad - pad / 2, "", dateWidth, DATE, ++pageNo);
    skipCnt = 0;
}

void NlLead() {
    WrLstC('\n');
    col = 0;

    if (--linLft == 0)
        NewPgl();
    while (col < wrapMarkerCol) {
        WrLstC(' ');
        col++;
    }
    WrLstC('-');
    while (++col < wrapTextCol)
        WrLstC(' ');
}
