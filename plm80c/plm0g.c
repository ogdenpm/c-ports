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
#include "os.h"

pstr_t const *CreateLit(pstr_t const *pstr)
{
    pstr_t *lit;

    lit = xmalloc(pstr->len + 3);
    memcpy(lit->str, pstr->str, pstr->len);
    memcpy(lit->str + pstr->len, " \n", 2);
    lit->len = 255;		/* put max size \n will terminate */
    return lit;
}


