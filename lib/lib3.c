/****************************************************************************
 *  lib3.c: part of the C port of Intel's ISIS-II lib             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "lib.h"

char const *SkipSpc(char const *chP)
{

	while (*chP == ' ')
		chP = chP + 1;
	return chP;
} /* SkipSpc */

char const *PastFileName(char const *chP)
{

	while (*chP == ':' || *chP == '.' || (*chP >= '0' && *chP <= '9') || (*chP >= 'A' && *chP <= 'Z'))
		chP = chP + 1;
	return chP;
} /* PastFileName */

