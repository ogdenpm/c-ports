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

void LstModuleInfo()
{
	word p, q, r;
	p = 0;
	for (q = 1; q <= procCnt; q++) {
		curInfoP = botInfo + procInfo[q];
		r = GetBaseVal();
		if (r > p )
			p = r;
	}
	SetSkipLst(3);
	Xputstr2cLst("MODULE INFORMATION:", 0);
	NewLineLst();
	SetSkipLst(1);
	TabLst(5);
	Xputstr2cLst("CODE AREA SIZE     = ", 0);
	XnumLst(csegSize, 0xFC, 0xF0);
	TabLst(2);
	XnumLst(csegSize, 5, 0xF6);
	NewLineLst();
	TabLst(5);
	Xputstr2cLst("VARIABLE AREA SIZE = ", 0);
	XnumLst(dsegSize, 0xFC, 0xF0);
	TabLst(2);
	XnumLst(dsegSize, 5, 0xF6);
	NewLineLst();
	TabLst(5);
	Xputstr2cLst("MAXIMUM STACK SIZE = ", 0);
	XnumLst(p, 0xFC, 0xF0);
	TabLst(2);
	XnumLst(p, 5, 0xF6);
	NewLineLst();
	TabLst(5);
	XnumLst(linesRead, 0, 0xA);
	Xputstr2cLst(" LINES READ", 0);
	NewLineLst();
	TabLst(5);
	XnumLst(programErrCnt, 0, 0xA);
	Xputstr2cLst(" PROGRAM ERROR", 0);
	if (programErrCnt != 1)
		Xputstr2cLst("S", 0);

	NewLineLst();
	SetSkipLst(1);
	Xputstr2cLst("END OF PL/M-80 COMPILATION", 0);
	NewLineLst();
	FlushLstBuf();
	CloseF(&lstFil);
	lfOpen = false;
}
