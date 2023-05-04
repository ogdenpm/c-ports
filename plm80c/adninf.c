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

void AdvNxtInfo()
{
    while (1) {
        curInfoP += GetLen();
        if (curInfoP >= topInfo) {
            curInfoP = 0;
            return;
        } else if (GetType() != CONDVAR_T)
            return;
    }
}


