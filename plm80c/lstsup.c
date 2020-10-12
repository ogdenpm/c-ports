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
/* common source for lstsp[456].plm */

void FlushLstBuf()
{
	if (lChCnt != 0) {
		if (! lfOpen) {
			OpenF(&lstFil, 2);
			lfOpen = true;
		}
		WriteF(&lstFil, lBufP, lChCnt);
		lChCnt = 0;
	}
}

void NewLineLst()
{
	if (col == 0)
		if (linLft == 0)
			NewPgl();
	Wr2cLst(0x0d0a);
	linLft--;
	col = 0;
}


void TabLst(byte tabTo)
{
	if (tabTo > 127) {
		tabTo = -tabTo;
		if (col >= tabTo)
			NewLineLst();
		tabTo -= col + 1;
	}
	while (tabTo-- != 0)
		PutLst(' ');

}


void NewPageNextChLst()
{
	linLft = 0;
}

void SetMarkerInfo(byte markerCol, byte marker, byte textCol)
{
	wrapMarkerCol = markerCol;
	wrapMarker = marker;
	wrapTextCol = textCol;
}


void SetStartAndTabW(byte startCol, byte width)
{
	margin = startCol - 1;
	tWidth = width;
}


void SetSkipLst(byte cnt)
{
	skipCnt = cnt;
}


void Xputstr2cLst(pointer str, byte endch)
{
	while (*str != endch)
		PutLst(*str++);
}

void XwrnstrLst(pointer str, byte cnt)
{
	while (cnt-- != 0)
		PutLst(*str++);
}


void XnumLst(word num, byte width, byte radix)
{
	byte i, buf[7];

	i = Num2Asc(num, width, radix, buf);
	XwrnstrLst(buf, i);
}
