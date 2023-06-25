/****************************************************************************
 *  loc5b.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "loc.h"

char *Delimit(char *pch) {
    char *p = strpbrk(pch, " ,()&\n");
    return p ? p : strchr(pch, '\0');
}
