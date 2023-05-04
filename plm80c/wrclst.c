/****************************************************************************
 *  wrclst.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void WrcLst(char ch)
{
    lBufP[lChCnt] = ch;
    if (lChCnt == lBufSz) {
        if (! lfOpen) {
            OpenF(&lstFil, 2);
            lfOpen = true;
        }
        WriteF(&lstFil, lBufP, lBufSz + 1);
        lChCnt = 0;
    } else
        lChCnt++;
}

void Wr2cLst(word arg1w)
{
    char *bp;

    bp = (char *)&arg1w;
    WrcLst(bp[1]);
    WrcLst(bp[0]);
}

void WrnStrLst(char const *str, word cnt)
{

    while (cnt-- != 0)
        WrcLst(*str++);
}
