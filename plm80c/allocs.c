/****************************************************************************
 *  allocs.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

offset_t AllocSymbol(word spc)
{
	offset_t tmp;

	if ((tmp = botSymbol - spc) < topInfo )
		FatalError(ERR83);
	return (botSymbol = tmp);
} /* AllocSymbol() */

