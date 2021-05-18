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

static word pad1 = {0x40};
static word pad2;


bool StrUcEqu(pointer s, pointer t)
{
    while (*s != 0) {
        if (*s != *t && *s != (*t & 0x5F))
            return false;
        s++;
        t++;
    }
    return true;
}


bool IsSkipping()
{
    return (mSpoolMode & 1) || skipIf[0];
}

void Sub546F()
{
    spIdx = NxtTokI();
    if (expectingOperands)
        SyntaxError();
    if (HaveTokens())
        if (!(tokenType[spIdx] == O_DATA || lineNumberEmitted))
            SyntaxError();
    if (inDB || inDW) {
            if (tokenIdx == 1 && ! BlankAsmErrCode() && tokenSize[0] != 1)
            tokenSize[0] = 2;
    }
    else if (! BlankAsmErrCode() && HaveTokens())
        if (tokenSize[spIdx] > 3)
            tokenSize[spIdx] = 3;
}


void FinishLine()
{
    pointer linenoP;
    bool updating;

    Sub546F();
    if (IsPhase2Print()) {    /* update the ascii line number */
        linenoP = &asciiLineNo[3];    /* point to last digit */
        updating = true;

        while (updating) {        /* adjust the line number */
            if (*linenoP == '9')    /* if 9 then roll over to 0 */
                *linenoP = '0';
            else {
                if (*linenoP == ' ')    /* new digit */
                    *linenoP = '1';
                else            /* just increment */
                    ++*linenoP;
                updating = false;
            }
            linenoP--;
        }

        if (ShowLine() || ! BlankAsmErrCode())
            PrintLine();
    }

    if (skipRuntimeError) {
        outP++;
        Flushout();
        Exit(1);
    }

    if (! isControlLine)
    {
        ii = 2;
        if (tokenIdx < 2 || inDB || inDW)
            ii = 0;

        w6BCE = tokStart[ii] + tokenSize[ii];
        if (IsSkipping() || !isInstr)
            w6BCE = lineBuf;

        if (ChkGenObj())
            Ovl8();
        b6B2C = true;
        segLocation[activeSeg] = effectiveAddr.w = (word)(segLocation[activeSeg] + (w6BCE - lineBuf));
    }

    if (controls.xref && haveUserSymbol)
        if (phase == 1)
            EmitXref(XREF_REF, name);

    FlushM();

    while (tokenIdx > 0) {
        PopToken();
    }

    InitLine();
    if (b6B33)
    {
        finished = true;
        if (IsPhase2Print() && controls.symbols)
            Sub7041_8447();

        EmitXref(XREF_FIN, name);    /* finalise xref file */
        if (ChkGenObj())
            ReinitFixupRecs();
    }
}