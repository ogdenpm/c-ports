/****************************************************************************
 *  emitm.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// vim:ts=4:expandtab:shiftwidth=4:
//
#include "asm80.h"

static byte fixupInitialLen[] = { 1, 2, 1, 3 };
static pointer fixupRecPtrs[] = { rPublics, rInterseg, rExtref, rContent };
static byte fixupRecLenChks[] = { 123, 58, 57, 124 };
static byte b6D7E[]           = { 10, 0x12, 0x40 }; /* 11 bits 00010010010 index left to right */

static byte rModhdr[MODHDR_MAX + 4]         = { 2 };
static byte *dtaP;
static pointer recSymP;

// portability helper functions
word putWord(pointer buf, word val) {
    buf[0] = val & 0xff;
    buf[1] = val >> 8;
    return val;
}

word getWord(pointer buf) {
    return buf[0] + buf[1] * 256;
}

word getLen(pointer buf) {
    return buf[HDR_LEN] + buf[HDR_LEN + 1] * 256;
}

word addLen(pointer buf, word val) {
    return putWord(&buf[HDR_LEN], getLen(buf) + val);
}

void WriteRec(pointer recP) {
    word recLen;
    byte i, crc;

    recLen = addLen(recP, 1) + 3; /* include crc byte + type + len word */
    for (crc = i = 0; i < recLen - 1; i++)
        crc -= recP[i];
    recP[recLen - 1] = crc; /* insert crc byte */
    if (fwrite(recP, 1, recLen, objFp) != recLen)
        IoError(objFile, "Write error");
}

static byte GetFixupType(void) {
    byte attr;
    if (((attr = token[spIdx].attr) & 0x5F) == 0)
        return 3;
    if ((attr & UF_EXTRN) != 0) /* external */
        return 2;
    if ((fixupSeg = attr & UF_SEGMASK) == 0) /* absolute */
        return 3;
    return (fixupSeg != activeSeg);
}

void ReinitFixupRecs(void) {
    byte i;
    pointer recP;

    for (i = 0; i <= 3; i++) {
        ii   = (i - 1) & 3; /* order as content, publics, interseg, externals */
        recP = fixupRecPtrs[ii];
        if (getLen(recP) > fixupInitialLen[ii])
            WriteRec(recP);

        putWord(&recP[HDR_LEN], fixupInitialLen[ii]);
        fixIdxs[ii] = 0;
        if (curFixupType != ii)
            initFixupReq[ii] = true;
    }
    putWord(&rContent[CONTENT_OFFSET], itemOffset + segLocation[rContent[CONTENT_SEGID] = activeSeg]);
    rPublics[PUBLICS_SEGID]   = curFixupHiLoSegId;
    rInterseg[INTERSEG_SEGID] = token[spIdx].attr & 7;
    rInterseg[INTERSEG_HILO] = rExtref[EXTREF_HILO] = curFixupHiLoSegId;
}

static void AddFixupRec(void) {
    word effectiveOffset;
    pointer recP; // to check doesn't conflict with plm global usage

    recP = fixupRecPtrs[curFixupType = GetFixupType()];
    if (getLen(recP) > fixupRecLenChks[curFixupType] || getLen(rContent) + token[spIdx].size > 124)
        ReinitFixupRecs();

    if (firstContent) {
        firstContent = false;
        putWord(&rContent[CONTENT_OFFSET], segLocation[rContent[CONTENT_SEGID] = activeSeg] + itemOffset);
    } else {
        // code lifted out of condition to force calculation
        effectiveOffset = getWord(&rContent[CONTENT_OFFSET]) + fix6Idx;
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
            rInterseg[INTERSEG_SEGID] = token[spIdx].attr & 7;
            rInterseg[INTERSEG_HILO]  = curFixupHiLoSegId;
        } else if (rInterseg[INTERSEG_HILO] != curFixupHiLoSegId ||
                   (token[spIdx].attr & 7) != rInterseg[INTERSEG_SEGID])
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
    for (byte i = 1; i <= token[spIdx].size; i++)
        rContent[CONTENT_DATA(fix6Idx++)] = *contentBytePtr++;

    addLen(rContent, token[spIdx].size);
}

static void IntraSegFix(void) {
    addLen(rReloc, 2);
    putWord(&rReloc[RELOC_DATA(fix22Idx++)], fixOffset);
}

static void InterSegFix(void) {
    addLen(rInterseg, 2);
    putWord(&rInterseg[INTERSEG_DATA(fix24Idx++)], fixOffset);
}

static void ExternalFix(void) {
    putWord(&rExtref[EXTREF_DATA(fix20Idx++)], token[spIdx].symId);
    putWord(&rExtref[EXTREF_DATA(fix20Idx++)], fixOffset);
    addLen(rExtref, 4);
}

static void Sub7131(void) {
    curFixupHiLoSegId = (token[spIdx].attr & 0x18) >> 3;
    fixOffset         = segLocation[activeSeg] + itemOffset;
    if (!(inDB || inDW) && (token[spIdx].size == 2 || token[spIdx].size == 3))
        fixOffset++;
    AddFixupRec();
    contentBytePtr = startItem;
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
    byte i;

    if (getLen(rExtnames) + 9 > 125) { /* check room for name */
        WriteRec(rExtnames);           /* flush existing extNam Record() */
        rExtnames[HDR_TYPE] = OMF_EXTNAMES;
        putWord(&rExtnames[HDR_LEN], 0);
        extNamIdx = 0;
    }
    addLen(rExtnames, nameLen + 2);                /* update length for this ref */
    rExtnames[EXTNAMES_DATA(extNamIdx)] = nameLen; /* Write() len */
    extNamIdx++;
    for (i = 0; i <= nameLen; i++) /* and name */
        rExtnames[EXTNAMES_DATA(extNamIdx + i)] = name[i];

    rExtnames[EXTNAMES_DATA(extNamIdx + nameLen)] = 0; /* and terminating 0 */
    extNamIdx += nameLen + 1;                          /* update where next ref writes */
}

void AddSymbol(void) {

    if ((topSymbol->flags & UF_EXTRN) != 0)
        return;

    *(wpointer)recSymP = topSymbol->offset;
    UnpackToken(topSymbol->tok, (dtaP = (recSymP += 2) + 1));
    dtaP[6]  = ' '; /* trailing space to ensure end */
    *recSymP = 0;   /* length of symbol */

    while (dtaP[0] != ' ') { /* find *recSymPgth of name */
        ++*recSymP;
        dtaP++;
    }
    dtaP[0] = 0; /* terminate name with 0 */
    recSymP = (pointer)(dtaP + 1);
}

void FlushSymRec(byte segId, byte isPublic) /* args to because procedure is no longer nested */
{
    if (putWord(&rPublics[HDR_LEN], (word)(recSymP - &rPublics[PUBLICS_SEGID])) >
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
        FlushSymRec(segId, isPublic);       /* also sets up segid for new record */
        topSymbol = symTab[TID_SYMBOL] - 1; /* point to type byte of user symbol (-1) */

        while (++topSymbol < endSymTab[TID_SYMBOL]) { // converted for C pointer arithmetic */
            if (recSymP > &rPublics[PUBLICS_MAX + 3] -
                              (MAXSYMSIZE + 4)) /* make sure there is room offset, len, symbol, 0 */
                FlushSymRec(segId, isPublic);

            if ((topSymbol->flags & UF_SEGMASK) == segId && topSymbol->type != T_MACRONAME &&
                NonHiddenSymbol() &&
                !TestBit(topSymbol->type, b6D7E) && // not O_LABEL, O_REF or O_NAME
                (!isPublic || (topSymbol->flags & UF_PUBLIC) != 0))
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
        *++dtaP = i;                                /* seg id */
        putWord(++dtaP, segLocation[i]); /* seg size */
        dtaP += 2;
        *dtaP = alignTypes[i - 1]; /* aln typ */
    }
    /* set record length (trnId/trnVn 4 * (segid, segLocation, align), crc) */
    putWord(&rModhdr[HDR_LEN], moduleNameLen + (2 + 4 * 4 + 1));
    WriteRec((pointer)&rModhdr);
}

void WriteModend(void) {
    rModend[MODEND_TYPE]  = startDefined;
    rModend[MODEND_SEGID] = startSeg;
    putWord(rModend + MODEND_OFFSET, startOffset);
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
        endItem   = token[spIdx].start + token[spIdx].size;
        startItem = token[spIdx].start;
        if (IsSkipping() || !isInstr)
            endItem = startItem;
        if (endItem > startItem) {
            Sub7131();
            itemOffset = itemOffset + token[spIdx].size;
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
    putWord(&rPublics[HDR_LEN], 1);
    rPublics[PUBLICS_SEGID] = SEG_ABS;
    rPublics[PUBLICS_DATA]  = 0;
    WriteSymbols(true); /* EMIT PUBLICS */
    if (controls.debug)
        WriteSymbols(false); /* EMIT LOCALS */
}
