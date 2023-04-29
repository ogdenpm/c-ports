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

/* packToken - packs the token pointed by tokStart, with length toksiz into 4 bytes
 packed version replaces original and toksize set to 4 bytes
*/
static byte Pack1(byte i)
{
	return  i >= tokenSize[0] ? 0 : tokStart[0][i] < 0x3f ? tokStart[0][i] - 0x2f : tokStart[0][i] - 0x34;
}

void PackToken(void) {
	*(wpointer)tokPtr = (Pack1(0) * 40 + Pack1(1)) * 40 + Pack1(2);
	*(wpointer)(tokPtr + 2) = (Pack1(3) * 40 + Pack1(4)) * 40 + Pack1(5);
	tokenSize[0] = 4;
}
