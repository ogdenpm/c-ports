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

void WriteRec(pointer recP, byte arg2b)
{
    word p;
    wpointer lenP;
    byte crc;
    word cnt;

    lenP = &((rec_t *)recP)->len;
    if (*lenP > 0 && OBJECT ) {
        crc = 0;
        p = 0;
        *lenP += arg2b + 1;
        cnt = *lenP + 2;
        while (p < cnt)
            crc -= recP[p++];

        recP[cnt] = crc;	/* insert checksum */
        Fwrite(&objFile, recP, cnt + 1);
    }
    *lenP = 0;
}



void RecAddByte(pointer recP, byte arg2b, byte arg3b)
{
    wpointer lenP;

    lenP = &((rec_t *)recP)->len;
    ((rec_t *)recP)->val[*lenP + arg2b] = arg3b;
    *lenP = *lenP + 1;
}



void RecAddWord(pointer arg1w, byte arg2b, word arg3w)
{
    RecAddByte(arg1w, arg2b, Low(arg3w));
    RecAddByte(arg1w, arg2b, High(arg3w));
}
