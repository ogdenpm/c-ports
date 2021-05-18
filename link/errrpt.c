/****************************************************************************
 *  link: C port of Intel's LINK v3.0                                       *
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


#include "link.h"
void FileError(word errCode, pointer file, bool errExit)
{
	word status;

	if (errCode != 0 )
	{
		file = Deblank(file);
		Write(0," ", 1, &status);
		Write(0, file , (word)(ScanBlank(file) - file), &status);
		Write(0, ",", 1, &status);
		ReportError(errCode);
		if (errExit )
			Exit(1);
	}
} /* FileError() */

