/****************************************************************************
 *  strequ.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"

bool Strequ(char const *s, char const *t, byte cnt)
{
	int i;

	for (i = 0; i < cnt; i++) {
		if (*s++ != *t++ )
			return false;
	}
	return true;
} /* Strequ() */
