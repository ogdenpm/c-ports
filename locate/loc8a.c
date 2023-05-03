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

static char hexch[] = "0123456789ABCDEF";

word ParseNumber(char **ppstr)
{
    char *pch, *pafter, *pendNum;
    byte i, radix, digit = 99; /* avoids gcc warning */
    word num, lastnum;

    pch = *ppstr;
    while (*pch == ' ') {	/* skip spaces */
        pch++;
    }
    *ppstr = pch;		/* update the source *ppstr */

    while (('0' <= *pch && *pch <= '9') || ('A' <= *pch && *pch <= 'F')) {
        pch++;
    }
    pafter = (pendNum = pch) + 1;
    if (*pch == 'H')
        radix = 16;
    else if (*pch == 'O' || *pch == 'Q')
        radix = 8;
    else
    {	/* *pcheck for D or B or numeric */
        pendNum = pch = (pafter = pch) - 1;
        if (*pch == 'B')
            radix = 2;
        else
        {
            radix = 10;
            if (*pch != 'D')	/* if a digit then include in number */
                pendNum = pendNum + 1;
        }
    }
    pch = *ppstr;	/* reset to start */
    num = lastnum = 0;
    while (pch < pendNum) {
        for (i = 0; i <= 15; i++) {
            if (*pch == hexch[i])
                digit = i;
        }
        if (digit >= radix)
            return 0;	/* illegal number */
        if ((num = lastnum * radix + digit) < lastnum)
            return 0;	/* overflow */
        lastnum = num;
        pch++;
    }
    *ppstr = pafter;
    return num;
} /* ParseNumber */;


bool Strequ(char const *pstr1, char const *pstr2, byte len)
{
    byte i;
    for (i = 0; i < len; i++) {
        if (pstr1[i] != pstr2[i])
            return false;
    }
    return true;
} /* Strequ */
