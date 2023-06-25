/****************************************************************************
 *  loc2b.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "loc.h"
#include <stdarg.h>
#include <stdlib.h>


word targetBase;
word unsatisfiedCnt;
static byte curcol;

// helper function for none le systems
word putWord(pointer buf, word val) {
    buf[0] = val & 0xff;
    buf[1] = val >> 8;
    return val;
}

void WriteByte(uint8_t val) {
    *outP++ = val;
}

void WriteWord(uint16_t val) {
    *outP++ = (byte)val;
    *outP++ = val >> 8;
}

void WriteName(pstr_t const *name) {
    PStrcpy(name, outP);
    outP += name->len + 1;
}

void InitRecord(byte type) {
    outRec[REC_TYPE] = type;
    outP             = outRec + REC_DATA;
} /* InitRecord() */

void EndRecord() {
    // 1025 (+3 for type & len, -1 for pending crc)
    if (putWord(&outRec[REC_LEN], (word)(outP - outRec - 2)) > 1025)
        FatalError("%s: Record length > 1025", outName);
    byte crc;
    pointer p;
    for (crc = 0, p = outRec; p < outP; crc -= *p++) /* calculate and insert crc */
        ;
    *outP++ = crc; // add the crc
    if (fwrite(outRec, 1, outP - outRec, outFp) != outP - outRec)
        IoError(outName, "Write error");
}

void CopyRecord() {
    if (fwrite(inRec, 1, recLen + 3, outFp) != recLen + 3)
        IoError(outName, "Write error");
}

void EmitModDat(dataFrag_t *block) {
    word len;
    byte crc = 0;

    InitRecord(R_MODDAT);
    len = block->eaddr - block->saddr + 1;
    putWord(&outRec[REC_LEN], len + 4);
    WriteByte(SABS);
    WriteWord(block->saddr);
    byte *data = AddrInMem(block->saddr);
    // calculate the crc
    for (byte *p = outRec; p < outP; p++)
        crc -= *p;
    for (byte *p = data; p < data + len; p++)
        crc -= *p;
    // write data to file directly
    if (fwrite(outRec, 1, outP - outRec, outFp) != outP - outRec ||
        (word)fwrite(data, 1, len, outFp) != len || fputc(crc, outFp) == EOF)
        IoError(outName, "write error");
}

void ForceSOL() {
    if (curcol) {
        fputc('\n', lstFp);
        curcol = 0;
    }
} /* ForceSOL */

void PrintColumn(byte ctype, ...) {
#define CWIDTH (6 + 5 + 32)    // value, type, symbol
    static byte curType = 99;
    static byte toPad;
    static char *fmt[] = { "VALUE TYPE SYMBOL", "%04X PUB  %.*s", "%04XH SYM  %.*s", "%04XH LIN  %5d", "      MOD  %.*s" };

    if (curType != ctype || curcol == columns) {
        ForceSOL();
        curType = ctype;
    }
    if (curcol++)
        Printf( "%*s", toPad, "");
    va_list args;
    va_start(args, ctype);
    toPad = CWIDTH - vfprintf(lstFp, fmt[ctype], args);
    va_end(args);
} /* PrintColumn */

byte SetTargetSeg(byte seg) { /* always returns 0 */
    targetBase = segBases[seg];
    return SABS;
} /* SetWorkingSeg */

void ProcModend() {
    pointer rst0P;

    if (ReadByte() == 1) /* is main module */
    {
        isMain     = true; /* flag as main module */
        SetTargetSeg(ReadByte());
        if (!seen.start) /* use the main module's address */
            startAddr = targetBase + ReadWord();
        if (seen.restart0) /* restart0 the put jump to startAddr */
        {
            rst0P  = AddrInMem(0); /* location of address 0 in cache */
            *rst0P = 0xc3;           /* insert the jmp */
            putWord(rst0P + 1, startAddr);
            AddSegFrag(AABS, SABS, 0, 3); /* add to the frags */
            AddDataFrag(0, 2);
        }
    } else
        startAddr = 0; /* 0 start address if not a main module */
    for (int i = 0; i < dataFragCnt; i++)
        EmitModDat(&dataFrags[i]);

    InitRecord(R_MODEND);
    WriteByte(isMain);
    WriteByte(SABS);
    WriteWord(startAddr);
    EndRecord();
    InitRecord(R_MODEOF);
    EndRecord();
} /* ProcModend */

void ProcModdat() {
    word locRecLen = recLen;
    byte seg       = ReadByte();
    if (seg == SSTACK)
        RecError("STACK segment has content"); /* ILLEGAL STACK CONTENT RECORD */

    SetTargetSeg(seg);
    word recordBase = targetBase;
    /* set the bounds for this moddat record checking for wrap round */
    inBlock.saddr = recordBase + ReadWord();
    if (inBlock.saddr + recLen - 5 > 0xffff) // over 64k!!
        IllegalRecord();
    inBlock.eaddr = inBlock.saddr + recLen - 5;
    if (seg == SABS) /* add this as a new frag */
        AddSegFrag(0, 0, inBlock.saddr, recLen - 4);
    else if ((segFlags[seg] & AMASK) == AUNKNOWN)
        IllegalRecord();    /* align is invalid */
    if (locRecLen > 1025) { /* long records only ok if ABS */
        if (seg != SABS)
            RecError("record too long"); /* RECORD TOO LONG */
        AddDataFrag(inBlock.saddr, inBlock.eaddr);
    }
    LoadModdat(seg); /* suck the data in */
    GetRecord();
    if (locRecLen > 1025) /* can only happen for ABS blocks so no fixup */
        return;

    pointer extFixup = NULL;
    int extFixupSize = 0;

    while (inType == R_FIXEXT || inType == R_FIXLOC || inType == R_FIXSEG) {
        if (inType == R_FIXEXT) {
            // append a copy of the externals fix record(s) to emit after data
            extFixup         = xrealloc(extFixup, extFixupSize + recLen + 3);
            pointer extStart = extFixup + extFixupSize;
            memcpy(extStart, inRec, recLen + 3);
            extFixupSize += recLen + 3;
            pointer extIn  = extStart + 3;       // past type and len
            pointer extEnd = extIn + recLen - 1; // point to CRC

            byte fixType   = *extIn++;
            if (fixType < FLOW || fixType > FBOTH)
                IllegalRecord();
            /* check the fixup record and adjust offsets */
            while (extIn < extEnd) {
                word nameIdx = getWord(extIn);
                if (nameIdx >= unsatisfiedCnt)
                    BadRecordSeq();                                    /* name index out of range */
                uint32_t offset = recordBase + getWord(extIn + 2); // allow >64k
                if (offset < inBlock.saddr || inBlock.eaddr < offset + (fixType == FBOTH))
                    RecError("Fixup bounds error");
                putWord(extIn + 2, (word)offset);
                ForceSOL(); /* make sure on new line to record the problem */
                PrintfAndLog("REFERENCE TO UNSATISFIED EXTERNAL(%d) AT %04XH\n", nameIdx, offset);
                extIn += 4;
            }
            // work out new CRC
            byte crc = 0;
            for (pointer p = extStart; p < extEnd; p++)
                crc -= *p;
            *extEnd = crc;
        } else {
            targetBase = recordBase; /* restore fixup seg base to the moddat's own */
            if (inType == R_FIXSEG)          /* if different seg the set it up */
                SetTargetSeg(ReadByte());
            byte fixType = ReadByte();

            while (inP < inEnd) {                                      /* process the record */
                uint32_t offset = recordBase + ReadWord(); // allow offset >64k
                if (offset < inBlock.saddr || inBlock.eaddr < offset + (fixType == FBOTH))
                    RecError("Fixup bounds error");
                pointer fixLoc = AddrInMem(offset);
                switch (fixType) {
                case FLOW:
                    *fixLoc += Low(targetBase);
                    break;
                case FHIGH:
                    *fixLoc += High(targetBase);
                    break;
                case FBOTH:
                    putWord(fixLoc, getWord(fixLoc) + targetBase);
                    break;
                default:
                    IllegalRecord();
                    break;
                }
            }
        }
        GetRecord();
    }
    if (extFixup) {
        EmitModDat(&inBlock); /* emit the data record fragment as it needs fixup */
        if (fwrite(extFixup, 1, extFixupSize, outFp) != extFixupSize)
            IoError(outName, "write error");
        free(extFixup);
    } else
        AddDataFrag(inBlock.saddr, inBlock.eaddr); /* record the fragment */

} /* ProcModdat */
