/****************************************************************************
 *  findmi.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void FindMemberInfo()
{
	word tmp;

	tmp = infoIdx;
    infoIdx =symtab[curSym].infoIdx;
	while (infoIdx != 0) {
		if (TestInfoFlag(F_MEMBER))
			if (tmp == GetParentOffset())
				return;
		infoIdx = GetLinkOffset();
	}
}
