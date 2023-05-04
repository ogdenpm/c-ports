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

void NewPgl()
{
    byte j, k, m;
    char pnum[3];

    if (! PAGING)
        return;
    WrcLst(0xc);
    linLft = PAGELEN;
    pageNo = pageNo + 1;
    Num2Asc(pageNo, 3, 10, pnum);
    j = PWIDTH - 41;
    if (j < TITLELEN)
        m = j;
    else
        m = TITLELEN;
    WrnStrLst(plm80Compiler, 20);
    WrnStrLst(TITLE, m);
    k = j - m + 2;
    while (k != 0) {
        WrcLst(' ');
        k = k - 1;
    }
    WrnStrLst(DATE, 9);
    WrnStrLst("  PAGE ", 7);
    WrnStrLst(pnum, 3);
    WrnStrLst("\r\n\n\n", 4);
    skipCnt = 0;
}


void NlLead()
{
    Wr2cLst(0xd0a);
    col = 0;
    linLft = linLft - 1;
    if (linLft == 0)
        NewPgl();
    while (col < wrapMarkerCol) {
        WrcLst(' ');
        col = col + 1;
    }
    if (wrapMarker != 0) {
        WrcLst(wrapMarker);
        col = col + 1;
    }
    while (col < wrapTextCol) {
        WrcLst(' ');
        col = col + 1;
    }
}
