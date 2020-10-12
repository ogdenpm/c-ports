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

static byte infoLengths[] = { 10, 12, 18, 18, 18, 22, 11, 10, 8, 9 };


offset_t AllocInfo(word infosize)
{
    offset_t p, q;

    Alloc(infosize, infosize);
    p = topInfo + 1;
    if (botSymbol < (q = topInfo + infosize))
        FatalError(ERR83);
    memset(ByteP(p), 0, infosize);
    topInfo = q;
    return p;
} /* AllocInfo() */


void CreateInfo(word scope, byte type)
{
    byte len;

    len = infoLengths[type];
    curInfoP = AllocInfo(len);
    if (curSymbolP != 0) {
        SetLinkOffset(SymbolP(curSymbolP)->infoP);
        SymbolP(curSymbolP)->infoP = curInfoP;
    }
    SetType(type);
    SetLen(len);
    SetScope(scope);
    SetSymbol(curSymbolP);
} /* CreateInfo() */
