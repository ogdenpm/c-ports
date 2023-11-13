/****************************************************************************
 *  listm.c: part of the C port of Intel's ISIS-II asm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"
#include "../shared/cmdline.h"
#include <stdarg.h>

int maxSymWidth = 6; // for formatting symbol tables
char dateStr[22] = { 0 }; // [yyyy-mm-dd hh:mm]

static word lastErrorLine;
static word curCol = 1;

#if !defined(_MSC_VER) && !defined min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

static char *subHeadings[] = { "  LOC  OBJ         LINE        SOURCE STATEMENT",
                               "SYMBOL CROSS REFERENCE",
                               "PUBLIC SYMBOLS",
                               "EXTERNAL SYMBOLS", // these will add (cont) as required
                               "USER SYMBOLS",
                               "SYMBOL CROSS REFERENCE" };

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

#define FIXEDLEN    27 // strlen("INTEL ASM80 V4.1  PAGE nnnn");
#define DATEWIDTH   19 // " [yyyy-mm-dd hh:mm]"
#define HEADERWIDTH 80
static void NewPageHeader(void) {
    char *date;
    int pad = min(HEADERWIDTH, pageWidth) - FIXEDLEN - (int)strlen(moduleName);
    if (pad >= DATEWIDTH) {
        pad -= DATEWIDTH;
        date = dateStr;
    } else
        date = "";
    Printf("\n\n\nINTEL ASM80 V4.1 %*s%s%*s%s PAGE %4u\n", pad / 2, "", moduleName, pad - pad / 2,
           "", date, pageCnt);

    if (controls.title)
        PrintStr(titleStr);

    Printf("\n\n%s%s\n\n", subHeadings[subHeadIdx], subHeadIdx > 1 ? " (cont)" : "");
    pageCnt++;
}

static void NewPage(void) {
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

void PrintChar(char c) {
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

void PrintSymbols(void) {
    subHeadIdx = true;
    if (!controls.symbols)
        return;
    bool showSym = IsPhase2Print();
    controls.debug |= controls.macroDebug;
    /* changes to better reflect what is happening rather than use strange offsets */
    segChar[0] = 'A'; /* show A instead of space for absolute */
    for (byte symGrp = 0; symGrp < 3; symGrp++) {
        subHeadIdx   = symGrp + 2;
        token.symbol = symTab[TID_SYMBOL] - 1; /* word user sym[-1].type */
        Printf("\n\n%s\n", subHeadings[subHeadIdx]);

        while (++token.symbol < endSymTab[TID_SYMBOL]) { // converted for c pointer arithmetic
            byte type     = token.symbol->type;
            byte flags    = token.symbol->flags;
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

                                    Printf("%-*s ", maxSymWidth, token.symbol->name);
                                    if (type == MACRONAME)
                                        PrintChar('+');
                                    else if (isExtSym)
                                        PrintChar('E');
                                    else
                                        PrintChar(segChar[flags & UF_SEGMASK]);

                                    Printf(" %04X    ", isExtSym ? 0 : token.symbol->value);
                                }
                            }
                    }
        }
    }

    if (controls.debug)
        subHeadIdx = 0;

    if (showSym)
        PrintChar(LF);
}

void AsmPrintCmdLine(void) {
    Outch(FF);
    DoEject();
    pageLineCnt += printCmdLine(lstFp, pageWidth, 0);
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
            Printf("%02X", lineBuf[startItem]);
        } else
            OutSpc(2);
        startItem++;
    }

    Outch(' ');
    byte attr = tokenStk[spIdx].attr;
    if (attr & UF_EXTRN) /* UF_EXTRN */
        Outch('E');
    else if (!showAddr)
        Outch(' ');
    else
        Outch(segChar[attr & 7]);
}

static void PrintErrorLineChain(void) {
    if (!errorOnLine)
        return;
    Printf(" (%4u)\n", lastErrorLine);
    lastErrorLine = lineNo;
}

void PrintLine(void) {
    while (1) {
        endItem = (startItem = tokenStk[spIdx].start) + tokenStk[spIdx].size;
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
            byte nestLvl = pendingInclude ? fileIdx - 1 : fileIdx;
            Outch("  1234"[nestLvl]);
            Outch(nestLvl > 0 ? '=' : ' ');
        } else
            OutStr("  ");

        if (lineNumberEmitted)
            PrintStr("    \n");
        else {
            lineNumberEmitted = true;
            curCol            = 19;
            Printf("%4u%c", lineNo, expandingMacro > 1 ? '+' : ' ');
            if (expandingMacro > 1) {
                macroLine[macroPIdx] = 0;
                PrintStr(macroLine);
            } else
                PrintStr(inBuf); // length may exceed limits of Printf
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
        sprintf(msg, "\nASSEMBLY COMPLETE, %4u ERROR%s (%4u )\n", errCnt, errCnt != 1 ? "S" : "",
                lastErrorLine);
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