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

void FixupBoundsChk(word addr)
{
    if (addr < inFragment.saddr || inFragment.eaddr < addr)
        FatalErr(ERR213);   /* Fixup Bounds Error */
} /* FixupBoundsChk */


void ProcLinNum()
{
    if (! seen.purge || seen.lines)  /* only process if ! purge || lines requested */
    {
        outSegType =  SetWorkingSeg(*inP /* inSeg */); /* get ABS seg and set working base address */
        if (! seen.purge)     /* need to create output record */
        {
            InitRecOut(R_LINNUM);
            *outP++ /* outSeg */ = outSegType;
        }
        inP++;      /* past segid */
        while (inP < erecP) {       /* loop over all entries */
            if (! seen.purge) /* emit to output record */
            {
                putWord(outP + LINE_offset, workingSegBase + getWord(inP + LINE_offset));    /* final location */
                putWord(outP + LINE_linNum, getWord(inP + LINE_linNum)); /* copy the line number */
                outP += LINE_sizeof;    /* past this entry */
                
            }
            if (seen.lines)   /* if required list the mapping */
            {
                BinAsc(workingSegBase + getWord(inP + LINE_offset), 16, '0', alin, 4);
                BinAsc(getWord(inP + LINE_linNum), 10, ' ', PSStr(x5), 5);
                PrintColumn(alin, (pstr_t *)x5);
            }
            inP += LINE_sizeof;  /* to next entry */
        }
        if (! seen.purge)     /* finish off output record */
            EndRecord();
    }
    GetRecord();    /* prep next record */
} /* ProcLinNum */


void ProcAncest()
{
    if (seen.lines || seen.symbols)  /* if lines || symbols */
    {
        PrintColumn(aMod, (pstr_t *)inP);    /* print the ancestor */
        ForceSOL();
    }
    if (! seen.purge)         /* if needed in output */
    {
        InitRecOut(R_ANCEST);   /* init record, copy the name and finalise */
        WriteBytes(&inP[0], inP[0] + 1);
        EndRecord();
    }
    GetRecord();    /* prep next record */
} /* ProcAncest */


void ProcDefs(byte list, char *template)
{
    if (! seen.purge || list)   /* only process if ! purged || list requested for these symbols */
    {
        if (*inP /* inSeg */ == SSTACK)   /* stack ? */
        {
            outSegType = SSTACK;
            workingSegBase = 0;         /* stack base is 0 */
        }
		else
            outSegType = SetWorkingSeg(*inP /* inSeg */);  /* sets working base address * returns ABS Seg() */
        if (! seen.purge)
        {
            InitRecOut(inRecordP[RECORD_rectyp]); /* initialise the output record */
            *outP /* outSeg */ = outSegType;
            outP = outP + 1;
        }
        inP = inP + 1;          /* past segId */
        while (inP < erecP) {
            if (! seen.purge)
            {
                putWord(outP + DEF_offset, workingSegBase + getWord(inP + DEF_offset));  /* add in the base address */
                PStrcpy(inP + DEF_name, outP + DEF_name); /* copy the name over */
                PSStr(outP + DEF_name)[PSLen(inP + DEF_name)] = 0;     /* add the trailing 0 */
                outP += 4 + PSLen(inP + DEF_name);        /* update output record past offset, name and extra 0 */
            }
            if (list)          /* if required list the location */
            {
                BinAsc(workingSegBase + getWord(inP + DEF_offset), 16, '0', template, 4);
                PrintColumn(template, (pstr_t *)(inP + DEF_name));
            }
            inP += 4 + PSLen(inP + DEF_name);     /* to the next entry - past offset, name and extra 0 */
        }
        if (! seen.purge)     /* if ! purged finish off the output record */
            EndRecord();
    }
    GetRecord();    /* prep for next record */
} /* ProcDefs */


void ProcExtnam()
{
    InitRecOut(R_EXTNAM);   /* initialise the EXTNAM in the located file */
    while (inP < erecP) {       /* process each item in the input EXTNAM record */
        ForceSOL();
        /* insert the index number of the unsatisfied external & print the Error() message */
        BinAsc((unsatisfiedCnt = unsatisfiedCnt + 1) - 1, 10, ' ', &aUnsatisfiedExt[21], 5);
        /* print to the start of the number */
        ConAndPrint(aUnsatisfiedExt, 21);
        /* print the rest of the message omitting leading spaces in the number */
        curColumn.cp = SkipSpc(&aUnsatisfiedExt[21]);
        ConAndPrint(curColumn.cp, 7 - (word)(curColumn.cp - &aUnsatisfiedExt[21]));
        /* print the name */
        ConAndPrint(PSStr(inP), PSLen(inP)); // pascal string at *inP
        ConAndPrint(crlf, 2);
        /* copy the name to the output record */
        PStrcpy(inP, outP);
        PSStr(outP)[PSLen(inP)] = 0;   /* add the zero */
        outP += 2 + PSLen(inP);      /* skip past the name in out and in records */
        inP += 2 + PSLen(inP);
    }
    EndRecord();    /* finish off the record */
    GetRecord(); /* prep the next record */
} /* ProcExtnam */


void PrintListingHeader(char const *buf, word len)
{
    PrintString(buf, len);
    PrintString(moduleName.str, moduleName.len);
    PrintString(aReadFromFile, 17);    /* '\r\nREAD FROM FILE ' */
    PrintString(inFileName.str, inFileName.len);
    PrintString(aWrittenToFile, 18); /* '\r\nWRITTEN TO FILE ' */
    PrintString(outFileName.str, outFileName.len);
    PrintCrLf();
} /* PrintListingHeader */


/* main procedure to locate the file */
void LocateFile()
{
    ProcHdrAndComDef();     /* determine all of the segment base addresses */
    InitRecOut(R_MODHDR);   /* create header record */
    WriteBytes((pointer)&moduleName, moduleName.len + 1); /* Write() the name */
    outP[0] = modhdrX1;  /* copy the passed in x field data */
    outP[1] = modhdrX2;
    outP = outP + 2;
    EndRecord();        /* located modhdrs don't have seg size info */
    /* print symbol table header message if (required */
    if (seen.publics  || seen.symbols || seen.lines)
    {
        PrintListingHeader(aSymbolTableOfM, 25);
        PrintCrLf();
        /* print the column headings */
        for (curColumn.w = 1; curColumn.w <= columns; curColumn.w++) {
            PrintColumn(aValueType, (pstr_t *)aSymbol);
        }
        PrintCrLf();
    }

    /* process the link file */
    while (inRecordP[RECORD_rectyp] != R_MODEND) {
        switch (inRecordP[RECORD_rectyp] >> 1) {
		case 0: IllegalReloc(); break; /* 0 */
		case 1: BadRecordSeq(); break; /* 2 - modhdr alReady processed */
		case 2: break;                 /* 4 - modend Exited() while alReady */
		case 3: ProcModdat(); break;       /* 6 - moddat */
		case 4: ProcLinNum(); break;       /* 8 - linnum */
		case 5: IllegalReloc(); break; /* 0A */
		case 6: IllegalReloc(); break; /* 0C */
		case 7: ErrChkReport(ERR204, inFileName.str, true); break; /* 0E - modeof Premature() EOF */
		case 8: ProcAncest(); break;       /* 10 - ancest */
		case 9: ProcDefs(seen.symbols, aSym); break;   /* 12 - locdef */
		case 10: IllegalReloc(); break; /* 14 */
		case 11: ProcDefs(seen.publics, aPub); break;   /* 16 - pubdef */
		case 12: ProcExtnam(); break;       /* 18 - extnam - externals */
		case 13: IllegalReloc(); break; /* 1A */
		case 14: IllegalReloc(); break; /* 1C */
		case 15: IllegalReloc(); break; /* 1E */
		case 16: BadRecordSeq(); break; /* 20 - fixext link should have processed these */
		case 17: BadRecordSeq(); break; /* 22 - fixloc link should have processed these */
		case 18: BadRecordSeq(); break; /* 24 - fixseg link should have processed these */
		case 19: BadRecordSeq(); break; /* 26 - libloc attempting to locate a library */
		case 20: BadRecordSeq(); break; /* 28 - libnam attempting to locate a library */
		case 21: BadRecordSeq(); break; /* 2A - libdic attempting to locate a library */
		case 22: BadRecordSeq(); break; /* 2C - libhdr attempting to locate a library */
		case 24: BadRecordSeq(); break; /* 2E - processed comdef alReady */
        }
    }
    ProcModend();       /* Read() the modend and generate the rest of the located file */
    Close(outputfd, &statusIO);
    ErrChkReport(statusIO, outFileName.str, true);
    if (havePagingFile)        /* get rid of any paging file */
    {
        Close(tmpfd, &statusIO);
        Delete(tmpFileName.str, &statusIO);
    }
    ForceSOL();     /* make sure at start of line if (symbols listed */
    PrintMemoryMap();   /* print the final memory map + any overlay Errors() */
    Close(readfd, &statusIO);
    ErrChkReport(statusIO, inFileName.str, true);
} /* LocateFile */

