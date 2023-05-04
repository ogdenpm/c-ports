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


static char endMsg[] = "PL/M-80 COMPILATION COMPLETE.  ";
static char errMsg[] = "XXXXX PROGRAM ERROR";

void EndCompile()
{
	Num2Asc(programErrCnt, 5, 10, errMsg);
	PrintStr(endMsg, Length(endMsg));
	PrintStr(errMsg, Length(errMsg));
	if (programErrCnt != 1 )
		PrintStr("S", 1);
	PrintStr("\r\n\r\n", 4);
}	

