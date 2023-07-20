/****************************************************************************
 *  lstinf.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void LstModuleInfo()
{
	word p, q, r;
	p = 0;
	for (q = 1; q <= procCnt; q++) {
		infoIdx = procInfo[q];
		r = GetBaseVal();
		if (r > p )
			p = r;
	}
    SetSkipLst(3);
	lstStr("MODULE INFORMATION:\n");
    SetSkipLst(1);
	lprintf("     CODE AREA SIZE     = %s  %5dD\n", hexfmt(4, csegSize)->str, csegSize );
    lprintf("     VARIABLE AREA SIZE = %s  %5dD\n", hexfmt(4, dsegSize)->str, dsegSize);
    lprintf("     MAXIMUM STACK SIZE = %s  %5dD\n", hexfmt(4, p)->str, p);
    lprintf("     %d LINES READ\n", linesRead);
    lprintf("     %d PROGRAM ERROR%s\n", programErrCnt, programErrCnt != 1 ? "S" : "");
    SetSkipLst(1);
    lprintf("END OF PL/M-80 COMPILATION\n");
	fclose(lstFile.fp);
    lstFile.fp = NULL;
}
