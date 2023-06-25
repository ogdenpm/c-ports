/****************************************************************************
 *  loc8a.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "loc.h"
#include <ctype.h>

// modified to return -1 on error
int ParseNumber(char const *token) {
    char const *pch;
    byte radix, digit;
    int num;

    for (pch = token; isxdigit(*pch); pch++)
        ;
    char suffix = toupper(*pch);
    if (suffix == 'H')
        radix = 16;
    else if (suffix == 'O' || suffix == 'Q')
        radix = 8;
    else if (*pch != '\0')
        return -1;
    else if (toupper(pch[-1]) == 'B' || toupper(pch[-1]) == 'D') {
        if (--pch == token)
            return -1;
        radix = toupper(pch[-1]) == 'B' ? 2 : 10;
    } else
        radix = 10;

    for (num = 0; token < pch; token++) {
        digit = isdigit(*token) ? *token - '0' : toupper(*token) - 'A' + 10;
        if (digit >= radix)
            return -1;
        num = num * radix + digit;
        if (num > 0xffff)
            return -1;
    }
    return num;
} /* ParseNumber */;
