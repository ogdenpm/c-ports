/****************************************************************************
 *  backup.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void Backup(loc_t *locP, word cnt)
{ /* Backup() the block/byte pos by cnt bytes */

    word byt = cnt % 128;

    locP->blk -= cnt / 128;
    if (byt > locP->byt) {
        locP->blk--;
        locP->byt += 128 - byt;
    } else
        locP->byt -= byt;
}

