/****************************************************************************
 *  cpytil.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
void CpyTill(char const *srcP, char  *dstP, word cnt, byte endch)
{
	while (cnt-- != 0) {
		if (*srcP == endch )
			return;
		*dstP++ = *srcP++;
	}
} /* CpyTill() */
