/****************************************************************************
 *  loc4a.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "loc.h"


_Noreturn void RecError(char const *errMsg) {
    fprintf(stderr, " %s", inName);

    if (moduleName)
        fprintf(stderr, "(%s)", moduleName->str);

    fprintf(stderr, ", %s\n Record Type %02XH, Record Number ", errMsg, inType);
    if (recNum > 0)
        fprintf(stderr, "%5d\n", recNum);
    else
        fputs("*****\n", stderr);

    Exit(1);
} /* FatalErr() */