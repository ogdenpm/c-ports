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

void NewPgl()
{
    byte i, j, k, m;
    byte pnum[3];

    if (! PAGING)
        return;
    WrcLst(0xc);
    linLft = PAGELEN;
    pageNo = pageNo + 1;
    i = Num2Asc(pageNo, 3, 10, pnum);
    j = PWIDTH - 41;
    if (j < TITLELEN)
        m = j;
    else
        m = TITLELEN;
    WrnStrLst(plm80Compiler, 20);
    WrnStrLst(TITLE, m);
    k = j - m + 2;
    while (k != 0) {
        WrcLst(' ');
        k = k - 1;
    }
    WrnStrLst(DATE, 9);
    WrnStrLst("  PAGE ", 7);
    WrnStrLst(pnum, 3);
    WrnStrLst("\r\n\n\n", 4);
    skipCnt = 0;
}


void NlLead()
{
    Wr2cLst(0xd0a);
    col = 0;
    linLft = linLft - 1;
    if (linLft == 0)
        NewPgl();
    while (col < wrapMarkerCol) {
        WrcLst(' ');
        col = col + 1;
    }
    if (wrapMarker != 0) {
        WrcLst(wrapMarker);
        col = col + 1;
    }
    while (col < wrapTextCol) {
        WrcLst(' ');
        col = col + 1;
    }
}
