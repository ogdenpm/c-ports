/****************************************************************************
 *  initm.c: part of the C port of Intel's ISIS-II asm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"
#include "os.h"
#include <ctype.h>
#include "utility.h"

byte aExtents[] = " lstobj";

/* skip white space in command line */
void CmdSkipWhite(void) {
    while (*cmdchP && (*cmdchP == ' ' || *cmdchP == TAB)) {
        cmdchP++;
    }
}

/* inits usage include overlay file initiatisation */
bool IsWhiteOrCr(byte c) {
    return c == ' ' || c == TAB || c == EOLCH;
}

void PrepSrcFile(char *srcName) {
    char *s;
    bool useLC = true;

    //    symTab[TID_KEYWORD] = (tokensym_t *) extKeywords;    /* extended key words */
    /* set location of symbol table */
    endSymTab[TID_KEYWORD] = symTab[TID_SYMBOL] = endSymTab[TID_SYMBOL] = symTab[TID_MACRO] =
        endSymTab[TID_MACRO]                                            = symbols;

    scanCmdLine = true; /* scanning command line */
    puts("\nINTEL 8080/8085 MACRO ASSEMBLER, V4.1");

    /* check if source filename part is all upper case
       if it is assume extents will be upper case as well
     */
    for (s = srcName; *s && !islower(*s); s++)
        if (isupper(*s))
            useLC = false;
    if (*s)
        useLC = true;

    // derive default lst and obj file names
    // the src path is copied except any extent
    // if the filename part of src was all upper case then
    // .LST or .OBJ are used else .lst or .obj
    s           = strrchr(basename(srcName), '.');
    size_t flen = s ? s - srcName : strlen(srcName);
    lstFile     = safeMalloc(flen + 5);
    objFile     = safeMalloc(flen + 5);
    strncpy(lstFile, srcName, flen);
    strcpy(lstFile + flen, useLC ? ".lst" : ".LST");
    strncpy(objFile, srcName, flen);
    strcpy(objFile + flen, useLC ? ".obj" : ".OBJ");

    files[0].name = srcName;
    srcfp = files[0].fp = SafeOpen(srcName, "rt"); /* Open() file for reading */
}

void ResetData(void) { /* extended initialisation */

    InitLine();
    expandingMacro = mSpoolMode = 0;
    b6B33 = scanCmdLine = skipIf[0] = b6B2C = inElse[0] = expandingMacroParameter = finished =
        segDeclared[0] = segDeclared[1] = inComment = hasVarRef = pendingInclude = false;
    noOpsYet = primaryValid = controls.list = ctlListChanged = true;
    controls.gen                                             = true;
    controls.cond                                            = true;
    macroDepth = macroSpoolNestDepth = macroCondStk[0] = macroCondSP = 0;
    saveIdx = lookAhead = activeSeg = ifDepth = opSP = opStack[0] = 0;
    macroBlkCnt                                                   = 0;
    segLocation[SEG_ABS] = segLocation[SEG_CODE] = segLocation[SEG_DATA] = maxSegSize[SEG_ABS] =
        maxSegSize[SEG_CODE] = maxSegSize[SEG_DATA] = effectiveAddr = localIdCnt = externId =
            errCnt                                                               = 0;
    passCnt++;

    srcLineCnt = pageCnt = pageLineCnt = 1;
    curOp                              = EOL;

    subHeadIdx                         = 0;
    curChar                            = ' ';
    // reset all of the controlSeen flags
    memset(controlSeen, false, sizeof(controlSeen));
    ;

    curMacroBlk = 0xFFFF;
    if (!IsPhase1()) { /* close any Open() include file */
        while (fileIdx != 0) {
            if (fclose(files[fileIdx].fp))
                IoError(files[fileIdx].name, "Close error");
            fileIdx--;
        }
        srcfp = files[0].fp;
        rewind(srcfp);
        inPtr = NULL;
    }

    topMacroArg = macroArgs;
}