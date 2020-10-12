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

void FindScopedInfo(word scope)
{
    word p, q;
    offset_t next;
    byte infoType;

    curInfoP = SymbolP(curSymbolP)->infoP;
    p = 0;
    while (curInfoP != 0) {
        if (scope == GetScope()) {
            infoType = GetType();
            if (infoType == LIT_T || infoType == MACRO_T)
                ;
            else if (TestInfoFlag(F_MEMBER))
                goto nxt;
            if (p != 0)	/* not at start of Chain() */
            {
                next = GetLinkOffset();	/* Move() to head of Chain() */
                q = curInfoP;		/* save current */
                curInfoP = p;		/* pick up previous */
                SetLinkOffset(next);	/* set its link */
                curInfoP = q;		/* restore current */
                SetLinkOffset(SymbolP(curSymbolP)->infoP);	/* set its link to current head */
                SymbolP(curSymbolP)->infoP = curInfoP;	/* set head to found info */
            }
            return;
        }
    nxt:	p = curInfoP;
        curInfoP = GetLinkOffset();
    }
}
