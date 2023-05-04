/****************************************************************************
 *  fatal.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void Fatal(char const *str, byte len)
{
	PrintStr("\r\n\nPL/M-80 FATAL ERROR --\r\n\n", 28);
	PrintStr(str, len);
	PrintStr("\r\n\nCOMPILATION TERMINATED\r\n\n", 28);
	//if (debugFlag)
	//	REBOOTVECTOR();
	//else
		Exit();
} /* Fatal() */
