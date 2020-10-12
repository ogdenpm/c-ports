/****************************************************************************
 *  oldplm80: Old C++ port of PLM80 v4.0                                    *
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


// $Id: trace.hpp,v 1.1 2003/10/04 21:08:48 Mark Ogden Exp $
extern const char *tx2Names[];
void traceTx1(void *buf, word len);
extern byte tx1ItemLengths[];
void traceRdTx1(byte tx1type, byte *buf, word len);
void traceTx2(word cnt, byte *buf, byte len);

void dumpInfo(info_pt p);
void dumpSym(symbol_pt p);
void dumpTx2Q(int n);
void dumpTx2QDetail(int n);
void dumpMem(char *file, word start, word end);

