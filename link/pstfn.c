/****************************************************************************
 *  pstfn.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"

char const *Delimit(char const *pch)
{

	while (*pch == ':' || *pch == '.' || (*pch >= '0' && *pch <= '9') || (*pch >= 'A' && *pch <= 'Z')) {
		pch++;
	}
	return pch;
} /* Delimit() */

