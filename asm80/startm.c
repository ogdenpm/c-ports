/****************************************************************************
 *  startm.c: part of the C port of Intel's ISIS-II asm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"
#include "../shared/os.h"
#include "../shared/cmdline.h"
#include <ctype.h>
#include  <time.h>

byte b3782[2]                   = { 0x81, 0x80 };
char moduleName[MAXSYMSIZE + 1] = "MODULE";
// static byte copyright[] = "(C) 1976,1977,1979,1980 INTEL CORP";

int macroTextSize;

byte GetCmdCh(void) {
    int c = *cmdchP++;
    return c != '\r' ? c : ' ';
}

void Outch(char c) {
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

bool IsEOL(void) {
    return curChar == EOLCH;
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
    if (!IsEOL())
        while (GetCh() != EOLCH)
            ;
}

bool ChkGenObj(void) {
    return phase == 2 && controls.object;
}

bool IsPhase2Print(void) {
    return phase == 2 && controls.print;
}

void RuntimeError(byte errCode) {
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

    if (errCode == RTE_FILE) { /* file? */
        if (tokBufIdx == 0) {
            fputs("BAD SYNTAX\n", stderr);
            if (!scanCmdLine) {
                Skip2EOL();
                fprintf(stderr, "%4u\n", srcLineCnt);
            }
        } else {
            fputs(tokBuf, stderr);
            fputc('\n', stderr);
        }
    }

    if (errCode == RTE_STACK) { /* stackError - suppress cascade errors */
        skipRuntimeError = true;
        return;
    }
    exit(1);
}

/* open file for read with status check */
FILE *SafeOpen(char const *pathP, char *access) {
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

byte GetNibble(pointer bp, byte idx) // not used
{
    byte n;

    bp += (idx >> 1); /* index into buffer the number of nibbles */
    n = *bp;          /* pick up the byte there */
    if (!(idx & 1))   /* pick up the right nibble */
        n >>= 4;
    return n & 0xF; /* mask to leave just the nibble */
}

void SourceError(byte errCh) {
    if (!IsSkipping() || topOp == ELSE) { /* ELSE */
        if (inExtrn)
            badExtrn = true;
        if (BlankAsmErrCode())
            errCnt++;

        if (MPorNoErrCode() || errCh == 'L' ||
            errCh == 'U')          /* no Error, or Multiple, Phase, Location or Undefined */
            if (asmErrCode != 'L') /* override unless already location counter Error */
                asmErrCode = errCh;
    }
}

void InsertByteInMacroTbl(byte c) {
    if (macroInIdx >= macroTextSize)
        macroText = xrealloc(macroText, macroTextSize += 256);
    macroText[macroInIdx++] = c;
}

void ParseControlLines(void) {
    while (GetCh() == '$') {
        if (IsSkipping()) {
            Skip2EOL();
            isControlLine = true;
            if (mSpoolMode == 1)
                spooledControl = true;
        } else
            ParseControls();
        FinishLine();
    }
    reget = 1; /* push back character */
}

void InitialControls(void) {
    cmdchP      = controlsP;
    scanCmdLine = true;
    ParseControls();
    if (IsPhase2Print())
        AsmPrintCmdLine();
    if (pendingInclude)
        OpenSrc();

    pendingInclude = isControlLine = scanCmdLine = false;
    ParseControlLines();  /* initial control lines allow primary controls */
    primaryValid = false; /* not allowed from now on */
    controls.debug =
        controls.debug && controls.object; /* debug doesn't make sense if no object code */
    controls.xref    = controls.xref && controls.print; /* disable controls if not printing */
    controls.symbols = controls.symbols && controls.print;
    controls.paging  = controls.paging && controls.print;
}

void InitLine(void) {
    if (pendingInclude)
        OpenSrc();

    lineNumberEmitted = has16bitOperand = isControlLine = errorOnLine = haveNonLabelSymbol =
        inExpression = expectOperand = xRefPending = haveUserSymbol = inDB = inDW = condAsmSeen =
            showAddr = usrLookupIsID = excludeCommentInExpansion = b9060 = needsAbsValue = false;
    haveLabel                                                                            = 0;
    atStartLine = expectOpcode = isInstr = expectOp = true;
    controls.eject = tokenIdx = argNestCnt = token.size = token.type = acc1ValType = acc2ValType =
        acc1RelocFlags                                                             = 0;
    hasVarRef = inQuotes = inComment = false;

    asmErrCode                       = ' ';
    macroPIdx                        = 0;
    startMacroLineIdx                = macroInIdx;
    expandingMacro                   = expandingMacro > 0 ? 0xff : 0;
    tokI                             = 1;
    srcLineCnt++;
}

void Start() {
    GetToken();         // skip invoke name
    char *srcName = GetToken();   //
    newInclude(srcName);    // treat as an include for makedepend usage
    PrepSrcFile(srcName);
    controlsP = cmdP; /* controls start after file name */
    phase     = 1;
    ResetData();
    
    time_t now;
    time(&now);
    strftime(dateStr, sizeof(dateStr), " [%F %R]", localtime(&now));

    InitialControls();

    if (controls.object) {
        objFp = SafeOpen(objFile, "wb+");
        RmOnError(objFile);
    }
    DoPass();
    phase = 2;
    if (controls.object) {
        if (getRecLen(rExtnames) > 0)
            WriteRec(rExtnames);

        if (externId == 0)
            WriteModhdr();
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
    if (controls.makedepend)
        WriteDepend(depFile);
    Exit(errCnt != 0);
}




void usage() {
    printf(
        "Usage: %s (-h | -v | -V | inputFile [asm-80 option]*)\n"
        "Where the - options are  (h) show help, (v) show simple version, (V) show extended "
        "version\n"
        "and the case insensitive asm-80 options are shown below. (* indicates default):\n"
        "COND*                | NOCOND        Turn on/off listing of conditional lines in listing.\n"
        "DEBUG                | NODEBUG*      Include local symbols and line information in object "
        "file.\n"
        "EJECT                                Eject listing to next page\n"
        "GEN*                 | MOGEN         Show macro expansion in listing\n"
        "INCLUDE (file)                       Set an initial include file to read\n"
        "LIST*                | NOLIST        Turn on/off listing.\n"
        "MACRODEBUG           | NOMACRODEBUG* Include generated macro symbols in listing and object file\n"
        "MACROFILE* [(drive)] | NOMACROFILE   Add support for MACROS, reserves associated keywords. Drive ignored\n"
        "MAKEDEPEND [(file)]                  Generate makefile dependencies. File defaults to "
        ".deps/{src}.d\n"
        "MOD85*               | MOMOD85       Enable RIM & SIM instructions for 8085\n"
        "OBJECT* [(file)]     | NOOBJECT      Object file. Default OBJECT({src}.obj)\n"
        "PAGELENGTH (length)                  Change length of page for listing from 60\n"
        "PAGEWIDTH (width)                    Change width of page listing from 120\n"
        "PAGING               | NOPAGING*     Enable listing pagination\n"
        "PRINT* [(file)]      | NOPRINT       Listing file. File defaults to {src}.lst\n"
        "RESTORE                              Pop controls (LIST, COND and GEN) from stack\n"
        "SAVE                                 Push controls (LIST, COND and GEN) on stack\n"
        "SYMBOLS*             | NOSYMBOLS     Symbol table in listing file.\n"
        "TITLE('titleString')                 Optional title to show in listing header.\n"
        "TTY                  | NOTTY         Simulate form-feed for teletypewriter output\n"
        "XREF                 | NOXREF*       Cross ref in listing file.\n"
        "See Intel ASM-80 documentation for more details, other than MAKEDEPEND extension\n"
        "Notes:\n"
        "* {src} is source file name minus any extent\n"
        "* MOD85 and MACROFILE are changed defaults from the ISIS hosted assembler\n"
        "* File names are of the format [:Fx:]path, where x is a digit and path\n"
        "  The :Fx:, (x is a digit), maps to a directory prefix from the corresponding ISIS_Fx environment variable\n"
        "* Long lines are supported, as is the Intel '&' line continuation option\n"
        "* Response file input for compiling is supported by using \"%s <file\"\n"
        "* The object file created. is deleted on error, which helps with make builds\n",
        invokeName, invokeName);
}
