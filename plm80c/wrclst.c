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

void WrcLst(byte ch)
{
    lBufP[lChCnt] = ch;
    if (lChCnt == lBufSz) {
        if (! lfOpen) {
            OpenF(&lstFil, 2);
            lfOpen = true;
        }
        WriteF(&lstFil, lBufP, lBufSz + 1);
        lChCnt = 0;
    } else
        lChCnt++;
}

void Wr2cLst(word arg1w)
{
    pointer bp;

    bp = (pointer)&arg1w;
    WrcLst(bp[1]);
    WrcLst(bp[0]);
}

void WrnStrLst(pointer str, word cnt)
{

    while (cnt-- != 0)
        WrcLst(*str++);
}
