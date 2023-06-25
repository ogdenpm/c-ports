/****************************************************************************
 *  loc7.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "loc.h"

/*
 * ISIS II does not have a STDERR device
 * So if the listing file is not a device
 * echo to stderr
 * in the case where stdout and stderr are left as the console
 * this will do the right thing.
 */
bool echoToStderr;

word ParseLPNumRP() {
    ExpectLP();
    word num = ParseSimpleNumber();
    ExpectRP();
    return num;
} /* ParseLPNumRP */

word ParseSimpleNumber() {
    int num = ParseNumber(GetToken());
    if (num < 0)
        FatalCmdLineErr("Invalid number"); /* invalid syntax */
    return (word)num;
}

byte GetCommonSegId(char *token) {
    char *s = strchr(token + 1, '/');
    if (!s || s[1])
        FatalCmdLineErr("invalid COMMON name");
    *s = '\0'; // remove trailing /
    token++;
    // standard is to use 254 downwards, so reverse order search
    for (int i = SBLANK; i >= SNAMED; i--)
        if (segNames[i] && stricmp(segNames[i], token) == 0)
            return i;
    FatalCmdLineErr("unknown COMMON name");
} /* GetCommonSegId */

/* use a shadow copy of commandLine to create '\0' terminated tokens */
char *GetToken() {
    SkipNonArgChars(cmdP);
    char *token                   = tokenLine + (cmdP - commandLine);
    cmdP                          = Delimit(cmdP);
    tokenLine[cmdP - commandLine] = '\0';
    return token;
}

void ProcArgsInit() {
    char *token;
    char *mark;

    /* display the sign on message skipping the form feed */
    fputs("OMF85 OBJECT LOADER " VERSION "\n", stdout);
    cmdP = commandLine;
    GetToken(); // skip invoke name and leading -
    inName = GetToken();
    mark   = cmdP;
    token  = GetToken();
    if (stricmp(token, "TO") == 0 || stricmp(token, "-o") == 0)
        outName = GetToken();
    else {
        cmdP    = mark; // back up
        char *s = basename(inName);
        char *t = strrchr(s, '.');
        if (!t || t == s)
            FatalCmdLineErr("TO expected");
        outName             = xstrdup(inName);
        outName[t - inName] = '\0'; // remove ext
    }

    if (!(inFp = Fopen(inName, "rb")))
        IoError(inName, "Open error");

    recNum = 0;
    /* check we have a relocation file */
    GetRecord();
    if (inType != R_MODHDR)
        FatalError("%s: has no module header record", inName);
    ProcHdrAndComDef();

    /* process the rest of the command args */
    SkipNonArgChars(cmdP);
    while (*cmdP != '\n') {
        ProcessControls();
        SkipNonArgChars(cmdP);
    }
    FixSegOrder();

    if (!(outFp = Fopen(outName, "wb"))) {
        IoError(outName, "Create error");
        echoToStderr = !isatty(fileno(lstFp));
    }
    /* and the print file (or console) */
    if (lstName) {
        if (!(lstFp = Fopen(lstName, "wt")))
            IoError(lstName, "Create error");
        Printf("OMF85 OBJECT LOADER " VERSION " INVOKED BY:\n");
        printCmdLine(lstFp);
    } else {
        lstName = "stdout";
        lstFp   = stdout;
    }
    printDriveMap(lstFp);
} /* ProcArgsInit */

word AlignAddress(byte align, word size, word laddr) {
    if (size == 0 || align == ABYTE) /* no alignment needed */
        return laddr;

    if (align == AINPAGE) {
        if (size > 256) { /* check if fits in page */
            PrintfAndLog("INPAGE SEGMENT > 256 BYTES COERCED TO PAGE BOUNDRY\n");
        } else if (High(laddr) == High(laddr + size - 1))
            return laddr;
    }
    return (laddr + 0xff) & 0xff00; /* get a whole page */
} /* AlignAddress */

// The original ProcModhdr has been split into two parts
// the first reads the MODHDR and sets up the sizes and align
// for segments
// The second part AssignAddress is called after the command
// line has been processed
// This allows COMMON names to processed ready for the command line
void ProcModhdr() {
    moduleName = pstrdup(ReadName());
    tranId     = ReadByte();
    tranVn     = ReadByte();
    while (inP < inEnd) { /* process all of the seg info */
        byte seg      = ReadByte();
        segSizes[seg] = ReadWord();
        byte align    = ReadByte();
        if (align < AINPAGE || ABYTE < align) /* check valid alignment */
            IllegalRecord();
        if (segFlags[seg]) // has been seen already
            IllegalRecord();
        segFlags[seg] = align;
    }
    GetRecord();
}

void AssignAddress() {
    bool loadHasSize = false;
    for (int i = 0; i < 256; i++) {
        if (segSizes[i]) {
            loadHasSize = true;
            break;
        }
    }

    if (!(segFlags[SSTACK] & AMASK))    /* if STACK not specified put default */
        segFlags[SSTACK] += ABYTE;      /* set to byte align */
    if (!(segFlags[SMEMORY] & AMASK))   /* if MEMORY not specified put default */
        segFlags[SMEMORY] += ABYTE;     /* set to byte align */
    if (!seen.stackSize && loadHasSize) /* it STACK not specified by user and we have data */
        segSizes[SSTACK] += 12;         /* reserve an additional 12 bytes */

    word loadAddress = 0x3680; // default ISIS load address
    bool atTop       = false;  // not at top of memory

    for (int order = 1; order < 255; order++) { /* set addresses, using load order */
        byte seg  = segOrder[order];            // use next seg in order
        word size = segSizes[seg];
        if (!(segFlags[seg] & FHASADDR)) { /* no address specified */
            if (atTop) {                   /* check for going over 64k */
                if (size > 0)
                    FatalError("%s: Program exceeds 64k", outName);
                segFlags[seg] |= FWRAP0;
            }
        } else { /* user specified address */
            if (size > 0)
                atTop = false;
            loadAddress = segBases[seg]; /* set the loadAddress to user specified */
        }
        /* update this segments base address allowing for alignment */
        segBases[seg] = AlignAddress(segFlags[seg] & AMASK, size, loadAddress);
        if (segBases[seg] == 0 && loadAddress)
            FatalError("%s: Program exceeds 64k", outName);
        if (seg == SMEMORY && size == 0 && loadHasSize) // size memory segment
            if (!atTop && MEMCK > segBases[SMEMORY])
                segSizes[SMEMORY] = size = MEMCK - segBases[SMEMORY];
        /* advance loadAddress and check for memory wrap around */
        if ((loadAddress = segBases[seg] + size) < segBases[seg]) {
            if (loadAddress) /* beyond 64k boundary */
                FatalError("%s: Program exceeds 64k", outName);
            else
                atTop = true;
        }
    }
    segBases[SSTACK] += segSizes[SSTACK]; /* stack goes down so update to top */
} /* ProcModhdr */

void ProcComdef() {
    segNames[SBLANK] = ""; // default  blank common
    while (inType == R_COMDEF) {
        while (inP < inEnd) {
            byte seg = ReadByte();
            if (seg < SNAMED || seg == SBLANK)
                IllegalRecord();
            /* check if (combine value not appropriate for common or if this one already seen */
            if ((segFlags[seg] & AMASK) == AUNKNOWN || (segFlags[seg] & FSEGSEEN) != 0)
                IllegalRecord();
            segFlags[seg] |= FSEGSEEN; /* flag as seen */
            segNames[seg] = pstrdup(ReadName())->str;
        }
        GetRecord();
    }
} /* ProcComdef */

void ProcHdrAndComDef() {
    ProcModhdr();
    ProcComdef();
    // check entries, if used should have FSEGSEEN and align type
    for (int seg = SNAMED; seg < SBLANK; seg++) {
        if (segFlags[seg] && segFlags[seg] < (FSEGSEEN + FLOW))
            BadRecordSeq();
    }
}
