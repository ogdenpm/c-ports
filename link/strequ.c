/****************************************************************************
 *  strequ.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"

bool PStrequ(pstr_t *s, pstr_t *t) {
    return s->len == t->len && strncmp(s->str, t->str, s->len) == 0;
}