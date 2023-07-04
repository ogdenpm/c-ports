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

uint16_t targetBase;
uint16_t unsatisfiedCnt;
static uint8_t curcol;

void EmitModDat(dataFrag_t *block) {
    InitRecord(R_MODDAT);
    WriteByte(SABS);
    WriteWord(block->saddr);
    EndUserRecord(AddrInMem(block->saddr), block->eaddr - block->saddr + 1);

}

void ForceSOL(void) {
    if (curcol) {
        Putc('\n');
        curcol = 0;
    }
}

#define CWIDTH (6 + 5 + 32) // value, type, symbol
void PrintColumn(uint8_t ctype, ...) {

    static uint8_t curType = 99;
    static uint8_t toPad;
    static char *fmt[] = { "VALUE TYPE SYMBOL", "%04X PUB  %.*s", "%04XH SYM  %.*s",
                           "%04XH LIN  %5d", "      MOD  %.*s" };

    if (curType != ctype || curcol == columns) {
        ForceSOL();
        curType = ctype;
    }
    if (curcol++)
        Printf("%*s", toPad, "");
    va_list args;
    va_start(args, ctype);
    toPad = CWIDTH - vPrintf(fmt[ctype], args);
    va_end(args);
}

uint8_t SetTargetSeg(uint8_t seg) { /* always returns 0 */
    targetBase = segBases[seg];
    return SABS;
}

void ProcModend(void) {
    uint8_t *rst0P;

    if (ReadByte() == 1) /* is main module */
    {
        isMain = true; /* flag as main module */
        SetTargetSeg(ReadByte());
        if (!seen.start) /* use the main module's address */
            startAddr = targetBase + ReadWord();
        if (seen.restart0) {       /* restart0 the put jump to startAddr */
            rst0P  = AddrInMem(0); /* location of address 0 in cache */
            *rst0P = 0xc3;         /* insert the jmp */
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
}

void ProcModdat(void) {
    uint16_t locRecLen = recLen;
    uint8_t seg        = ReadByte();
    if (seg == SSTACK)
        RecError("STACK segment has content"); /* ILLEGAL STACK CONTENT RECORD */

    SetTargetSeg(seg);
    uint16_t recordBase = targetBase;
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
            RecError("Record too long"); /* RECORD TOO LONG */
        AddDataFrag(inBlock.saddr, inBlock.eaddr);
    }
    LoadModdat(seg); /* suck the data in */
    GetRecord();
    if (locRecLen > 1025) /* can only happen for ABS blocks so no fixup */
        return;

    uint8_t *extFixup = NULL;
    int extFixupSize  = 0;

    while (inType == R_FIXEXT || inType == R_FIXLOC || inType == R_FIXSEG) {
        if (inType == R_FIXEXT) {
            // append a copy of the externals fix record(s) to emit after data
            extFixup          = xrealloc(extFixup, extFixupSize + recLen + 3);
            uint8_t *extStart = extFixup + extFixupSize;
            memcpy(extStart, inRec, recLen + 3);
            extFixupSize += recLen + 3;
            uint8_t *extIn  = extStart + 3;       // past type and len
            uint8_t *extEnd = extIn + recLen - 1; // point to CRC

            uint8_t fixType = *extIn++;
            if (fixType < FLOW || fixType > FBOTH)
                IllegalRecord();
            /* check the fixup record and adjust offsets */
            while (extIn < extEnd) {
                uint16_t nameIdx = getWord(extIn);
                if (nameIdx >= unsatisfiedCnt)
                    BadRecordSeq();                                /* name index out of range */
                uint32_t offset = recordBase + getWord(extIn + 2); // allow >64k
                if (offset < inBlock.saddr || inBlock.eaddr < offset + (fixType == FBOTH))
                    RecError("Fixup bounds error");
                putWord(extIn + 2, (uint16_t)offset);
                ForceSOL(); /* make sure on new line to record the problem */
                PrintfAndLog("REFERENCE TO UNSATISFIED EXTERNAL(%d) AT %04XH\n", nameIdx, offset);
                extIn += 4;
            }
            // work out new CRC
            uint8_t crc = 0;
            for (uint8_t *p = extStart; p < extEnd; p++)
                crc -= *p;
            *extEnd = crc;
        } else {
            targetBase = recordBase; /* restore fixup seg base to the moddat's own */
            if (inType == R_FIXSEG)  /* if different seg the set it up */
                SetTargetSeg(ReadByte());
            uint8_t fixType = ReadByte();

            while (inP < inEnd) {                          /* process the record */
                uint32_t offset = recordBase + ReadWord(); // allow offset >64k
                if (offset < inBlock.saddr || inBlock.eaddr < offset + (fixType == FBOTH))
                    RecError("Fixup bounds error");
                uint8_t *fixLoc = AddrInMem(offset);
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
        if (fwrite(extFixup, 1, extFixupSize, omfOutFp) != extFixupSize)
            IoError(omfOutName, "Write error");
        free(extFixup);
    } else
        AddDataFrag(inBlock.saddr, inBlock.eaddr); /* record the fragment */
}
