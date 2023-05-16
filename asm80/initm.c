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


/* inits usage include overlay file initiatisation */
bool IsWhiteOrCr(byte c)
{
    return c == ' ' || c == TAB || c == CR;
}

void PrepSrcFile(char *srcName) {
    char *s;

    symTab[TID_KEYWORD] = (tokensym_t *) extKeywords;    /* extended key words */
    /* set location of symbol table */
    endSymTab[TID_KEYWORD] = symTab[TID_SYMBOL] = endSymTab[TID_SYMBOL] = (tokensym_t *)(symHighMark = MEMORY);
   
    scanCmdLine = true;        /* scanning command line */
    puts("\nISIS-II 8080/8085 MACRO ASSEMBLER, V4.1");

 
    // derive default lst and obj file names
    // the src path is copied except any extent
    // if the filename part of src was all upper case then
    // .LST or .OBJ are used else .lst or .obj
    s  = strrchr(basename(srcName), '.');
    size_t flen = s ? s - srcName : strlen(srcName);
    lstFile  = malloc(flen + 5);
    objFile  = malloc(flen + 5);
    strncpy(lstFile, srcName, flen);
    strcpy(lstFile + flen, useLC ? ".lst" : ".LST");
    strncpy(objFile, srcName, flen);
    strcpy(objFile + flen, useLC ? ".obj" : ".OBJ");


    files[0].name = srcName;
    srcfp = files[0].fp = SafeOpen(srcName, "rt"); /* Open() file for reading */

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
        effectiveAddr = localIdCnt = externId = errCnt = 0;
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
    rContent[HDR_TYPE] = OMF_CONTENT;
    putWord(&rContent[HDR_LEN], 3);
    rPublics[HDR_TYPE] = OMF_RELOC;
    putWord(&rPublics[HDR_LEN], 1);
    rInterseg[HDR_TYPE] = OMF_INTERSEG;
    putWord(&rInterseg[HDR_LEN], 2);
    rExtref[HDR_TYPE] = OMF_EXTREF;
    putWord(&rExtref[HDR_LEN], 1);
}

