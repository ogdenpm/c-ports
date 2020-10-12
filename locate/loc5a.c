/****************************************************************************
 *  locate: C port of Intel's Locate v3.0                                   *
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


#include "loc.h"

static byte hexdigits[] = "0123456789ABCDEF";

void BinAsc(word number, byte base, byte pad, pointer bufP, byte ndigits)
{
	byte i;
	
	for (i = 1; i <= ndigits; i++) {
		bufP[ndigits - i] = hexdigits[number % base];
		number = number / base;
	}

	i = 0;
	while (bufP[i] == '0' && i < ndigits - 1) {
		bufP[i] = pad;
		i = i + 1;
	}
} /* BinAsc */


pointer PastAFN(pointer pch)
{
	while (1) {
		pch = PastFileName(pch);
		if (*pch == '*' || *pch == '?' )
			pch = pch + 1;
		else
			return pch;
	}
} /* PastAFN */


