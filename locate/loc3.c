/****************************************************************************
 *  loc3.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

/*
 * vim:ts=4:shiftwidth=4:expandtab:
 */
#include "loc.h"

void ProcLinNum(void) {
    if (!seen.purge || seen.lines) { // check if needed
        SetTargetSeg(ReadByte());    /* get ABS seg and set working base address */
        if (!seen.purge) {           /* need to create output record */
            InitRecord(R_LINNUM);
            WriteByte(SABS);
        }
        while (inP < inEnd) { /* loop over all entries */
            uint16_t offset = ReadWord() + targetBase;
            uint16_t lineno = ReadWord();
            if (!seen.purge) { /* emit to output record */
                WriteWord(offset);
                WriteWord(lineno);
            }
            if (seen.lines) /* if required list the mapping */
                PrintColumn(CTLIN, offset, lineno);
        }
        if (!seen.purge) /* finish off output record */
            EndRecord();
    }
    GetRecord(); /* prep next record */
}

void ProcAncest(void) {

    if (!seen.purge)
        CopyRecord();
    if (seen.lines || seen.symbols) { // check if needed
        pstr_t const *name = ReadName();
        PrintColumn(CTMOD, name->len, name->str); /* print the ancestor */
        ForceSOL();
    }

    GetRecord(); /* prep next record */
}

void ProcDefs(uint8_t list, uint8_t ctype) {
    uint8_t outSeg = SABS;
    if (!seen.purge || list) { /* check if we need to process */
        uint8_t seg = ReadByte();
        if (seg == SSTACK) {
            outSeg         = SSTACK;
            targetBase = 0; /* stack base is 0 */
        } else
            SetTargetSeg(seg);
        if (!seen.purge) {
            InitRecord(inType); /* initialise the output record */
            WriteByte(outSeg);
        }
        while (inP < inEnd) {
            uint16_t offset  = ReadWord() + targetBase;
            pstr_t const *name = ReadName();
            ReadByte();
            if (!seen.purge) {
                WriteWord(offset);
                WriteName(name);
                WriteByte(0);
            }
            if (list)
                PrintColumn(ctype, offset, name->len, name->str);
        }
        if (!seen.purge) /* if not purged finish off the output record */
            EndRecord();
    }
    GetRecord(); /* prep for next record */
}

void ProcExtnam(void) {
    // copy over the record to the output
    CopyRecord();
    warnings |= warningMask & 1;
    while (inP < inEnd) { /* process each item in the input EXTNAM record */
        ForceSOL();
        pstr_t const *name = ReadName();
        ReadByte();
        PrintfAndLog("UNSATISFIED EXTERNAL(%d) %.*s\n", unsatisfiedCnt++, name->len, name->str);
    }
    GetRecord(); /* prep the next record */
}

void PrintListingHeader(char const *heading) {
    Printf("\n%s %s\nREAD FROM FILE %s\nWRITTEN TO FILE %s\n", heading, moduleName->str, omfInName,
           omfOutName);
}

/* main procedure to locate the file */
void LocateFile(void) {
    AssignAddress();      // allocate segment addresses
    InitRecord(R_MODHDR); /* create header record */
    WriteName(moduleName);
    WriteByte(tranId);
    WriteByte(tranVn);
    EndRecord(); /* located modhdrs don't have seg size info */
    /* print symbol table header message if (required */
    if (seen.publics || seen.symbols || seen.lines) {
        PrintListingHeader("SYMBOL TABLE OF MODULE ");
        fputc('\n', lstFp);
        /* print the column headings */
        for (int i = 0; i < columns; i++)
            PrintColumn(CTHEAD);
        fputc('\n', lstFp);
    }
    /* process the link file */
    while (inType != R_MODEND) {
        switch (inType) {
        case R_MODHDR: // already processed
            BadRecordSeq();
            break;
        case R_MODEND:
            break; // shouldn't get here
        case R_MODDAT:
            ProcModdat();
            break;
        case R_LINNUM:
            ProcLinNum();
            break;
        case R_MODEOF: // premature EOF
            RecError("Unexpected EOF record");
            break;
        case R_ANCEST:
            ProcAncest();
            break;
        case R_LOCDEF:
            ProcDefs(seen.symbols, CTSYM);
            break;
        case R_PUBDEF:
            ProcDefs(seen.publics, CTPUB);
            break; /* 16 - pubdef */
        case R_EXTNAM:
            ProcExtnam();
            break;
        case R_FIXEXT: // should have been handled in ProcModdat
        case R_FIXLOC:
        case R_FIXSEG:
        case R_LIBLOC: // embedded library !!
        case R_LIBNAM:
        case R_LIBDIC:
        case R_LIBHDR:
        case R_COMDEF:
            BadRecordSeq(); // already handled
            break;
        default:
            IllegalReloc();
            break;
        }

    }
    ProcModend(); /* Read the modend and generate the rest of the located file */
    ForceSOL();       /* make sure at start of line if (symbols listed */
    PrintMemoryMap(); /* print the final memory map + any overlay Errors() */
    fclose(omfInFp);
}
