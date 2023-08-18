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
    if (!PAGING)
        return;
    linLft         = PAGELEN;
    int titleWidth = PWIDTH - 41;
    int titlelen   = TITLELEN <= titleWidth ? TITLELEN : titleWidth;
    fprintf(lstFile.fp, "\fPL/M-80 COMPILER    %*.*s  %s  PAGE %3d\n\n\n", titleWidth, titlelen,
            TITLE, DATE, ++pageNo);
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
    if (wrapMarker) {
        WrLstC(wrapMarker);
        col++;
    }
    while (col < wrapTextCol) {
        WrLstC(' ');
        col++;
    }
}