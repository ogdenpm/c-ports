/****************************************************************************
 *  plm3b.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void WriteRec(pointer recP, byte arg2b)
{
    word p;
    pointer lenP;
    byte crc;
    word cnt;

    lenP = ((rec_t *)recP)->len;
    if (getWord(lenP) > 0 && OBJECT ) {
        crc = 0;
        p = 0;
        putWord(lenP, getWord(lenP) + arg2b + 1);
        cnt = getWord(lenP) + 2;
        while (p < cnt)
            crc -= recP[p++];

        recP[cnt] = crc;	/* insert checksum */
        Fwrite(&objFile, recP, cnt + 1);
    }
    putWord(lenP, 0);
}



void RecAddByte(pointer recP, byte arg2b, byte arg3b)
{
    pointer lenP;

    lenP = ((rec_t *)recP)->len;
    ((rec_t *)recP)->val[getWord(lenP) + arg2b] = arg3b;
    putWord(lenP, getWord(lenP) + 1);
}



void RecAddWord(pointer arg1w, byte arg2b, word arg3w)
{
    RecAddByte(arg1w, arg2b, Low(arg3w));
    RecAddByte(arg1w, arg2b, High(arg3w));
}
