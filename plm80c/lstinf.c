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

void LstModuleInfo() {
    word maxStack = 0;

    for (int i = 1; i <= procCnt; i++) {
        info = procInfo[i];
        if (info->stackUsage > maxStack)
            maxStack = info->stackUsage;
    }

    SetSkipLst(3);
    lstStr("MODULE INFORMATION:\n");
    SetSkipLst(1);
    lprintf("     CODE AREA SIZE     = %0*XH  %5dD\n", csegSize >= 0xa000 ? 5 : 4, csegSize, csegSize);
    lprintf("     VARIABLE AREA SIZE = %0*XH  %5dD\n", dsegSize >= 0xa000 ? 5 : 4, dsegSize, dsegSize);
    lprintf("     MAXIMUM STACK SIZE = %0*XH  %5dD\n", maxStack >= 0xa000 ? 5 : 4, maxStack, maxStack);
    lprintf("     %d LINES READ\n", linesRead);
    lprintf("     %d PROGRAM ERROR%s\n", programErrCnt, programErrCnt != 1 ? "S" : "");
    SetSkipLst(1);
    lprintf("END OF PL/M-80 COMPILATION\n");
    fclose(lstFile.fp);
    lstFile.fp = NULL;
}
