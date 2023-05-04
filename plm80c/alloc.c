/****************************************************************************
 *  alloc.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void Alloc(word size1, word size2)
{
	blk1Used += size1;
	blk2Used += size2;
	if (blk1Used >= blkSize1 || blk2Used >= blkSize2)
		FatalError(ERR83);
} /* Alloc() */
