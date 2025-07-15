/****************************************************************************
 *  plm0g.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"

pstr_t const *CreateLit(word wTokenLen, char const *str) {
    pstr_t *lit;

    lit = safeMalloc(wTokenLen + 3);
    memcpy(lit->str, str, wTokenLen);
    memcpy(lit->str + wTokenLen, " \n", 2);
    lit->len = 255; /* put max size \n will terminate */
    return lit;
}
