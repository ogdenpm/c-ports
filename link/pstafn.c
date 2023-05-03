/****************************************************************************
 *  pstafn.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"

char const *ScanBlank(char const *pch)
{
	while (1) {
		pch = Delimit(pch);
		if (*pch == '*' || *pch == '?' )
			pch = pch + 1;
		else
			return pch;
	}
} /* ScanBlank() */
