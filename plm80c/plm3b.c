/****************************************************************************
 *  plm3b.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"

void WriteRec(pointer rec, uint8_t fixed) {
    uint8_t crc;
    size_t cnt;

    uint16_t len = getWord(&rec[REC_LEN]);
    if (len > 0 && OBJECT) {
        crc = 0;
        len += fixed + 1;
        putWord(&rec[REC_LEN], len);       // final record length
        cnt = len + 2;                     // allow for type, len but not crc uint8_t
        for (unsigned p = 0; p < cnt; p++) // calculate the crc
            crc -= rec[p];

        rec[cnt++] = crc; /* insert checksum */
        if (fwrite(rec, 1, cnt, objFile.fp) != cnt)
            IoError(objFile.fNam, "Write error");
    }
    putWord(&rec[REC_LEN], 0);
}

void RecAddByte(pointer rec, uint8_t offset, uint8_t val) {
    uint16_t len                     = getWord(&rec[REC_LEN]);
    rec[REC_DATA + len + offset] = val;
    putWord(&rec[REC_LEN], len + 1);
}

void RecAddWord(pointer rec, uint8_t offset, uint16_t val) {
    RecAddByte(rec, offset, Low(val));
    RecAddByte(rec, offset, High(val));
}
