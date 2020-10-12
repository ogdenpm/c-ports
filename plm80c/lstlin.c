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

static byte tmp[] = "     1";
static word lastNo = 1;

void LstLineNo()
{
    pointer q;
    bool i;

    if (stmtCnt > lastNo + 20 )
    {
        lastNo = Num2Asc(stmtCnt, 6, 10, tmp);
        lastNo = stmtCnt;
    }
    else
    while (lastNo < stmtCnt) {
        q = &tmp[5];
        i = true;
        while (i) {
            if (*q == '9' )
                *q = '0';
            else
            {
                if (*q == ' ' )
                    *q = '1';
                else
                    ++*q;
                i = false;
            }
            q = q - 1;
        }
        lastNo = lastNo + 1;
    }
    XwrnstrLst(&tmp[2], 4);
}
