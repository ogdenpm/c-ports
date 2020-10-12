/****************************************************************************
 *  plm80: C port of Intel's ISIS-II PLM80 v4.0                             *
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


#include "plm.h"
static byte copyright[] = "[C] 1976, 1977, 1982 INTEL CORP";

jmp_buf exception;

static void FinishLexPass()
{
    if (afterEOF)
        SyntaxError(ERR87);	/* MISSING 'end' , end-OF-FILE ENCOUNTERED */
    WriteLineInfo();
    WrByte(L_EOF);
    RewindTx1();
    TellF(&srcFil, (loc_t *)&srcFileTable[srcFileIdx + 8]);
    Backup((loc_t *)&srcFileTable[srcFileIdx + 8], offLastCh - offCurCh);
    CloseF(&srcFil);
} /* FinishLexPass() */


static void ParseCommandLine()
{
    InitF(&srcFil, "SOURCE", (pointer)&srcFileTable[srcFileIdx]);
    OpenF(&srcFil, 1);
    SeekF(&srcFil,  (loc_t *)&srcFileTable[srcFileIdx + 8]);
    offCurCh = offLastCh;
    if (offFirstChM1 != 0)
        while (cmdLineP != 0) {
            ParseControlLine(offFirstChM1 + ByteP(cmdLineP));   // ParseControlLine will move to first char
            offFirstChM1 = 2;   // offset to first char - 1
            cmdLineP = CmdP(cmdLineP)->link;                    // continuation line
        }
    offFirstChM1 = 0;       // 0 if no more cmd line
    curScopeP = (wpointer)curScope;
    inChrP = "\n" - 1;      // GNxtCh will see \n and get a non blank line
    blockDepth = 1;
    procChains[1] = 0;
    GNxtCh();               // get the first char
} /* ParseCommandLine() */

static void LexPass()
{
    ParseCommandLine();
    ParseProgram();
} /* LexPass() */


word Start0()
{
    if (setjmp(exception) == 0) {
        state = 20;	/* 9B46 */
        LexPass();
    }
    FinishLexPass();		/* exception goes to here */
    return 1; // Chain(overlay[1]);
}
