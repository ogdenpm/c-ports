/****************************************************************************
 *  listm.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"
#include <stdarg.h>

int maxSymWidth = 6;
 
static void PrintChar(char c);



static char lstHeader[]             = "  LOC  OBJ         LINE        SOURCE STATEMENT\n\n";
static char const *symbolMsgTable[] = { "PUBLIC SYMBOLS", "EXTERNAL SYMBOLS",
                                        "USER SYMBOLS" };

int Printf(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[512];
    int cnt = vsprintf(buf, fmt, args);
    for (char *s = buf; *s; s++)
        PrintChar(*s);
    va_end(args);
    return cnt;
}


static void PrintStr(char const *str) {
    while (*str != 0)
        PrintChar(*str++);
}


void SkipToEOP(void) {
    while (pageLineCnt <= pageLength) {
        Outch(LF);
        pageLineCnt++;
    }
}

#define FIXEDLEN 28  // strlen("INTEL ASM80 V4.1  PAGE nnnn");
static void NewPageHeader(void) {
    int pad = (67 - FIXEDLEN - (int)strlen(moduleName));
    Printf("\n\n\nINTEL ASM80 V4.1 %*s%s%*s PAGE %4u\n", pad / 2, "",
           moduleName, pad - pad / 2, "",  pageCnt);

    if (controls.title)
        PrintStr(titleStr);

    PrintStr("\n\n");
    if (!b68AE)
        PrintStr(lstHeader);
    pageCnt++;
}

void NewPage(void) {
    if (curCol) {
        Outch(LF);
        curCol = 0;
    }
    if (controls.tty)
        SkipToEOP();
    else
        Outch(FF);

    pageLineCnt = 1;
    if (!scanCmdLine)
        NewPageHeader();
}

void DoEject(void) {
    if (ShowLine())
        while (controls.eject > 0) {
            NewPage();
            controls.eject--;
        }
}

static void PrintChar(char c) {
    byte cnt;

    if (c == FF) {
        NewPage();
        return;
    }


    if (c == LF) {
        if (controls.paging) {
            if (++pageLineCnt >= pageLength - 2) {
                if (controls.tty)
                    Outch(LF);
                if (controls.eject > 0)
                    controls.eject--;
                NewPage();
                return;
            }
        }
        curCol = 0; // replaces '\r' to reset col to 0
    }

    cnt = 1;
    if (c == TAB) {
        cnt = 8 - (curCol & 7);
        c   = ' ';
    }

    while (cnt != 0) {
        if (curCol < 132) {
            if (c >= ' ')
                curCol++;
            if (curCol > pageWidth) {
                PrintChar(LF);
                fprintf(lstFp, "%24s", "");
                curCol = 25;
            }
            Outch(c);
        }
        cnt--;
    }
}

static byte segChar[] = " CDSME"; /* seg id char */

void Sub7041_8447(void) {

    b68AE = true;
    if (!controls.symbols)
        return;
    bool showSym = IsPhase2Print();
    controls.debug |= controls.macroDebug;
    /* changes to better reflect what is happening rather than use strange offsets */
    segChar[0] = 'A'; /* show A instead of space for absolute */
    for (byte symGrp = 0; symGrp <= 2; symGrp++) { 
        topSymbol  = symTab[TID_SYMBOL] - 1; /* word user sym[-1].type */
        Printf("\n\n%s\n", symbolMsgTable[symGrp]);

        while (++topSymbol < endSymTab[TID_SYMBOL]) { // converted for c pointer arithmetic
            byte type  = topSymbol->type;
            byte flags = topSymbol->flags;
            bool isExtSym = (flags & UF_EXTRN);
            if (type != 9)
                if (type != 6)
                    if (NonHiddenSymbol()) {
                        static byte symGrpFlags[2] = { UF_PUBLIC, UF_EXTRN };

                        if (symGrp != 0 || type != 3)
                            if (symGrp == 2 || (flags & symGrpFlags[symGrp]) != 0) {
                                if (showSym) {
                                    if (pageWidth - curCol < 11 + maxSymWidth)
                                        PrintChar(LF);

                                    Printf("%-*s ", maxSymWidth, topSymbol->name);
                                    if (type == MACRONAME)
                                        PrintChar('+');
                                    else if (isExtSym)
                                        PrintChar('E');
                                    else
                                        PrintChar(segChar[flags & UF_SEGMASK]);

                                    Printf(" %04X    ", isExtSym ? 0 : topSymbol->value);
                                }
                            }
                    }
        }
    }

    if (controls.debug)
        b68AE = false;

    if (showSym)
        PrintChar(LF);
}

void PrintCmdLine(void) {
    Outch(FF);
    DoEject();
    Printf("%s %s", _argv[0], _argv[1]);
    for (int i = 2; i < _argc; i++)
        Printf("%s%s", curCol + strlen(_argv[i]) > pageWidth ? " \\\n    " : " ", _argv[i]);
    PrintChar('\n');
    NewPageHeader();
}

void OutStr(char const *s) {
    while (*s)
        Outch(*s++);
}

void OutSpc(int n) {
    while (n-- != 0)
        Outch(' ');
}

static bool MoreBytes(void) {
    return startItem < endItem;
}

static void PrintCodeBytes(void) {
    byte i;

    if (showAddr |= MoreBytes()) { /* print the word */
        Printf("%04X", effectiveAddr);
    } else
        OutSpc(4);

    Outch(' ');
    for (i = 1; i <= 4; i++) {
        if (MoreBytes() && isInstr) {
            effectiveAddr++;
            Printf("%02X", *startItem);
        } else
            OutSpc(2);
        startItem++;
    }

    Outch(' ');
    if ((kk = token[spIdx].attr) & UF_EXTRN) /* UF_EXTRN */
        Outch('E');
    else if (!showAddr)
        Outch(' ');
    else
        Outch(segChar[kk & 7]);
}

static void PrintErrorLineChain(void) {
    if (!errorOnLine)
        return;
    Printf(" (%4u)\n", lastErrorLine);
    lastErrorLine = lineNo;
}

void PrintLine(void) {
    while (1) {
        endItem = (startItem = token[spIdx].start) + token[spIdx].size;
        if (IsSkipping())
            endItem = startItem;

        Outch(asmErrCode);
        Outch(mSpoolMode == 0xFF ? '-' : ' ');

        if (!BlankAsmErrCode()) {
            asmErrCode  = ' ';
            errorOnLine = true;
        }
        if (isControlLine)
            OutSpc(15);
        else
            PrintCodeBytes();

        if (fileIdx > 0) {
            byte nestLevel[] = "  1234";
            /* plm uses byte arith so pendingInclude = true(255) treated as -1 */
            Outch(nestLevel[ii = pendingInclude ? fileIdx - 1 : fileIdx]);
            Outch(ii > 0 ? '=' : ' ');
        } else
            OutStr("  ");

        if (lineNumberEmitted)
            PrintStr("    \n");
        else {
            lineNumberEmitted = true;
            curCol            = 19;
            Printf("%4u%c", lineNo, expandingMacro > 1 ? '+' : ' ');
            if (expandingMacro > 1) {
                *macroP = 0;
                PrintStr(macroLine);
                PrintChar('\n');
            } else
                PrintStr(inBuf);    // length may exceed limits of Printf
        }

        if (isControlLine) {
            if (controls.paging)
                DoEject();
        } else {
            while (MoreBytes()) {
                OutSpc(2);
                PrintCodeBytes();
                PrintChar(LF);
            }

            if (spIdx > 0 && (inDB || inDW)) {
                Sub546F();
                continue;
            }
        }
        break;
    }

    PrintErrorLineChain();
}

void AsmComplete(FILE *fp) {
    char msg[80];
    if (!errCnt)
        sprintf(msg, "\nASSEMBLY COMPLETE,   NO ERRORS\n");
    else
        sprintf(msg, "\nASSEMBLY COMPLETE, %4u ERROR%s (%4u )\n", errCnt,
                errCnt != 1 ? "S" : "", lastErrorLine);
    if (fp == lstFp)
        PrintStr(msg);
    else
        fputs(msg, fp);
}

void FinishPrint(void) {
    pageLineCnt = 1;
    AsmComplete(stdout);
}

void FinishAssembly(void) {
    fclose(files[0].fp);

    if (controls.xref) /* invoke asxref ?? */
        GenAsxref();


}
