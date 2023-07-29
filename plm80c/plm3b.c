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
#include "os.h"

void WriteRec(pointer rec, byte fixed)
{
    byte crc;
    word cnt;

    word len = getWord(&rec[REC_LEN]);
    if (len > 0 && OBJECT ) {
        crc = 0;
        len += fixed + 1;
        putWord(&rec[REC_LEN], len);    // final record length
        cnt = len + 2;                  // allow for type, len but not crc byte
        for (int p = 0; p < cnt; p++)   // calculate the crc
            crc -= rec[p];

        rec[cnt] = crc;	/* insert checksum */
        if (fwrite(rec, 1, cnt + 1, objFile.fp) != cnt + 1)
            IoError(objFile.fNam, "Write error");
    }
    putWord(&rec[REC_LEN], 0);
}



void RecAddByte(pointer rec, byte offset, byte val)
{
    word len                                     = getWord(&rec[REC_LEN]);
    rec[REC_DATA + len + offset]  = val;
    putWord(&rec[REC_LEN], len + 1);
}



void RecAddWord(pointer rec, byte offset, word val)
{
    RecAddByte(rec, offset, Low(val));
    RecAddByte(rec, offset, High(val));
}
