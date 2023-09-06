/****************************************************************************
 *  endcom.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include <stdio.h>
#include <stdlib.h>

void EndCompile() {
    printf("PL/M-80 COMPILATION COMPLETE.  %-5d PROGRAM ERROR%s\n\n", programErrCnt,
           programErrCnt == 1 ? "" : "S");
    if (programErrCnt && objFile.fp) {
        fclose(objFile.fp);
        unlink(objFile.fNam);
    }
}
