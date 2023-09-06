/****************************************************************************
 *  page.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void NewPgl() {
    byte pad = 0;
    if (!PAGING)
        return;
    linLft         = PAGELEN;
    int titleWidth = PWIDTH - 30 - (int)strlen(DATE);
    int titlelen   = TITLELEN <= titleWidth ? TITLELEN : titleWidth;
    if (titleWidth > titlelen)
        pad = titleWidth - titlelen;
    fprintf(lstFile.fp, "\fPL/M-80 COMPILER  %*s%.*s%*s  %s  PAGE %3d\n\n\n", pad / 2, "",
            titlelen, TITLE, pad - pad / 2, "", DATE, ++pageNo);
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
