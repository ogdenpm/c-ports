/****************************************************************************
 *  linkov.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"
#include <stdlib.h>

// static char COPYRIGHT[] = "[C] 1976, 1977, 1979 INTEL CORP";
char OVERLAYVERSION[]     = "V3.0";
static word zero          = 0;
static byte fakeModhdr[1] = { 6 };
static byte space         = ' ';

char msgrefin[]           = " - REFERENCED IN ";

psModName_t modName;
static byte ancestorNameNotSet;
static byte fixType;
static word segIdx; // converted to word to handle 0-255 loops
// static byte pad[3];
static word segId;
static word outRelocOffset;
static symbol_t **extMapP = 0;
static word externsCount  = 0;
record_t *outRecordP;

void AddExtMap(symbol_t *symP) {
    extMapP[externsCount++] = symP; /* record the symbol mapping */
} /* AddExtMap() */

symbol_t *GetSymbolP(word symId) {

    if (symId >= externsCount) /* out of range */
        IllFmt();
    return extMapP[symId]; /* return the symbolP */
} /* GetSymbolP() */

void InitExternsMap() {
    if (extMapP == 0) /* ! already allocated */
        extMapP = (symbol_t **)malloc(maxExternCnt * sizeof(symbol_t **));
    externsCount = 0;
} /* InitExternsMap() */

void FlushTo() {
    Write(tofilefd, soutP, (word)(outP - soutP), &statusIO);
    FileError(statusIO, toFileName, true);
    outP = soutP;
} /* FlushTo() */

void InitRecord(byte type) {
    if (eoutP - outP < 1028)
        FlushTo();
    outRecordP         = (record_t *)outP;
    outRecordP->rectyp = type;
    outP += 3;
} /* InitRecord() */

void EndRecord() {
    byte crc;
    pointer pch;

    if ((outRecordP->reclen = (word)(outP - &outRecordP->rectyp - 2)) > 1025)
        FileError(ERR211, toFileName, true); /* Record() to long */
    crc = 0;
    for (pch = &outRecordP->rectyp; pch <= outP - 1; pch++) { /* calculate and insert crc */
        crc += *pch;
    }
    *outP++ = -crc; // add the crc
} /* EndRecord() */

bool ExtendRec(word cnt) {
    byte type;

    if (outP + cnt - &outRecordP->rectyp < 1028) /* room in buffer */
        return false;
    type = outRecordP->rectyp; /* type for extension record */
    EndRecord();               /* Close() off the current record */
    InitRecord(type);          /* and prepare another */
    return true;
} /* ExtendRec() */

void static EmitMODHDRComSegInfo(byte segId, word len, byte combine) {
    if (ExtendRec(SEGDEF_sizeof))                /* make sure enough room */
        FileError(ERR226, toFileName, true); /* mod hdr too long */
    outP[SEGDEF_segId] = segId;                  /* emit segid, name, combine and Size() */
    putWord(outP + SEGDEF_len, len);
    outP[SEGDEF_combine] = combine;
    outP += SEGDEF_sizeof;
} /* EmitMODHDRComSegInfo() */

void EmitMODHDR() {
    InitRecord(R_MODHDR);
    Pstrcpy(&outModuleName, outP); /* copy the module name */
    outP += outModuleName.len + 1;
    *outP++ = outTranId; /* the two reserved bytes */
    *outP++ = outTranVn;
    for (segIdx = SEG_CODE; segIdx <= SEG_MEMORY; segIdx++) { /* regular segments */
        if (segLen[segIdx] > 0)
            EmitMODHDRComSegInfo((byte)segIdx, segLen[segIdx], alignType[segIdx]);
    }
    if (segLen[0] > 0) /* unamed common segment */
        EmitMODHDRComSegInfo(SEG_BLANK, segLen[0], alignType[0]);

    /* named common segments */
    for (comdefInfoP = headSegOrderLink; comdefInfoP; comdefInfoP = comdefInfoP->nxtSymbol)
        EmitMODHDRComSegInfo(comdefInfoP->linkedSeg, comdefInfoP->len, comdefInfoP->flags);
    EndRecord();
} /* EmitMODHDR() */

void EmitEnding() {
    InitRecord(R_MODEND);                /* init the record */
    outP[MODEND_modType] = modEndModTyp; /* fill in the mod type, start seg and offset */
    outP[MODEND_segId]   = modEndSegId;
    putWord(outP + MODEND_offset, modEndOffset);
    outP += MODEND_sizeof; /* advance past the data inserted */
    EndRecord();           /* finalise */
    InitRecord(R_MODEOF);  /* emit and eof record as well */
    EndRecord();
    FlushTo(); /* make sure all on disk */
} /* EmitEnding() */

void EmitCOMDEF() {
    if (!headSegOrderLink) /* no named common */
        return;
    InitRecord(R_COMDEF); /* prep the record */
    /* chase down the definitions in seg order */
    for (comdefInfoP = headSegOrderLink; comdefInfoP; comdefInfoP = comdefInfoP->nxtSymbol) {
        if (ExtendRec(2 + comdefInfoP->name.len)) /* overflow to another record if needed */
            ; /* if (really not needed here as there are no issues with overflowing */
        *outP = comdefInfoP->linkedSeg; /* copy the seg and name */
        Pstrcpy(&comdefInfoP->name, outP + 1);
        outP += COMNAM_sizeof + comdefInfoP->name.len; /* advance output pointer */
    }
    EndRecord(); /* finalise */
} /* EmitCOMDEF() */

void EmitPUBLICS() {
    for (segIdx = 0; segIdx <= 255; segIdx++) { /* scan all segs */
        if (segmap[segIdx])                     /* seg used */
        {
            InitRecord(R_PUBDEF);   /* init the record */
            *outP++ = (byte)segIdx; /* seg needed */
            /* scan all files */
            for (curObjFile = objFileHead; curObjFile; curObjFile = curObjFile->link) {
                /* && all modules */
                for (curModule = curObjFile->modList; curModule; curModule = curModule->link) {
                    /* && all symbols */
                    for (symbolP = curModule->symlist; symbolP; symbolP = symbolP->nxtSymbol) {
                        if (symbolP->linkedSeg == segIdx) { /* this symbol in the right seg */
                            /* makes sure enough room in record */
                            if (ExtendRec(4 + symbolP->name.len)) {
                                /* overflowed to new record so add the segid */
                                *outP++ = (byte)segIdx;
                            }
                            /* Write() the offset */
                            putWord(outP + DEF_offset, symbolP->offsetOrSym);
                            Pstrcpy(&symbolP->name, (pstr_t *)(outP + DEF_name)); /* and the name */
                            outP[symbolP->name.len + 3] = 0; /* add the extra 0 reserved byte */
                            outP += 4 + symbolP->name.len;   /* account for data added */
                        }
                    }
                }
            }
            EndRecord(); /* finish any Open() record */
        }
    }
} /* EmitPUBLICS() */

void EmitEXTNAMES() {
    if (headUnresolved == 0) /* no unresolved */
        return;
    InitRecord(R_EXTNAM); /* init the record */
    unresolved = 0;

    for (symbolP = headUnresolved; symbolP; symbolP = symbolP->nxtSymbol) {
        if (ExtendRec(2 + symbolP->name.len)) /* check room for len, symbol && 0 */
            ;                                 /* no need for special action on extend */
        Pstrcpy(&symbolP->name, outP);        /* copy the len + symbol */
        outP[symbolP->name.len + 1] = 0;      /* add a 0 */
        outP += 2 + symbolP->name.len;        /* advance past inserted data */
        symbolP->offsetOrSym = unresolved++;  /* record the final ext sym id */
    }
    EndRecord(); /* clean closure of record */
} /* EmitEXTNAMES() */

void Emit_ANCESTOR() {
    if (ancestorNameNotSet) /* we have a module name to use */
    {
        InitRecord(R_ANCEST);    /* init the record */
        Pstrcpy(&modName, outP); /* copy name */
        outP += modName.len + 1;
        EndRecord();
        ancestorNameNotSet = 0; /* it is now set */
    }
} /* Emit_ANCESTOR() */

byte SelectOutSeg(byte seg) {
    if (seg == SEG_CODE)
        outRelocOffset = curModule->scode;
    else if (seg == SEG_DATA)
        outRelocOffset = curModule->sdata;
    else
        outRelocOffset = 0; /* only code and data modules are relative to module location */
    return segmap[seg];     /* return seg mapping */
} /* SelectOutSeg() */

void Pass2MODHDR() {
    Pstrcpy(inP, &modName);                /* Read() in the module name */
    ancestorNameNotSet = true;             /* note the ancestor record has not been written */
    for (segId = 0; segId <= 255; segId++) /* init the segment mapping */
        segmap[segId] = (byte)segId;
    GetRecord();
} /* Pass2MODHDR() */

void Pass2COMDEF() {
    while (inP < erecP) { /* while more common definitions */
        if (!Lookup((pstr_t *)(inP + COMNAM_name), &comdefInfoP, F_ALNMASK)) /* check found */
            FatalErr(ERR219);                                                /* Phase() Error() */
        segmap[*inP] = comdefInfoP->linkedSeg; /* record the final linked seg where this goes */
        inP += COMNAM_sizeof + ((pstr_t *)(inP + COMNAM_name))->len; /* past this def */
    }
    GetRecord();
} /* Pass2COMDEF() */

void Pass2EXTNAMES() {
    while (inP < erecP) {                                  /* while more external definitions */
        if (!Lookup((pstr_t *)inP, &symbolP, F_SCOPEMASK)) /* get the name */
            FatalErr(ERR219);                              /* phase Error() - didn't Lookup() !!! */
        AddExtMap(symbolP);
        if (symbolP->flags == F_EXTERN) /* still an extern */
        {                               /* Write() the unresolved reference info */
            WriteAndEcho(&space, 1);
            WriteAndEcho(symbolP->name.str, symbolP->name.len);
            WAEFnAndMod(msgrefin, 17); /* ' - REFERENCED IN ' */
        }
        inP += 2 + *inP; /* 2 for len and extra 0 */
    }
    GetRecord();
} /* Pass2EXTNAMES() */

/* the names below are made global to support nested procedures
 * rather than adding additional arguments to function calls
 */

static word inContentStart, inContentEnd;
static fixup_t *fixupP, *headRelocP;
static reloc_t *relocP;
word outContentRelocOffset;

static void BoundsChk(word addr) {
    if (addr < inContentStart || inContentEnd < addr)
        FatalErr(ERR213); /* fixup bounds Error() */
}

static void GetTypeAndSegHead(fixup_t *afixupP, word typeAndSeg) {
    /* chase down the fixup chain matching seg and fixup type */
    for (fixupP = afixupP->link; fixupP; fixupP = fixupP->link) {
        if (fixupP->typeAndSeg == typeAndSeg) /* found existing list */
            return;
        afixupP = fixupP; /* step along */
    }
    fixupP             = (fixup_t *)GetHigh(sizeof(fixup_t)); /* add to the list */
    fixupP->link       = afixupP->link;
    afixupP->link      = fixupP;
    fixupP->typeAndSeg = typeAndSeg; /* save the typeAndSeg */
    fixupP->relocList  = 0;
} /* GetTypeAndSegHead() */

static void AddRelocFixup(byte seg, word addr) {
    GetTypeAndSegHead((fixup_t *)&headRelocP, seg * 256 + fixType); /* add to reloc list */
    relocP            = (reloc_t *)GetHigh(sizeof(reloc_t));
    relocP->link      = fixupP->relocList;
    fixupP->relocList = relocP;
    relocP->offset    = addr + outContentRelocOffset;
} /* AddRelocFixup() */

void Pass2CONTENT() {
    byte outContentSeg, crc;
    pointer p;
    pointer savedOutP;
    word savedRecLen;
    word bytes2Read;
    pointer markheap = 0; // compiler complains
    fixup_t *headexternP;

    inRecordP = (record_t *)fakeModhdr;        /* keep inRecordP pointing to a modhdr */
    InitRecord(R_MODDAT);                      /* init record */
    outRecordP->reclen = savedRecLen = recLen; /* output length() same as Input() length() */
    savedOutP                        = outP;   /* saved start of record */
    crc                              = High(recLen) + R_MODDAT + Low(recLen); /* initialise crc */
    bufP                             = inP;

    while (recLen > 0) {        /* process all of the record */
        if (savedRecLen > 1025) /* flush current output buf */
            FlushTo();
        if ((bytes2Read = recLen) > npbuf) /* Read() in at most npbuf bytes */
            bytes2Read = npbuf;
        ChkRead(bytes2Read);                       /* Load() them into memory */
        memmove(outP, bufP, bytes2Read);           /* copy to the output buf */
        for (p = outP; p < outP + bytes2Read; p++) /* update the CRC */
            crc += *p;
        bufP += bytes2Read; /* advance the pointers */
        outP += bytes2Read;
        recLen -= bytes2Read;
    }
    if (crc != 0)
        FatalErr(ERR208);   /* Checksum() Error() */
    GetRecord();            /* prime next record */
    if (savedRecLen > 1025) /* we can't fix up a big record */
        return;

    headexternP = headRelocP = 0;                                /* initialise the fixup chains */
    savedOutP                = (outP = savedOutP) + 3;           /* skip seg and offset */
    outContentSeg            = SelectOutSeg(outP[MODDAT_segId]); /* get the mapped link segment */
    outContentRelocOffset    = outRelocOffset;                   /* and the content reloc base */
    inContentEnd =
        (inContentStart = getWord(outP + MODDAT_offset)) + savedRecLen - 5; /* the address range */
    outP[MODDAT_segId] = outContentSeg; /* update the out seg & address values */
    putWord(outP + MODDAT_offset, inContentStart + outContentRelocOffset);
    markheap = GetHigh(0); /* get heap marker */

    /* process the relocate records */
    while (inRecordP->rectyp == R_FIXEXT || inRecordP->rectyp == R_FIXLOC ||
           inRecordP->rectyp == R_FIXSEG) {
        if (inRecordP->rectyp == R_FIXEXT) {
            if ((fixType = *inP) - 1 > 2) /* make sure combine is valid */
                IllFmt();
            inP++;                                       /* past the record byte */
            while (inP < erecP) {                        /* process all of the extref fixups */
                BoundsChk(getWord(inP + EXTREF_offset)); /* check fixup valid */
                if (fixType == FBOTH)
                    BoundsChk(getWord(inP + EXTREF_offset) +
                              1); /* check upper byte also in range */
                symbolP = GetSymbolP(
                    getWord(inP + EXTREF_symId)); /* get symbol entry for the ext symid */
                if (symbolP->flags == F_PUBLIC)   /* is a public so resolved */
                {
                    p = getWord(inP + EXTREF_offset) - inContentStart +
                        savedOutP;     /* Lookup() fixup address in buffer */
                    switch (fixType) { /* relocate to segs current base */
                    case 1:
                        *p = *p + Low(symbolP->offsetOrSym);
                        break;
                    case 2:
                        *p = *p + High(symbolP->offsetOrSym);
                        break;
                    case 3:
                        putWord(p, getWord(p) + symbolP->offsetOrSym);
                        break;
                    }
                    if (symbolP->linkedSeg != SEG_ABS) /* if ! ABS add a fixup entry */
                        AddRelocFixup(symbolP->linkedSeg, getWord(inP + EXTREF_offset));
                } else /* is an extern still */
                {
                    GetTypeAndSegHead((fixup_t *)&headexternP,
                                      fixType); /* add to extern list, seg not needed */
                    relocP = (reloc_t *)GetHigh(
                        sizeof(extfixup_t));               /* allocate the extFixup descriptor */
                    relocP->link      = fixupP->relocList; /* chain it in */
                    fixupP->relocList = relocP;
                    ((extfixup_t *)relocP)->offset =
                        getWord(inP + EXTREF_offset) +
                        outContentRelocOffset;                      /* add in the location */
                    ((extfixup_t *)relocP)->symId = symbolP->symId; /* and the symbol id */
                }
                inP = inP + 4;
            }
        } else /* reloc or interseg */
        {
            segIdx = outContentSeg; /* get default reloc seg */
            outRelocOffset =
                outContentRelocOffset;         /* and reloc base to that of the content record */
            if (inRecordP->rectyp == R_FIXSEG) /* if we are interseg then update the reloc seg */
            {
                segIdx = SelectOutSeg(*inP); /* also updates the outRelocOffset */
                inP    = inP + 1;
            }
            if (segIdx == 0) /* ABS is illegal */
                IllFmt();
            if ((fixType = *inP) - 1 > 2) /* bad fix up type ? */
                IllFmt();
            inP = inP + 1;               /* past fixup */
            while (inP < erecP) {        /* process all of the relocates */
                BoundsChk(getWord(inP)); /* fixup in range */
                if (fixType == FBOTH)    /* && 2nd byte for both byte fixup */
                    BoundsChk(getWord(inP) + 1);
                p = getWord(inP) - inContentStart + savedOutP; /* location of fixup */
                switch (fixType) {                             /* relocate to seg current base */
                case FLOW:
                    *p += Low(outRelocOffset);
                    break;
                case FHIGH:
                    *p += High(outRelocOffset);
                    break;
                case FBOTH:
                    putWord(p, getWord(p) + outRelocOffset);
                    break;
                }
                AddRelocFixup((byte)segIdx, getWord(inP)); /* add a new reloc fixup */
                inP += 2;
            }
        }
        GetRecord(); /* next record */
    }
    outP += savedRecLen - 1;
    EndRecord(); /* finish the content record */
    /* process the extern lists */
    for (fixupP = headexternP; fixupP; fixupP = fixupP->link) {
        InitRecord(R_FIXEXT);              /* create a extref record */
        *outP++ = Low(fixupP->typeAndSeg); /* set the fix type */
        /* process all of the extref of this fixtype */
        for (relocP = fixupP->relocList; relocP; relocP = relocP->link) {
            if (ExtendRec(4)) { /* make sure we have room */
                                /* if (! add the fixtype to the newly created follow on record */
                *outP++ = Low(fixupP->typeAndSeg);
            }
            putWord(outP + EXTREF_symId, ((extfixup_t *)relocP)->symId); /* put the sym number */
            putWord(outP + EXTREF_offset,
                    ((extfixup_t *)relocP)->offset); /* and the fixup location */
            outP += 4;                               /* update to reflect 4 bytes written */
        }
        EndRecord(); /* Close() any Open() record */
    }
    /* now do the relocates */
    for (fixupP = headRelocP; fixupP; fixupP = fixupP->link) {
        InitRecord(R_FIXSEG);                              /* interseg record */
        outP[INTERSEG_segId]   = High(fixupP->typeAndSeg); /* fill in segment */
        outP[INTERSEG_fixType] = Low(fixupP->typeAndSeg);  /* and fixtype */
        outP += 2;
        /* chase down the references */
        for (relocP = fixupP->relocList; relocP; relocP = relocP->link) {
            if (ExtendRec(2)) { /* two bytes || create follow on record */
                                /* fill in follow on record */
                outP[INTERSEG_segId]   = High(fixupP->typeAndSeg);
                outP[INTERSEG_fixType] = Low(fixupP->typeAndSeg);
                outP += 2;
            }
            putWord(outP, ((extfixup_t *)relocP)->offset); /* set the fill the offset */
            outP += 2;
        }
        EndRecord();
    }
    membot = markheap; /* return heap */

} /* Pass2CONTENT() */

void Pass2LINENO() {
    Emit_ANCESTOR(); /* make sure we have a valid ancestor record */
    InitRecord(R_LINNUM);
    *outP++ = SelectOutSeg(*inP++); /* add the seg id info */
    while (inP < erecP) {           /* while more public definitions */
        putWord(outP + LINE_offset,
                outRelocOffset + getWord(inP + LINE_offset));    /* relocate the offset */
        putWord(outP + LINE_linNum, getWord(inP + LINE_linNum)); /* copy the line number */
        outP += 4;
        inP += 4;
    }
    EndRecord();
    GetRecord();
} /* Pass2LINENO() */

void Pass2ANCESTOR() {
    Pstrcpy(outP, &modName);   /* copy the module name over and mark as valid */
    ancestorNameNotSet = true; /* note it isn't written yet */
    GetRecord();
} /* Pass2ANCESTOR() */

void Pass2LOCALS() {
    Emit_ANCESTOR();                /* emit ancestor if (needed */
    InitRecord(R_LOCDEF);           /* init locals record */
    *outP++ = SelectOutSeg(*inP++); /* map the segment and set up relocation base */

    /* note the code below relies on the source file having records <1025 */
    while (inP < erecP) { /* while more local definitions */
        putWord(outP + DEF_offset,
                outRelocOffset + getWord(inP + DEF_offset)); /* Write() offset and symbol */
        Pstrcpy((pstr_t *)(inP + DEF_name), (pstr_t *)(outP + DEF_name));
        ((pstr_t *)(outP + DEF_name))->str[((pstr_t *)(outP + DEF_name))->len] = 0;
        outP += 4 + ((pstr_t *)(inP + DEF_name))->len; /* advance out and in pointers */
        inP += 4 + ((pstr_t *)(inP + DEF_name))->len;
    }
    EndRecord(); /* clean end */
    GetRecord(); /* next record */
} /* Pass2LOCALS() */

/* process pass 2 records */
void Phase2() {
    soutP = outP = GetLow(npbuf); /* reserve the output buffer */
    eoutP        = soutP + npbuf;
    InitExternsMap();
    Open(&tofilefd, toFileName, 2, 0, &statusIO); /* target file */
    FileError(statusIO, toFileName, true);
    EmitMODHDR(); /* process the simple records */
    EmitCOMDEF();
    EmitPUBLICS();
    EmitEXTNAMES();
    /* process all files */
    for (curObjFile = objFileHead; curObjFile; curObjFile = curObjFile->link) {
        if (!curObjFile->publicsMode) { /* publics only file doesn't need more processing*/
            OpenObjFile();                   /* Open() file */
            /* for each module in the file */
            for (curModule = curObjFile->modList; curModule; curModule = curModule->link) {
                GetRecord();                /* Read() modhdr */
                if (curObjFile->hasModules) /* if we have modules Seek() to the current module's
                                               location */
                {
                    Position(curModule->blk, curModule->byt);
                    GetRecord(); /* and Load() its modhdr */
                }
                if (inRecordP->rectyp != R_MODHDR)
                    FatalErr(ERR219); /* phase Error() */
                InitExternsMap();     /* prepare for processing this module's extdef records */
                while (inRecordP->rectyp != R_MODEND) { /* run through the whole module */
                    switch (inRecordP->rectyp) {
                    case 0x00:
                        IllegalRelo();
                        break;
                    case 0x02:
                        Pass2MODHDR();
                        break; /* R_MODHDR */
                    case 0x04:;
                        break; /* R_MODEND */
                    case 0x06:
                        Pass2CONTENT();
                        break; /* R_CONTENT */
                    case 0x08:
                        Pass2LINENO();
                        break;
                    case 0x0A:
                        IllegalRelo();
                        break;
                    case 0x0C:
                        IllegalRelo();
                        break;
                    case 0x0E:
                        FileError(ERR204, inFileName, true);
                        break; /* 0E Premature() eof */
                    case 0x10:
                        Pass2ANCESTOR();
                        break;
                    case 0x12:
                        Pass2LOCALS();
                        break;
                    case 0x14:
                        IllegalRelo();
                        break;
                    case 0x16:
                        GetRecord();
                        break;
                    case 0x18:
                        Pass2EXTNAMES();
                        break;
                    case 0x1A:
                        IllegalRelo();
                        break;
                    case 0x1C:
                        IllegalRelo();
                        break;
                    case 0x1E:
                        IllegalRelo();
                        break;
                    case 0x20:
                        BadRecordSeq();
                        break;
                    case 0x22:
                        BadRecordSeq();
                        break;
                    case 0x24:
                        BadRecordSeq();
                        break;
                    case 0x26:
                        BadRecordSeq();
                        break;
                    case 0x28:
                        BadRecordSeq();
                        break;
                    case 0x2A:
                        BadRecordSeq();
                        break;
                    case 0x2C:
                        BadRecordSeq();
                        break;
                    case 0x2E:
                        Pass2COMDEF();
                        break;
                    }
                }
            }
            CloseObjFile();
        }         /* of else */
    }             /* of do while */
    EmitEnding(); /* Write() final modend and eof record */
    Close(tofilefd, &statusIO);
    FileError(statusIO, toFileName, true);
} /* Phase2() */
