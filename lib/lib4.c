/****************************************************************************
 *  lib: C port of Intel's LIB v2.1                                         *
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


#include "lib.h"

static byte hexdigits[] = "0123456789ABCDEF";

void Binasc(word num, byte base, byte padch, pointer chaP, byte width)
{
	byte i;

	for (i = 1; i <= width; i++) {
		chaP[width - i] = hexdigits[num % base];
		num /= base;
	}
	i = 0;
	while (chaP[i] == '0' && i < width - 1) {
		chaP[i] = padch;
		i = i + 1;
	}
} /* Binasc */


