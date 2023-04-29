/****************************************************************************
 *  asm80: C port of ASM80 v4.1                                             *
 *  Copyright (C) 2020 Mark Ogden <mark.pm.ogden@btinternet.com>            *
 *                                                                          *
 *  This program is free software; you can redistribute it and/or           *
 *  modify it under the terms of the GNU General Public License             *
 *  as published by the Free Software Foundation; either version 2          *
 *  of the License, or (at your option) any later version.                  *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program; if not, write to the Free Software             *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,              *
 *  MA  02110-1301, USA.                                                    *
 *                                                                          *
 ****************************************************************************/


#include "asm80.h"
/* to force the code generation this needs a non-standard definition of put2Hex */
void Put2Hex(void (*func)(char), word arg2w);
static void PrintChar(char c);

static char aAssemblyComple[] = "\r\nASSEMBLY COMPLETE,   NO ERRORS";
#define aNoErrors	(aAssemblyComple + 20)
static char spaceLP[] = " (     )";
#define space5RP	(spaceLP + 2)
//static word pad754E;
static char lstHeader[] = "  LOC  OBJ         LINE        SOURCE STATEMENT\r\n\n";
static char const *symbolMsgTable[] = { "\r\nPUBLIC SYMBOLS\r\n", "\r\nEXTERNAL SYMBOLS\r\n", "\r\nUSER SYMBOLS\r\n" };


static void Out2Hex(byte n)
{
    Put2Hex(Outch, n);
}


static void Print2Hex(byte n)
{
    Put2Hex(PrintChar, n);
}



static void PrintStr(char const *str)
{
    while (*str != 0)
        PrintChar(*str++);
}

static void PrintNStr(byte cnt, char const *str)
{
    while (cnt-- > 0)
        PrintChar(*str++);
}


static void PrintCRLF(void) {
    PrintChar(CR);
    PrintChar(LF);
}

static char aNumStr[] = "     ";


static void Itoa(word n, char *buf)
{
    memcpy(buf, spaces5, 5);
    buf += 4;

    while (1) {
        *buf-- = n % 10 + '0';
        if ((n /= 10) == 0)
            return;
    }
}


void PrintDecimal(word n)
{
    Itoa(n, aNumStr);
    PrintStr(&aNumStr[1]);
}

void SkipToEOP(void)
{
    while (pageLineCnt <= controls.pageLength) {
        Outch(LF);
        pageLineCnt++;
    }
}


static void NewPageHeader(void)
{
//    byte twoLF[] = "\r\n\n";        /* Not used */
//    byte threeLF[] = "\r\n\n\n";    /* CR not used */

    PrintStr("\n\n\n");
    PrintStr(asmHeader);
    PrintDecimal(pageCnt);
    PrintCRLF();
    if (controls.title)
        PrintNStr(titleLen, titleStr);

    PrintCRLF();
    PrintCRLF();
    if (! b68AE)
        PrintStr(lstHeader);
    pageCnt++;
}


void NewPage(void)
{
    if (controls.tty)
        SkipToEOP();
    else
        Outch(FF);

    pageLineCnt = 1;
    if (! scanCmdLine)
        NewPageHeader();
}


void DoEject(void)
{
    if (ShowLine())
    while (controls.eject > 0) {
        NewPage();
        controls.eject--;
    }
}




static void PrintChar(char c)
{
    byte cnt;

    if (c == FF) {
        NewPage();
        return;
    }

    if (c == LF)
        if (controls.paging) {
            if (++pageLineCnt >= controls.pageLength - 2) {
                if (controls.tty)
                    Outch(LF);
                if (controls.eject > 0)
                    controls.eject = controls.eject - 1;
                NewPage();
                return;
            }
        }

    if (c == CR)
        curCol = 0;

    cnt = 1;
    if (c == TAB) {
        cnt = 8 - (curCol & 7);
        c = ' ';
    }

    while (cnt != 0) {
        if (curCol < 132) {
            if (c >= ' ')
                curCol = curCol + 1;
            if (curCol > controls.pageWidth) {
                PrintCRLF();
                PrintStr(spaces24);
                curCol++;
            }
            Outch(c);
        }
        cnt--;
    }
}

static byte segChar[] = " CDSME";    /* seg id char */

void PrintAddr2(void(*printFunc)(byte), byte zeroAddr)	// no longer used
{
    tokenSym.bPtr--;    /* backup into value */
    
    printFunc(zeroAddr ? 0 : *tokenSym.bPtr);    /* print word or 0 */
}

void Sub7041_8447(void)
{
    byte symGrp;
    byte type, flags;
    byte zeroAddr = false;	// fix potentially not initialised bug. plm would have held value from previous call

    b68AE = true;
    if (!controls.symbols)
        return;
    /* changes to better reflect what is happening rather than use strange offsets */
    segChar[0] = 'A';        /* show A instead of space for absolute */
    for (symGrp = 0; symGrp <= 2; symGrp++) {
        kk = IsPhase2Print() && controls.symbols;
        controls.debug = controls.debug || controls.macroDebug;
        tokenSym.curP = symTab[TID_SYMBOL] - 1;        /* word user sym[-1].type */
        PrintCRLF();
        PrintStr(symbolMsgTable[symGrp]);

        while (++tokenSym.curP < endSymTab[TID_SYMBOL]) {	// converted for c pointer arithmetic
            type = tokenSym.curP->type;
            flags = tokenSym.curP->flags;
            if (type != 9)
                if (type != 6)
                    if (NonHiddenSymbol()) {
                        byte symGrpFlags[2] = { UF_PUBLIC, UF_EXTRN };

                        if (symGrp != 0 || type != 3)
                            if (symGrp == 2 || (flags & symGrpFlags[symGrp]) != 0) {
                                UnpackToken(tokenSym.curP->tok, tokStr);
                                if (kk) {
                                    if (controls.pageWidth - curCol < 17)
                                        PrintCRLF();

                                    PrintStr(tokStr);
                                    PrintChar(' ');
                                    if (type == T_MACRONAME)
                                        PrintChar('+');
                                    else if ((zeroAddr = (flags & UF_EXTRN) != 0))
                                        PrintChar('E');
                                    else
                                        PrintChar(segChar[flags & UF_SEGMASK]);

                                    PrintChar(' ');
                                    Print2Hex(zeroAddr ? 0 : tokenSym.curP->value >> 8);
                                    Print2Hex(zeroAddr ? 0 : tokenSym.curP->value & 0xff);
                                    PrintStr(spaces4);
                                }
                            }
                    }
        }
    }

    if (controls.debug)
        b68AE = false;

    if (kk)
        PrintCRLF();

}


void PrintCmdLine(void)
{
    Outch(FF);
    DoEject();
    *actRead.bp = 0;
    PrintStr(cmdLineBuf);
    NewPageHeader();
}


void OutStr(char const *s)
{
    while (*s)
        Outch(*s++);
}

static void OutNStr(word cnt, pointer s)
{
    while (cnt-- > 0)
        Outch(*s++);
}


static bool MoreBytes(void)
{
    return startItem < endItem;
}



static void PrintCodeBytes(void) {
    byte i;

    if (showAddr |= MoreBytes()) {    /* print the word */
        Out2Hex(effectiveAddr.hb);
        Out2Hex(effectiveAddr.lb);
    } else
        OutStr(spaces4);

    Outch(' ');
    for (i = 1; i <= 4; i++) {
        if (MoreBytes() && isInstr) {
            effectiveAddr.w++;
            Out2Hex(*startItem);
        } else
            OutStr(spaces2);
        startItem++;
    }

    Outch(' ');
    if ((kk = tokenAttr[spIdx]) & UF_EXTRN)    /* UF_EXTRN */
        Outch('E');
    else if (! showAddr)
        Outch(' ');
    else
        Outch(segChar[kk & 7]);
}


static void PrintErrorLineChain(void) {
    char LP[] = " (";
    char RP[] = ")";

    if (! errorOnLine)
        return;

    PrintStr(LP); 
    PrintNStr(4, lastErrorLine);
    PrintStr(RP);
    PrintCRLF();
    memcpy(lastErrorLine, asciiLineNo, 4);
}



void PrintLine(void)
{
    while (1) {
        endItem = (startItem = tokStart[spIdx]) + tokenSize[spIdx];
        if (IsSkipping())
            endItem = startItem;

        Outch(asmErrCode);
        if (mSpoolMode == 0xFF)
            Outch('-');
        else
            Outch(' ');

        if (!BlankAsmErrCode()) {
            asmErrCode = ' ';
            errorOnLine = true;
        }
        if (isControlLine)
            OutStr(spaces15);
        else
            PrintCodeBytes();

        if (fileIdx > 0) {
            byte nestLevel[] = "  1234";
            /* plm uses byte arith so pendingInclude = true(255) treated as -1 */
            Outch(nestLevel[ii = pendingInclude ? fileIdx + 1 : fileIdx]);
            if (ii > 0)
                Outch('=');
            else
                Outch(' ');
        }
        else
            OutStr(spaces2);

        if (lineNumberEmitted) {
            OutStr(spaces4);
            PrintCRLF();
        }
        else {
            lineNumberEmitted = true;
            OutNStr(4, asciiLineNo);
            if (expandingMacro > 1)
                Outch('+');
            else
                Outch(' ');
            if (expandingMacro > 1) {
                curCol = 24;
                *macroP = 0;
                PrintStr(macroLine);
                PrintChar(LF);
            }
            else {
                curCol = 24;
                PrintNStr(lineChCnt, startLineP);
                if (*inChP != LF)
                    PrintChar(LF);
            }
        }

        if (isControlLine) {
            if (controls.paging)
                DoEject();
        }
        else {
            while (MoreBytes()) {
                OutStr(spaces2);
                PrintCodeBytes();
                PrintCRLF();
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

void AsmComplete(void)
{
    if (errCnt > 0)
        Itoa(errCnt, aNoErrors);
    PrintNStr(32 - (errCnt == 1), aAssemblyComple);
    if (errCnt > 0) {
        memcpy(space5RP, lastErrorLine, 4);
        PrintNStr(8, spaceLP);
    }
    Outch(CR);
    Outch(LF);
}

void FinishPrint(void)
{
    if (controls.print)
        CloseF(outfd);
    outfd = 0;
    pageLineCnt = 1;
    AsmComplete();
    Flushout();
}

void FinishAssembly(void)
{
    CloseF(infd);
    CloseF(macrofd);
    Delete(asmacRef, &statusIO);
    if (controls.object)   /* ?? why only for MACRO version */
        CloseF(objfd);

    if (controls.xref) /* invoke asxref ?? */
        GenAsxref();

    Exit(errCnt != 0);
}
