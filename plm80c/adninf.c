/****************************************************************************
 *  adninf.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

index_t AdvNxtInfo(index_t idx)
{
    while (1) {
        if (++idx >= infoCnt)
            return 0;
        else if (GetType() != CONDVAR_T)
            return idx;
    }
}


