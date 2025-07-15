/****************************************************************************
 *  emitm.c: part of the C port of Intel's ISIS-II asm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

// vim:ts=4:expandtab:shiftwidth=4:
//
#include "asm80.h"
#include "os.h"

static byte b6D7E[] = { 10, 0x12, 0x40 }; /* 11 bits 00010010010 index left to right */

static byte rModhdr[MODHDR_MAX + 4] = { 2 };
static byte *dtaP;
static pointer recSymP;
static pointer contentBytePtr;

static byte fixupSeg;
static word fixOffset;
static byte curFixupHiLoSegId;
static byte curFixupType;
static byte fixIdxs[4] = { 0, 0, 0, 0 };
#define fix22Idx   fixIdxs[0]
#define fix24Idx   fixIdxs[1]
#define fix20Idx   fixIdxs[2]
#define contentIdx fixIdxs[3]

static byte extNamIdx                 = 0;
static bool initFixupReq[4]           = { true, true, true, true };
static bool firstContent              = true;
static byte rEof[4]                   = { OMF_EOF, 0, 0 };
byte rExtnames[EXTNAMES_MAX + 4]      = { OMF_EXTNAMES };

static byte rContent[CONTENT_MAX + 4] = { OMF_CONTENT };
static byte rPublics[PUBLICS_MAX + 4] = { OMF_PUBLICS, 1, 0 };
#define rReloc rPublics // shared
static byte rInterseg[INTERSEG_MAX + 4];
static byte rExtref[EXTREF_MAX + 4];
static byte rModend[MODEND_MAX + 4] = { OMF_MODEND, 4, 0 };
static byte fixupInitialLen[]       = { 1, 2, 1, 3 };
static pointer fixupRecPtrs[]       = { rReloc, rInterseg, rExtref, rContent };
static byte fixupRecLenChks[] = { PUBLICS_MAX - 2, INTERSEG_MAX - 2, EXTREF_MAX - 4, CONTENT_MAX };
static word itemOffset;

/* to force equivalent behaviour for old code
   for the remaining space calculation assume 6 chars are needed
   for symbols, larger symbols will return their real length
*/
int minStrlen(char const *s) {
    int len = (int)strlen(s);
    return len < 6 ? 6 : len;
}
// portability helper functions
word setWord(pointer buf, word val) {
    buf[0] = val & 0xff;
    buf[1] = val >> 8;
    return val;
}

word getWord(pointer buf) {
    return buf[0] | (buf[1] << 8);
}

word getRecLen(pointer buf) {
    return buf[HDR_LEN] | (buf[HDR_LEN + 1] << 8);
}

word addRecLen(pointer buf, word val) {
    return setWord(&buf[HDR_LEN], getRecLen(buf) + val);
}

void WriteRec(pointer recP) {
    word recLen;
    byte i, crc;

    recLen = addRecLen(recP, 1) + 3; /* include crc byte + type + len word */
    for (crc = i = 0; i < recLen - 1; i++)
        crc -= recP[i];
    recP[recLen - 1] = crc; /* insert crc byte */
    if (fwrite(recP, 1, recLen, objFp) != recLen)
        IoError(objFile, "Write error");
}

static byte GetFixupType(void) {
    byte attr;
    if (((attr = tokenStk[spIdx].attr) & 0x5F) == 0)
        return 3;
    if ((attr & UF_EXTRN) != 0) /* external */
        return 2;
    if ((fixupSeg = attr & UF_SEGMASK) == 0) /* absolute */
        return 3;
    return (fixupSeg != activeSeg); // 0 - intra, 1 - inter
}

void ReinitFixupRecs(void) {
    byte i;
    pointer recP;
    /* order as content, reloc, interseg, externals */
    static byte recOrder[4] = { 3, 0, 1, 2 };

    for (i = 0; i < 4; i++) {
        int rec = recOrder[i];
        recP    = fixupRecPtrs[rec];
        if (getRecLen(recP) > fixupInitialLen[rec])
            WriteRec(recP);

        setWord(&recP[HDR_LEN], fixupInitialLen[rec]);
        fixIdxs[rec] = 0;
        if (curFixupType != rec)
            initFixupReq[rec] = true;
    }
    setWord(&rContent[CONTENT_OFFSET],
            itemOffset + segLocation[rContent[CONTENT_SEGID] = activeSeg]);
    rPublics[PUBLICS_SEGID]   = curFixupHiLoSegId;
    rInterseg[INTERSEG_SEGID] = tokenStk[spIdx].attr & 7;
    rInterseg[INTERSEG_HILO] = rExtref[EXTREF_HILO] = curFixupHiLoSegId;
}

static void AddFixupRec(void) {
    word effectiveOffset;
    pointer recP; // to check doesn't conflict with plm global usage

    recP = fixupRecPtrs[curFixupType = GetFixupType()];
    if (getRecLen(recP) > fixupRecLenChks[curFixupType] ||
        getRecLen(rContent) + tokenStk[spIdx].size > 124)
        ReinitFixupRecs();

    if (firstContent) {
        firstContent = false;
        setWord(&rContent[CONTENT_OFFSET],
                segLocation[rContent[CONTENT_SEGID] = activeSeg] + itemOffset);
    } else {
        // code lifted out of condition to force calculation
        effectiveOffset = getWord(&rContent[CONTENT_OFFSET]) + contentIdx;
        if (rContent[CONTENT_SEGID] != activeSeg ||
            effectiveOffset != segLocation[activeSeg] + itemOffset ||
            effectiveOffset < getWord(&rContent[CONTENT_OFFSET]))
            ReinitFixupRecs();
    }

    switch (curFixupType) {
    case 0:
        if (initFixupReq[0]) {
            initFixupReq[0]         = false;
            rPublics[PUBLICS_SEGID] = curFixupHiLoSegId;
        } else if (rPublics[PUBLICS_SEGID] != curFixupHiLoSegId)
            ReinitFixupRecs();
        break;
    case 1:
        if (initFixupReq[1]) {
            initFixupReq[1]           = false;
            rInterseg[INTERSEG_SEGID] = tokenStk[spIdx].attr & 7;
            rInterseg[INTERSEG_HILO]  = curFixupHiLoSegId;
        } else if (rInterseg[INTERSEG_HILO] != curFixupHiLoSegId ||
                   (tokenStk[spIdx].attr & 7) != rInterseg[INTERSEG_SEGID])
            ReinitFixupRecs();
        break;
    case 2:
        if (initFixupReq[2]) {
            initFixupReq[2]      = false;
            rExtref[EXTREF_HILO] = curFixupHiLoSegId;
        } else if (rExtref[EXTREF_HILO] != curFixupHiLoSegId)
            ReinitFixupRecs();
        break;
    case 3:
        break; /* abs no fixup */
    }
}

static void RecAddContentBytes(void) {
    for (byte i = 1; i <= tokenStk[spIdx].size; i++)
        rContent[CONTENT_DATA(contentIdx++)] = *contentBytePtr++;

    addRecLen(rContent, tokenStk[spIdx].size);
}

static void IntraSegFix(void) {
    addRecLen(rReloc, 2);
    setWord(&rReloc[RELOC_DATA(fix22Idx++)], fixOffset);
}

static void InterSegFix(void) {
    addRecLen(rInterseg, 2);
    setWord(&rInterseg[INTERSEG_DATA(fix24Idx++)], fixOffset);
}

static void ExternalFix(void) {
    setWord(&rExtref[EXTREF_DATA(fix20Idx++)], tokenStk[spIdx].symId);
    setWord(&rExtref[EXTREF_DATA(fix20Idx++)], fixOffset);
    addRecLen(rExtref, 4);
}

static void Sub7131(void) {
    curFixupHiLoSegId = (tokenStk[spIdx].attr & 0x18) >> 3;
    fixOffset         = segLocation[activeSeg] + itemOffset;
    if (!(inDB || inDW) && (tokenStk[spIdx].size == 2 || tokenStk[spIdx].size == 3))
        fixOffset++;
    AddFixupRec();
    contentBytePtr = lineBuf + startItem;
    RecAddContentBytes();
    switch (GetFixupType()) {
    case 0:
        IntraSegFix();
        break;
    case 1:
        InterSegFix();
        break;
    case 2:
        ExternalFix();
        break;
    case 3:
        break; /* no fixup as absolute */
    }
}

void WriteExtName(void) {
    if (getRecLen(rExtnames) + nameLen + 3 > PUBLICS_MAX) { /* check room for name */
        WriteRec(rExtnames);                                /* flush existing extNam Record() */
        rExtnames[HDR_TYPE] = OMF_EXTNAMES;
        setWord(&rExtnames[HDR_LEN], 0);
        extNamIdx = 0;
    }
    addRecLen(rExtnames, nameLen + 2);             /* update length for this ref */
    rExtnames[EXTNAMES_DATA(extNamIdx)] = nameLen; /* Write() len */
    extNamIdx++;
    strcpy((char *)&rExtnames[EXTNAMES_DATA(extNamIdx)], name);
    extNamIdx += nameLen + 1; /* update where next ref writes */
}

void AddSymbol(void) {
    if ((token.symbol->flags & UF_EXTRN) != 0)
        return;

    setWord(recSymP, token.symbol->addr);
    recSymP[2] = (byte)strlen(token.symbol->name);
    strcpy((char *)recSymP + 3, token.symbol->name);
    recSymP += 4 + recSymP[2];
}

void FlushSymRec(byte segId, byte isPublic) /* args because procedure is no longer nested */
{
    if (setWord(&rPublics[HDR_LEN], (word)(recSymP - &rPublics[PUBLICS_SEGID])) >
        1) /* something to Write() */
        WriteRec(rPublics);
    rPublics[HDR_TYPE]      = isPublic ? OMF_PUBLICS : OMF_LOCALS; /* PUBLIC or DoLocal */
    rPublics[PUBLICS_SEGID] = segId;
    recSymP                 = rPublics + PUBLICS_DATA;
}

static void WriteSymbols(byte isPublic) /* isPublic= true -> PUBLICs else LOCALs */
{
    byte segId;

    recSymP = rPublics + PUBLICS_DATA;
    for (segId = 0; segId <= 4; segId++) {
        FlushSymRec(segId, isPublic);          /* also sets up segid for new record */
        token.symbol = symTab[TID_SYMBOL] - 1; /* point to type byte of user symbol (-1) */

        while (++token.symbol < endSymTab[TID_SYMBOL]) {
            if (recSymP > &rPublics[PUBLICS_MAX + 3] -
                              (minStrlen(token.symbol->name) +
                               4)) /* make sure there is room offset, len, symbol, 0 */
                FlushSymRec(segId, isPublic);

            if ((token.symbol->flags & UF_SEGMASK) == segId && token.symbol->type != MACRONAME &&
                NonHiddenSymbol() &&
                !TestBit(token.symbol->type, b6D7E) && // not O_LABEL, O_REF or O_NAME
                (!isPublic || (token.symbol->flags & UF_PUBLIC) != 0))
                AddSymbol();
        }
        FlushSymRec(segId, isPublic);
    }
}

void WriteModhdr(void) {
    byte i;

    /* fill the module name */
    memcpy(&rModhdr[MODHDR_DATA(1)], moduleName, (rModhdr[MODHDR_DATA(0)] = moduleNameLen));
    dtaP    = &rModhdr[MODHDR_DATA(moduleNameLen + 1)];
    *dtaP++ = 0;                                      // trnId
    *dtaP   = 0;                                      // trnVn
    if (segLocation[SEG_CODE] < maxSegSize[SEG_CODE]) /* code segment */
        segLocation[SEG_CODE] = maxSegSize[SEG_CODE];
    if (segLocation[SEG_DATA] < maxSegSize[SEG_DATA]) /* data segment */
        segLocation[SEG_DATA] = maxSegSize[SEG_DATA];

    for (i = 1; i <= 4; i++) {
        *++dtaP = i;                     /* seg id */
        setWord(++dtaP, segLocation[i]); /* seg size */
        dtaP += 2;
        *dtaP = alignTypes[i - 1]; /* aln typ */
    }
    /* set record length (trnId/trnVn 4 * (segid, segLocation, align), crc) */
    setWord(&rModhdr[HDR_LEN], moduleNameLen + (1 + 2 + 4 * 4));
    WriteRec((pointer)&rModhdr);
}

void WriteModend(void) {
    rModend[MODEND_TYPE]  = startDefined;
    rModend[MODEND_SEGID] = startSeg;
    setWord(rModend + MODEND_OFFSET, startOffset);
    WriteRec(rModend);
    WriteRec(rEof);
}

void Ovl8(void) {
    itemOffset = 0;
    tokI       = 1;
    spIdx      = 1;
    if (b6B33)
        return;

    while (spIdx != 0) {
        spIdx     = NxtTokI();
        endItem   = tokenStk[spIdx].start + tokenStk[spIdx].size;
        startItem = tokenStk[spIdx].start;
        if (IsSkipping() || !isInstr)
            endItem = startItem;
        if (endItem > startItem) {
            Sub7131();
            itemOffset = itemOffset + tokenStk[spIdx].size;
        }
        if (!(inDB || inDW))
            spIdx = 0;
    }
}

void Ovl11(void) {
    if (externId != 0) {
        fseek(objFp, 0L, SEEK_SET); /* rewind */
        WriteModhdr();
        fseek(objFp, 0L, SEEK_END); /* back to end */
    }
    rPublics[HDR_TYPE] = OMF_PUBLICS; /* public declarations record */
    setWord(&rPublics[HDR_LEN], 1);
    rPublics[PUBLICS_SEGID] = SEG_ABS;
    rPublics[PUBLICS_DATA]  = 0;
    WriteSymbols(true); /* EMIT PUBLICS */
    if (controls.debug)
        WriteSymbols(false); /* EMIT LOCALS */
}

void InitRecTypes(void) {
    rContent[HDR_TYPE] = OMF_CONTENT;
    setWord(&rContent[HDR_LEN], 3);
    rPublics[HDR_TYPE] = OMF_RELOC;
    setWord(&rPublics[HDR_LEN], 1);
    rInterseg[HDR_TYPE] = OMF_INTERSEG;
    setWord(&rInterseg[HDR_LEN], 2);
    rExtref[HDR_TYPE] = OMF_EXTREF;
    setWord(&rExtref[HDR_LEN], 1);
}