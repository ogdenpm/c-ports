/****************************************************************************
 *  initm.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"

byte aExtents[] = " lstobj";
static byte aDebug[] = "DEBUG";


/* skip white space in command line */
void CmdSkipWhite(void) { 
    while (*cmdchP && (*cmdchP == ' ' || *cmdchP == TAB)) {
        cmdchP++;
    }
}

/*
    return drive ('0'-'9') of current program.
    skips TRACE if present
    returns '0' if no drive specified
*/
byte GetDrive(void) {
    if (*cmdchP == ':') {
        cmdchP += 2;
        return *cmdchP;
    } else
        for (ii = 0; ii <= 4; ii++) {        /* case insensitive compare to TRACE */
        if (*cmdchP != aDebug[ii] && aDebug[ii] + 0x20 != *cmdchP)
                return '0';    /* must be a file name so drive 0 */
            cmdchP++;
        }
    CmdSkipWhite();
    if (*cmdchP != ':')
        return '0';
    cmdchP += 2;
    return *cmdchP;
}

void AddExtents(void) {
    for (ii = 1; ii <= 3; ii++) {
        lstFile[kk + ii] = aExtents[ii];
        objFile[kk + ii] = aExtents[ii+3];
    }
    lstFile[kk + 4] = objFile[kk + 4] = '\0';
}


/* inits usage include overlay file initiatisation */
bool IsWhiteOrCr(byte c)
{
    return c == ' ' || c == TAB || c == CR;
}

void GetAsmFile(void) {
    /* select key words depending on whether macro version or not */
    symTab[TID_KEYWORD] = (tokensym_t *) extKeywords;    /* extended key words */
    /* set location of symbol table */
    endSymTab[TID_KEYWORD] = symTab[TID_SYMBOL] = endSymTab[TID_SYMBOL] = (tokensym_t *)(symHighMark = MEMORY);
   
    scanCmdLine = true;        /* scanning command line */
    puts("\nISIS-II 8080/8085 MACRO ASSEMBLER, V4.1");

    CmdSkipWhite();

    while (! IsWhiteOrCr(*cmdchP)) {    /* skip to end of program name */
        cmdchP++;
    }

    CmdSkipWhite();
    if (*cmdchP == CR)        /* no args !! */
        RuntimeError(RTE_FILE);

    ii = true;      /* copying name */
    kk = 0;     /* length of file name */
    while (! IsWhiteOrCr(*cmdchP) && kk < 14) {    /* copy file name over to the files list */
        files[0].name[kk] = *cmdchP;
        if (ii)        /* and the name for the lst && obj files */
            lstFile[kk] = objFile[kk] = *cmdchP;
        if (*cmdchP == '.')
        {
            AddExtents();    /* add lst and obj file extents */
            // PMO ii = false moved to after AddExtents as AddExtents leaves ii = 4 which is false in plm
            ii = false; /* don't copy extent for lst & obj files */
        }
        kk++;
        cmdchP++;
    }
    files[0].name[kk] = '\0';
    srcfp = files[0].fp = SafeOpen(files[0].name, "rt"); /* Open() file for reading */
    controlsP = cmdchP;        /* controls start after file name */
    if (ii)            /* no extent in source file */
    {
        lstFile[kk] = '.';    /* add the . and the extents */
        objFile[kk] = '.';
        AddExtents();
    }

    files[0].name[kk] = ' ';    /* append trailing space */
}


void ResetData(void) {    /* extended initialisation */

    InitLine();
    expandingMacro = mSpoolMode = 0;
    b6B33 = scanCmdLine = skipIf[0] = b6B2C = inElse[0] =  expandingMacroParameter =
        finished = segDeclared[0]  = segDeclared[1] = inComment = hasVarRef = pendingInclude = false;
    noOpsYet = primaryValid = controls.list = ctlListChanged = true;
    controls.gen = true;
    controls.cond = true;
    macroDepth = macroSpoolNestDepth = macroCondStk[0] = macroCondSP = 0; 
    saveIdx = lookAhead = activeSeg = ifDepth = opSP = opStack[0] = 0;
    macroBlkCnt = 0;
    segLocation[SEG_ABS] = segLocation[SEG_CODE] = segLocation[SEG_DATA] =
        maxSegSize[SEG_ABS] = maxSegSize[SEG_CODE] = maxSegSize[SEG_DATA] =
        effectiveAddr.w = localIdCnt = externId = errCnt = 0;
    passCnt++;

    srcLineCnt = pageCnt = pageLineCnt = 1;
    curOp                              = T_CR;

    b68AE = false;
    curChar = ' ';
    for (ii = 0; ii <= 11; ii++)          /* reset all the control seen flags */
        controlSeen[ii] = false;

    curMacroBlk = 0xFFFF;
    if (! IsPhase1()) {   /* close any Open() include file */
        while (fileIdx != 0) {
            if (fclose(files[fileIdx].fp))
                IoError(files[fileIdx].name,  "Close error");
            fileIdx--;
        }
        srcfp   = files[0].fp;
        rewind(srcfp);
    }

    baseMacroTbl = Physmem() + 0xBF;
}

void InitRecTypes(void) {
    rContent.type = OMF_CONTENT;
    putWord(rContent.len, 3);
    rPublics.type = OMF_RELOC;
    putWord(rPublics.len, 1);
    rInterseg.type = OMF_INTERSEG;
    putWord(rInterseg.len, 2);
    rExtref.type = OMF_EXTREF;
    putWord(rExtref.len, 1);
}

