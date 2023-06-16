/****************************************************************************
 *  link3.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"
#include <stdlib.h>

char *segName[] = { "ABSOLUTE", "CODE    ", "DATA    ", "STACK   ", "MEMORY  " };

bool segUsed[6];
byte segToUse   = SEG_BLANK - 1;
bool commonSeen = false;

comseginfo_t comSegInfo[256];
segFrag_t *segFrags[6];
segFrag_t *segFragP;
segFrag_t *curSegFragP, *prevSegFragP;
symbol_t *tailSegOrderLink;
symbol_t *nxtSymbolP;
module_t *hmoduleP;
bool publicsOnly;
byte moreRecords;
byte haveModuleHdr;
byte inSegCombine;
byte curSeg;
word inSegLen;
word newExtId;
word newUnresolved;
word externCnt;
word inSegOffset;

// simple code to covert pascal to C string
// uses a shared string area so subsequent
// calls will overwrite

char *p2cstr(pstr_t *p) {
    static char str[32]; // module name can be up to 31 chars
    if (p->len > 31)
        FatalError("%.*s: name > 31 chars", p->len, p->str);
    memcpy(str, p->str, p->len);
    str[p->len] = '\0';
    return str;
}


pstr_t *ReadName() {
    pstr_t *pname = (pstr_t *)inP;
    if (inP >= inEnd || (inP += pname->len + 1) > inEnd)
        IllFmt();
    return pname; /* read a name */
} /* ReadName() */

/* read in 4 byte block num and byte num inP points to the data, return a uint32_t offset */
uint32_t ReadLocation() {
    uint32_t offset = ReadWord() * 128;
    return offset + ReadWord();
}
uint16_t ReadWord() {
    if ((inP += 2) > inEnd)
        IllFmt();
    return getWord(inP - 2);
}

uint8_t ReadByte() {
    if (inP >= inEnd)
        IllFmt();
    return *inP++;
}

/* get next record, test type, and error if not as expected */
void ExpectRecord(byte type) {
    GetRecord();
    recNum = 0;
    if (type != inType)
        BadRecordSeq();
} /* ExpectType() */

/* print segment base, Size() and alignment */
void PrintBaseSizeAlign(word baddr, word bsize, byte align) {
    if ((++align) <= 1) /* ANONE || AUNKNOWN */
        Printf("\n%04XH  %04XH  ", baddr, baddr + bsize - 1);
    else /* replace start / stop with spaces */
        Printf("\n%14s", "");
    Printf("%4XH  %c  ", bsize, " AIPB"[align]);
}

/* wrtie module statistics */
void WriteStats() {
    //	byte i;

    if (!mapWanted) /* user doesn't want */
        return;
    Printf("\nLINK MAP OF MODULE %s\nWRITTEN TO FILE %s", p2cstr((pstr_t *)&moduleName),
           outName);
    Printf("\nMODULE IS %s MAIN MODULE\n\n", moduleType == MT_NOTMAIN ? "NOT A" : "A");

    Printf("SEGMENT INFORMATION:\n"
           "START   STOP LENGTH REL NAME\n");

    for (byte seg = SEG_CODE; seg <= SEG_MEMORY; seg++) { /* regular segments */
        if (segLen[seg] > 0) {                            /* segment is used */
            PrintBaseSizeAlign(0, segLen[seg], alignType[seg]);
            Printf(segName[seg]);
            if (alignType[seg] == AINPAGE && segLen[seg] > 256)
                Printf("  *INPAGE Segment > 256 bytes*");
            /* print the details for any gap fragments */
            for (segFrag_t *frag = segFrags[seg]; frag; frag = frag->next) {
                PrintBaseSizeAlign(frag->bot, frag->top - frag->bot + 1, ANONE);
                Printf("%s  *GAP*", segName[seg]);
            }
        }
    }
    /* common block segments */
    for (symbol_t *commSym = headCommSym; commSym; commSym = commSym->nxtSymbol->hashChain) {
        PrintBaseSizeAlign(0, commSym->len, commSym->flags); /* print Size() info */
        Printf("/%s/", p2cstr(&commSym->name));

        if (commSym->flags == AINPAGE && commSym->len > 256) /* warn of problems */
            Printf("  *INPAGE Segment > 256 bytes*");

    }
    if (segLen[0] > 0) { /* handle blank common */
        PrintBaseSizeAlign(0, segLen[0], alignType[0]);
        Printf("//");
        if (alignType[0] == 1 && segLen[0] > 256)
            Printf("  *INPAGE Segment > 256 Bytes*");
    }
    word hiAddr = 0; /* high water mark for over lap detection - start at address 0 */
    /* frags for SEG_ABS are real data blocks go through them */
    for (segFrag_t *frag = segFrags[0]; frag; frag = frag->next) {
        /* print the sizes and segment */
        PrintBaseSizeAlign(frag->bot, frag->top - frag->bot + 1, 0);
        Printf(segName[SEG_ABS]);
        if (frag->bot < hiAddr) /* check for overlap */
            Printf("  *OVERLAP*");

        if (frag->top >= hiAddr) /* update the high water mark for next block */
            hiAddr = frag->top + 1;
    }

    Printf("\n\nINPUT MODULES INCLUDED:\n");
    /* go over each file and module processed */
    for (objFile = objFileList; objFile; objFile = objFile->next) {
        for (module = objFile->modules; module; module = module->next) {
            /* ignore if publics only && nothing loaded */
            if (!objFile->publics || module->symbols)
                Printf(" %s(%s)%s\n", objFile->name, p2cstr(&module->name),
                       objFile->publics ? " (PUBLICS)" : "");
        }
    }
} /* WriteStats() */

void ChainUnresolved() { /* creates a chain of unresolved externals */
    symbol_t *p;
    word toChain;

    if (unresolved == 0) /* nothing to write */
        return;
    PrintAndEcho("\nUnresolved External Names:\n");

    toChain = unresolved;
    for (byte i = 0; i <= 127; i++) { /* traverse all of the HashF() chains */
        for (symbol_t *symbol = hashTab[i]; symbol; symbol = symbol->hashChain) {
            /* traverse the single HashF() chain */
            if (symbol->flags == F_EXTERN) { /* we have an extern */
                /* Lookup() where to insert this symbol on the list */
                nxtSymbolP = (symbol_t *)&unresolvedList;
                for (p = unresolvedList; p; p = p->nxtSymbol) { /* scan whole list if (necessary */
                    if (p->offsetOrSym > symbol->offsetOrSym)
                        break; /* passed insert point using sym num */
                    nxtSymbolP = (symbol_t *)&p->nxtSymbol;
                }
                symbol->nxtSymbol    = nxtSymbolP->hashChain; /* add to the unresolved chain */
                nxtSymbolP->hashChain = symbol;
                if (--toChain == 0) /* done */
                    return;
            }
        }
    }
} /* ChainUnresolved() */

/* inserts a block of data into segment list at proper address */
void CreateFragment(byte seg, word bot, word top) {
    if (!mapWanted) /* we are not creating a map so ignore */
        return;
    segFragP      = xmalloc(sizeof(segFrag_t)); /* allocate and initialise the fragment */
    segFragP->bot = bot;
    segFragP->top = top;
    prevSegFragP  = (segFrag_t *)&segFrags[seg]; /* separate lists for each segment */
    curSegFragP   = segFrags[seg];
    /* look for insert point or reach the end */
    while (curSegFragP && curSegFragP->bot > bot) {
        prevSegFragP = curSegFragP; /* next */
        curSegFragP  = curSegFragP->next;
    }
    segFragP->next     = prevSegFragP->next; /* insert this fragment */
    prevSegFragP->next = segFragP;
} /* CreateFragment() */

void P1CommonSegments() {
    if (curSeg == SEG_BLANK) { /* blank common */
        if (segUsed[0])        /* record seen in slot 0 */
            IllFmt();
        segUsed[0] = true;
        if (inSegLen > segLen[0]) /* record max Size() */
            segLen[0] = inSegLen;
        if (alignType[0] == AUNKNOWN)
            alignType[0] = inSegCombine;
        else if (alignType[0] != ABYTE || inSegCombine != ABYTE)
            alignType[0] = APAGE; /* if (! both byte align make page align */
    } else {
        if (!commonSeen) {
            memset(comSegInfo, AUNKNOWN, sizeof(comSegInfo));
            commonSeen = true;
        }
        if (comSegInfo[curSeg].combine != AUNKNOWN) /* duplicate */
            IllFmt();
        comSegInfo[curSeg].combine        = inSegCombine; /* save combine and Size() */
        comSegInfo[curSeg].lenOrLinkedSeg = inSegLen;
    }
} /* P1CommonSegments() */

void P1StdSegments() {
    word prevLen, segLoadBase;

    if (segUsed[curSeg]) /* duplicate seg Size() info */
        IllFmt();
    segUsed[curSeg] = true;
    if (curSeg == SEG_ABS || curSeg > SEG_MEMORY)
        IllFmt();
    if (inSegLen == 0) /* nothing to do */
        return;
    if (curSeg == SEG_CODE || curSeg == SEG_DATA) {
        if (alignType[curSeg] == AUNKNOWN) { /* first seg */
            alignType[curSeg] = inSegCombine;
            segLen[curSeg]    = inSegLen;
            segLoadBase       = 0;
        } else {
            prevLen = segLen[curSeg];
            switch (inSegCombine) {
            case 1:                                   /* IN PAGE */
                if (Low(prevLen) + inSegLen <= 0x100) /* if fits in current page */
                    segLoadBase = prevLen + inSegLen; /* calculate the base */
                else                                  /* else start new page */
                    segLoadBase = ((prevLen + 0xFF) & 0xFF00) + inSegLen;

                if (alignType[curSeg] != AINPAGE ||
                    segLoadBase > 0x100) /* check if we should use page align */
                    alignType[curSeg] = APAGE;
                break;
            case 2:                        /* PAGE */
                alignType[curSeg] = APAGE; /* calculate new page */
                segLoadBase       = ((prevLen + 0xFF) & 0xFF00) + inSegLen;
                break;
            case 3: /* byte */
                /* if were in page we now have to assume page align */
                if (alignType[curSeg] == AINPAGE)
                    alignType[curSeg] = APAGE;
                segLoadBase = prevLen + inSegLen; /* Lookup() out Load() address */
                break;
            default:
                IllFmt();
            }
            segLen[curSeg] = segLoadBase; /* update the overall seg len */
            if ((segLoadBase = segLoadBase - inSegLen) >
                prevLen) /* backup to start of this Load() address */
                CreateFragment(curSeg, prevLen,
                               segLoadBase - 1); /* not contiguous so create fragment */
            if (segLen[curSeg] < segLoadBase)    /* oops we went over 64k */
                RecError("Segment too large");   /* segment too large */
        }
        if (curSeg == SEG_CODE) /* update the code / data base address */
            module->cbias = segLoadBase;
        else
            module->dbias = segLoadBase;
    } else {                               /* SEG_STACK or SEG_MEMORY or SEG_RESERVE */
        if (alignType[curSeg] == AUNKNOWN) /* set initial combine */
            alignType[curSeg] = inSegCombine;
        else if (alignType[curSeg] != ABYTE || inSegCombine != ABYTE) /* APAGE if ! both ABYTE */
            alignType[curSeg] = APAGE;
        segLen[curSeg] += inSegLen; /* length() is additive */
    }
} /* P1StdSegments() */

/*
    select requested seg, setting inSegOffset to point to base
    returns returns seg - note for common len is replaced by final linked seg
    after the comdef records have been processed
*/

byte SelectInSeg(byte seg) {
    inSegOffset = 0;
    if (seg == SEG_CODE)
        inSegOffset = module->cbias;
    else if (seg == SEG_DATA)
        inSegOffset = module->dbias;
    else if (seg >= SEG_NAMCOM && seg != SEG_BLANK) {
        if (!commonSeen) /* selecting common when none exists !! */
            RecError("Content for undeclared COMMON");
        if (comSegInfo[seg].combine == ANONE) /* named common has been seen so ok */
            return (byte)comSegInfo[seg].lenOrLinkedSeg;
        if (comSegInfo[seg].combine != AUNKNOWN)
            BadRecordSeq();
        IllFmt();
    }
    return seg;
} /* SelectInSeg() */

void P1ModHdr() {
    static bool haveIdVn = false;

    if (haveModuleHdr) /* catch multiple header errors */
        BadRecordSeq();
    haveModuleHdr = true;
    nxtSymbolP    = (symbol_t *)&module->symbols;
    if (publicsOnly) /* ! loading data */
        return;
    ReadName();
    if (!haveIdVn) { /* copy x1x2 data */
        haveIdVn = true;
        tranId   = ReadByte(); // tranId
        tranVn   = ReadByte(); // tranVn
    } else {
        if (ReadByte() != tranId) /* propgate none 0 only if same */
            tranId = 0;
        if (ReadByte() != tranVn)
            tranVn = 0;
    }
    module->cbias = segLen[SEG_CODE]; /* code and data offsets of this module */
    module->dbias = segLen[SEG_DATA];
    commonSeen    = false;
    memset(segUsed, false, sizeof(segUsed));

    while (inP < inEnd) { /* while more segments */
        curSeg       = ReadByte();
        inSegLen     = ReadWord();
        inSegCombine = ReadByte();

        if (inSegCombine - 1 > 2) /* only AINPAGE - ABYTE valid */
            IllFmt();
        if (curSeg >= SEG_NAMCOM)
            P1CommonSegments();
        else
            P1StdSegments();
    }
} /* P1ModHdr() */

void P1ModEnd() {
    moreRecords = 0;
    if (publicsOnly)
        return;
    if (ReadByte() == MT_MAIN) {
        if (moduleType != MT_NOTMAIN) /* duplicate main modules !! */
            ModuleWarning("More than 1 main module, conflict in ");
        else {
            moduleType = MT_MAIN;                 /* record main and save entry point */
            entrySeg  = SelectInSeg(ReadByte()); // seg
            entryOffset = inSegOffset + ReadWord();
        }
    }
    if (commonSeen) /* check common alignments */
        for (int i = 0; i < 256; i++) {
            if (comSegInfo[i].combine + 1 > 1) /* only AUNKNOWN & ANONE valid */
                BadRecordSeq();
        }
} /* P1ModEnd() */

void Pass1CONTENT() {

    if (!publicsOnly) {                         /* skip if just processing publics */
        if ((curSeg = ReadByte()) == SEG_ABS) { /* absolute record */
            uint16_t offset = ReadWord();
            if (recLen > 4)
                CreateFragment(SEG_ABS, offset, offset + recLen - 5);
        } else {               /* relocatable record */
            if (recLen > 1025) /* only abs > 1025 */
                RecError("Non Absolute content record length > 1025");
            if (curSeg == SEG_STACK)
                RecError("Illegal stack content record");
            if ((curSeg < SEG_NAMCOM && !segUsed[curSeg]) || (curSeg == SEG_BLANK && !segUsed[0]))
                RecError("Content defined for undeclared segment");
            else
                curSeg = SelectInSeg(curSeg);
        }
    }
} /* Pass1CONTENT() */

void Pass1COMDEF() {
    if (publicsOnly) /* ! needed */
        return;
    if (!commonSeen) /* can't have common def if no common segments */
        BadRecordSeq();
    while (inP < inEnd) {
        curSeg       = ReadByte();
        pstr_t *name = ReadName();
        if (curSeg < SEG_NAMCOM || curSeg == SEG_BLANK) /* ! a named common */
            IllFmt();
        if (comSegInfo[curSeg].combine + 1 < 2) /* AUNKNOWN && ANONE invalid */
            IllFmt();
        symbol_t *commSym;
        if (Lookup(name, &commSym, F_ALNMASK)) /* already exist ? */
        {
            /* if (! both ABYTE) make APAGE */
            if (commSym->flags != ABYTE || comSegInfo[curSeg].combine != ABYTE)
                commSym->flags = APAGE;
            if (commSym->len != comSegInfo[curSeg].lenOrLinkedSeg) /* if not same size */
            {
                if (comSegInfo[curSeg].lenOrLinkedSeg > commSym->len) /* set as max of sizes */
                    commSym->len = comSegInfo[curSeg].lenOrLinkedSeg;
                PrintAndEcho("/%s/", &commSym->name);
                ModuleWarning(" -Unequal COMMON length, conflict in ");
            }
        } else { /* new entry required */
            commSym->hashChain = xmalloc(sizeof(symbol_t) + name->len);
            commSym            = commSym->hashChain; /* link in and mark new end of chain */
            commSym->hashChain = 0;
            commSym->flags     = comSegInfo[curSeg].combine; /* save the combine value */
            /* check we haven't created too many segs in the linked file */
            if (segToUse < SEG_NAMCOM)
                RecError("Too many COMMON segments");
            commSym->seg = segToUse--;     /* record the linked seg for this segment */
            Pstrcpy(name, &commSym->name); /* copy the name */
            commSym->len                  = comSegInfo[curSeg].lenOrLinkedSeg; /* and Size() */
            commSym->nxtSymbol->hashChain = tailSegOrderLink->hashChain; /* chain into seg order */
            tailSegOrderLink->hashChain   = commSym;
            tailSegOrderLink              = commSym->nxtSymbol;
        }
        comSegInfo[curSeg].combine = ANONE; /* flag as done */
        /* replace len with the linkedSeg */
        comSegInfo[curSeg].lenOrLinkedSeg = commSym->seg;
    }
}

void MarkPublic(symbol_t *symbol, uint16_t offset) {
    symbol->flags        = F_PUBLIC; /* now public */
    symbol->seg          = curSeg;   /* location known */
    symbol->offsetOrSym  = inSegOffset + offset;
    segmap[curSeg]        = 0xFF;                  /* flag as used seg */
    symbol->nxtSymbol    = nxtSymbolP->hashChain; /* add to the publics chain */
    nxtSymbolP->hashChain = symbol;
    nxtSymbolP            = (symbol_t *)&symbol->nxtSymbol;
} /* MarkPublic() */

void Pass1PUBNAMES() {
    if ((curSeg = ReadByte()) != SEG_ABS &&
        publicsOnly) /* publics only requires absolute targets */
        return;
    curSeg = SelectInSeg(curSeg); /* get the linked seg */

    while (inP < inEnd) { /* while more public definitions */
        /* Lookup() extern || public */
        uint16_t offset = ReadWord();
        pstr_t *name    = ReadName();

        symbol_t *symbol;
        if (Lookup(name, &symbol, F_SCOPEMASK)) {
            if (symbol->flags == F_PUBLIC) { /* error if ! same location */
                if (curSeg != 0 || symbol->seg != curSeg || symbol->offsetOrSym != offset) {
                    PrintAndEcho(p2cstr(&symbol->name));
                    ModuleWarning(" - Multiply defined, duplicate in ");
                }
            } else { /* was extern but we now have an address */
                unresolved--;
                MarkPublic(symbol, offset);
            }
        } else { /* create a new entry */
            symbol->hashChain = xmalloc(sizeof(symbol_t) + name->len);
            symbol            = symbol->hashChain; /* add to HashF() chain */
            symbol->hashChain = 0;
            Pstrcpy(name, &symbol->name); /* add the name */
            MarkPublic(symbol, offset);            /* make it a public */
        }
        ReadByte(); // end 0
    }
} /* Pass1PUBNAMES() */

void Pass1EXTNAMES() {
    if (publicsOnly) /* skip if ! wanted */
        return;
    while (inP < inEnd) { /* while more external definitions */
        pstr_t *name = ReadName();
        externCnt++;                                /* bump the number of externs */
        symbol_t *symbol;
        if (!Lookup(name, &symbol, F_SCOPEMASK)) { /* ! currently extern || public */
            symbol->hashChain   = xmalloc(sizeof(symbol_t) + name->len); /* create an entry */
            symbol              = symbol->hashChain;                    /* link in */
            symbol->hashChain   = 0;
            symbol->flags       = F_EXTERN; /* extern and record symcnt number */
            symbol->offsetOrSym = ++newExtId;
            Pstrcpy(name, &symbol->name); /* copy name */
            unresolved++;                  /* one more to resolve */
            newUnresolved++;
        }
        ReadByte(); // 0
    }
} /* Pass1EXTNAMES() */

/* process pass 1 records */
void P1Records(byte newModule) {
    if (newModule) /* create entry for new module */
    {

        hmoduleP->next  = (module = xmalloc(sizeof(module_t) + inP[0]));
        module->next    = 0;
        module->symbols = 0;
        Pstrcpy(inP, &module->name);
        hmoduleP = module;
    }
    externCnt     = 0;
    recNum        = 1;
    haveModuleHdr = 0;
    moreRecords   = true;
    while (moreRecords) {
        switch (inType) {
        case R_MODHDR:
            P1ModHdr();
            break;
        case R_MODEND:
            P1ModEnd();
            break;
        case R_MODDAT:
            Pass1CONTENT();
            break;
        case R_MODEOF:
            FatalError("%s: Unexpected MODEOF record", objName);
            break;
        case R_PUBDEF:
            Pass1PUBNAMES();
            break;
        case R_EXTNAM:
            Pass1EXTNAMES();
            break;
        case R_LINNUM:
        case R_ANCEST:
        case R_LOCDEF:
        case R_FIXEXT:
        case R_FIXLOC:
        case R_FIXSEG:
            break; // ignored on pass 1
        case R_LIBLOC:
        case R_LIBNAM:
        case R_LIBDIC:
        case R_LIBHDR:
            BadRecordSeq();
            break;
        case R_COMDEF:
            Pass1COMDEF();
            break;
        default:
            IllegalRelo();
            break;
        }
        GetRecord();
    }
    if (externCnt > maxExternCnt) /* get max of externs */
        maxExternCnt = externCnt;
    /* make sure next record is valid or eof */
    if (inType != R_MODEOF && inType != R_MODHDR && inType != R_LIBNAM)
        RecError("EOF record missing");
} /* P1Records() */

typedef struct {
    uint32_t location;
    pointer dictionary;
    bool required;
} lib_t;

void P1LibScan() {
    objFile->isLib = true;
    word count     = ReadWord();
    if (unresolved > 0) {
        hmoduleP          = (module_t *)&objFile->modules;
        uint32_t location = ReadLocation();
        Position(location);   /* Seek() to library module names record */
        ExpectRecord(R_LIBNAM); /* get rec and validate type */
        ExpectRecord(R_LIBLOC); /* should have the locations */
        // load all of the locations to memory to avoid repeated reloads
        if (count * 4 > recLen - 1)
            IllFmt();
        lib_t *libData = xmalloc(sizeof(lib_t) * count);
        for (int i = 0; i < count; i++)
            libData[i].location = ReadLocation();
        ExpectRecord(R_LIBDIC); // Dictionary is now in memory
        char *dictionary = xmalloc(recLen + 1);
        memcpy(dictionary, inP, recLen);
        dictionary[recLen] = '\0'; // extra byte so strchr works on bad files
        char *p            = dictionary;
        for (int i = 0; i < count; i++) {
            libData[i].dictionary = (pointer)p;
            p                     = strchr(p, '\0') + 1;
            if (p >= dictionary + recLen)
                IllFmt();
        }

        newUnresolved = unresolved; /* first pass attempts to resolve all unresolved */
        /* loop incase scan adds more externs that the lib can resolve */
        while (newUnresolved > 0) {
            word toResolve = newUnresolved; /* 1st pass scans for all unresolved */

            for (int i = 0; i < count; i++) // remove required marker
                libData[i].required = false;
            /* scan the dictionary across all names or until no unresolved */
            for (int i = 0; i < count && toResolve; i++) {
                for (pointer s = libData[i].dictionary; *s; s += s[0] + 1) {
                    /* matched an unresolved external */
                    symbol_t *symbol;
                    if (Lookup((pstr_t *)s, &symbol, F_EXTERN)) {
                        libData[i].required = true;
                        toResolve--;
                    }
                }
            }
            if (toResolve == newUnresolved) // no modules found
                break;
            newUnresolved = 0;
            for (int i = 0; i < count; i++) { /* process each module needed */
                if (libData[i].required) {
                    libData[i].dictionary[0] = '\0'; // no need scan again as they will be defined
                    Position(libData[i].location);
                    ExpectRecord(R_MODHDR);
                    P1Records(true); /* creates a new module entry */
                    module->location = libData[i].location;
                }
            }

        } /* do while */
        free(dictionary);
        free(libData);
    }

} /* P1LibScan() */

void P1LibUserModules() {
    word modIdx, i;
    bool unmatched;
    pstr_t *name;

    ReadWord(); // count

    Position(ReadLocation()); /* Seek to the libnam section - libhdr is current rec */
    ExpectRecord(R_LIBNAM);
    modIdx    = 0;
    unmatched = true;
    while ((name = ReadName()) && unmatched) { /* scan the library looking for specified modules */
        modIdx++;                              /* index for later into libloc data */
        unmatched = false;                     /* set flag for Exit() if (module ! matched */
                                               /* go over the supplied list of modules */
        for (module = objFile->modules; module; module = module->next) {
            if (module->cbias == 0) { /* ! matched yet */
                if (PStrequ(name, &module->name))
                    module->cbias = modIdx; /* record matched module */
                else
                    unmatched = true; /* at least one module not matched */
            }
        }
    }
    ExpectRecord(R_LIBLOC); /* expect the libloc record */

    for (i = 1; i <= modIdx; i++) { /* for all locations */
        uint32_t location = ReadLocation();
        /* scan the module list to see if (we need this one */
        for (module = objFile->modules; module; module = module->next) {
            if (module->cbias == i)
                module->location = location;
        }
    }
    hmoduleP = (module_t *)&objFile->modules; /* pointer to remove missing modules from the chain */
    for (module = objFile->modules; module; module = module->next) {
        if (module->cbias == 0) {
            ModuleWarning("Module not in library, looking for ");
            hmoduleP->next = module->next; /* remove this module from the list */
        } else {
            Position(module->location); /* process the module */
            ExpectRecord(R_MODHDR);
            P1Records(false);  /* module already known so don't create entry for it */
            hmoduleP = module; /* this module remains on the list */
        }
    }
} /* P1LibUserModules() */

void Phase1() {
    for (byte i = SEG_ABS; i <= SEG_RESERVE; i++)
        segFrags[i] = 0;
    newExtId           = 0;                        /* no symbols */
    tailSegOrderLink = (symbol_t *)&headCommSym; /* no common segments */
    /* process each item in the Input() list */
    for (objFile = objFileList; objFile; objFile = objFile->next) {
        OpenObjFile(); /* open the file */
        publicsOnly = objFile->publics;
        GetRecord();              /* Load() the first record */
        if (inType == R_LIBHDR) /* library? */
        {
            if (objFile->isLib) /* user specified modules */
                P1LibUserModules();
            else
                P1LibScan();               /* library scan */
        } else if (inType == R_MODHDR) { /* simple object file */
            if (objFile->isLib)            /* oops user thought it was a library */
                RecError("Not a library"); /* not a library */
            hmoduleP = (module_t *)&objFile->modules;
            while (inType == R_MODHDR) /* process each module in file */
                P1Records(true);         /* this is a new module */

            if (inType != R_MODEOF)
                RecError("EOF record missing"); /* no eof */
        } else
            RecError("Module header record missing");
        CloseObjFile();
    }
    printDriveMap();
    WriteStats();
    ChainUnresolved();
} /* Phase1() */
