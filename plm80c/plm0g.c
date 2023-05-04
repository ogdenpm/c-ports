/****************************************************************************
 *  plm0g.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

offset_t CreateLit(pointer pstr)
{
    word litLen;
    offset_t litSymbol;

    litLen = pstr[0] + 1;
    litSymbol = AllocSymbol(litLen + 3);
    memmove(ByteP(litSymbol + 1), pstr, litLen);
    memmove(ByteP(litSymbol + litLen + 1), " \n", 2);
    ByteP(litSymbol)[0] = 255;		/* put max size \n will terminate */
    return litSymbol;
}


