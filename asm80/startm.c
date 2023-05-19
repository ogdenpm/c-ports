/****************************************************************************
 *  startm.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"
#include <ctype.h>

byte b3782[2] = { 0x80, 0x81 };
char moduleName[7] = "MODULE";
    //static byte copyright[] = "(C) 1976,1977,1979,1980 INTEL CORP";



byte GetCmdCh(void) {
    return *cmdchP++;
}    


void Outch(char c)
{
        putc(c, lstFp);
}


bool IsSpace(void) {
    return curChar == ' ';
}

bool IsTab(void) {
    return curChar == TAB;
}

bool IsWhite(void) {
    return IsSpace() || IsTab();
}

bool IsRParen(void) {
    return curChar == ')';
}

bool IsCR(void) {
    return curChar == CR;
}

bool IsComma(void) {
    return curChar == ',';
}

bool IsLT(void) {
    return curChar == '<';
}


bool IsGT(void) {
    return curChar == '>';
}


bool IsPhase1(void) {
    return phase == 1;
}

void Skip2EOL(void) {
    if (!IsCR())
        while (GetCh() != CR)
            ;
}


bool ChkGenObj(void) {
    return phase == 2 && controls.object;
}


bool IsPhase2Print(void) {
    return phase == 2 && controls.print;
}



void RuntimeError(byte errCode)
{

    static char const *aErrStrs[] = { "STACK", "TABLE", "COMMAND", "EOF", "FILE", "MEMORY" };

    if (skipRuntimeError)
        return;

    if (IsPhase1() && errCode == RTE_STACK) {
        b6B33 = true;
        return;
    }
    fprintf(stderr, "\n%s ERROR%s", aErrStrs[errCode], errCode == RTE_FILE ? ", " : "\n");
    if (IsPhase2Print()) /* repeat to the print file if required */
        fprintf(lstFp, "\n%s ERROR%s", aErrStrs[errCode], errCode == RTE_FILE ? ", " : "\n");


    if (errCode == RTE_FILE) {    /* file? */
        if (tokBufIdx == 0) {
            fputs("BAD SYNTAX\n", stderr);
            if (!scanCmdLine) {
                Skip2NextLine();
                fprintf(stderr, "%4u\n", srcLineCnt);
            }
        } else {
            fputs(tokBuf, stderr);
            fputc('\n', stderr);
        }
    }

    if (errCode == RTE_STACK) {   /* stackError - suppress cascade errors */
        skipRuntimeError = true;
        return;
    }
    exit(1);
}

/* open file for read with status check */
FILE *SafeOpen(char const *pathP, char *access)
{
    FILE *fp;

    if (!(fp = Fopen(pathP, access)))
        IoError(pathP, "Open error");
    return fp;
}



bool BlankAsmErrCode(void) {
    return asmErrCode == ' ';
}

bool MPorNoErrCode(void) // no error, multiple definition or phase error
{
    return BlankAsmErrCode() || asmErrCode == 'M' || asmErrCode == 'P';
}


byte GetNibble(pointer bp, byte idx)	// not used
{
    byte n;

    bp += (idx >> 1);    /* index into buffer the number of nibbles */
    n = *bp;            /* pick up the byte there */
    if (!(idx & 1))        /* pick up the right nibble */
        n >>= 4;
    return n & 0xF;    /* mask to leave just the nibble */
}

void SourceError(byte errCh)
{
    if (! IsSkipping() || topOp == K_ELSE) {   /* ELSE */
        if (inExtrn)
            badExtrn = true;
        if (BlankAsmErrCode())
            errCnt++;

        if (MPorNoErrCode() || errCh == 'L' || errCh == 'U')    /* no Error, or Multiple, Phase, Location or Undefined */
            if (asmErrCode != 'L')    /* override unless already location counter Error */
                asmErrCode = errCh;
    }
}


void InsertByteInMacroTbl(byte c)
{
    *macroInPtr++ = c;
    if (macroInPtr >= &macroText[MAXMACROTEXT])
        RuntimeError(RTE_TABLE);    /* table Error() */
}


void InsertCharInMacroTbl(byte c)	// as InsertByteInMacroTbl but expands CR to CR LF
{
    InsertByteInMacroTbl(c);
    if (c == CR)
        InsertByteInMacroTbl(LF);
}



void ParseControlLines(void) {
    while (GetCh() == '$') {
        if (IsSkipping()) {
            Skip2NextLine();
            isControlLine = true;
            if (mSpoolMode == 1)
                spooledControl = true;
        } else
            ParseControls();
        FinishLine();
    }
    reget = 1;    /* push back character */
}


void InitialControls(void) {
    cmdchP = controlsP;
    scanCmdLine = true;
    ParseControls();
    if (IsPhase2Print())
        PrintCmdLine();
    if (pendingInclude)
        OpenSrc();
    
    pendingInclude = isControlLine = scanCmdLine = false;
    ParseControlLines();            /* initial control lines allow primary controls */
    primaryValid = false;            /* not allowed from now on */
    controls.debug = controls.debug && controls.object;    /* debug doesn't make sense if no object code */
    controls.xref = controls.xref && controls.print;        /* disable controls if not printing */
    controls.symbols = controls.symbols && controls.print;
    controls.paging = controls.paging && controls.print;
}


void InitLine(void) {
    lineChCnt  = 0;
    if (pendingInclude)
        OpenSrc();
#ifdef SHOWLINE
    for (int i  = 0; i < lineChCnt; i++)
        putchar(inBuf[i]);
#endif
    lineNumberEmitted = has16bitOperand = isControlLine = errorOnLine = haveNonLabelSymbol =
        inExpression = expectingOperands = xRefPending = haveUserSymbol = inDB = inDW =
            condAsmSeen = showAddr = usrLookupIsID = excludeCommentInExpansion = b9060 =
                needsAbsValue                                                  = false;
    gotLabel                                                                   = 0;
    atStartLine = expectingOpcode = isInstr = expectOp = true;
    controls.eject = tokenIdx = argNestCnt = token[0].size = token[0].type = acc1ValType =
        acc2ValType = acc1RelocFlags = 0;
    hasVarRef = inQuotes = inComment = false;

    asmErrCode                       = ' ';
    macroP                           = macroLine;
    startMacroLine                   = macroInPtr;
    expandingMacro                   = expandingMacro > 0 ? 0xff : 0;
    tokI                             = 1;
    srcLineCnt++;
    macroP = macroLine;
}

void Start(char *srcName) {
    PrepSrcFile(srcName);
    controlsP = cmdchP; /* controls start after file name */
    phase = 1;
    ResetData();
    InitialControls();
    if (!(macroFp = tmpfile()))
        IoError("Macro file", "Create error");

    if (controls.object)
        objFp = SafeOpen(objFile, "wb+");

    DoPass();
    phase = 2;
    if (controls.object) {
        if (getLen(rExtnames) > 0)
            WriteRec(rExtnames);    /* in overlay 2 */

        if (externId == 0)
            WriteModhdr();        /* in overlay 2 */
        InitRecTypes();
    }
    if (controls.print)
        lstFp = SafeOpen(lstFile, "wt");
    ResetData();
    InitialControls();
    DoPass();
    if (controls.print)
        AsmComplete(lstFp);

    if (controls.object) {
        Ovl11();
        WriteModend();
    }

    FinishPrint();
    FinishAssembly();
    exit((killObjFile = errCnt != 0));
}

