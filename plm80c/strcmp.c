/****************************************************************************
 *  strcmp.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
byte Strncmp(char const *s1, char const *s2, byte cnt)
{
	while (cnt != 0) {
		if (*s1 < *s2 )
			return 1;
		if (*s1 > *s2 )
			return 2;
		cnt = cnt - 1;
		s1++; s2++;
	}
	return 0;
} /* Strncmp() */
