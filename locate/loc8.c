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

static byte devices[] = "F0F1F2F3F4F5TITO"
					    "VIVOI1O1TRHRR1R2"
		                "TPHPP1P2LPL1BBCI"
		                "COF6F7F8F9";

/* convert the spath info into a standard file name :xx:file.ext */
void MakeFullName(spath_t *pinfo, pointer pstr)
{
	byte i;

	/* put the :xx: */
	pstr[0] = ':';
	pstr[1] = devices[pinfo->deviceId + pinfo->deviceId];
	pstr[2] = devices[pinfo->deviceId + pinfo->deviceId + 1];
	pstr[3] = ':';
	pstr = pstr + 4;	/* past the :xx: inserted */
	/* put the file */
	for (i = 0; i <= 5; i++) {
		if ((*pstr = pinfo->name[i]) != 0 )
			pstr = pstr + 1;
	}
	/* only put .ext if (it exists */
	if (pinfo->ext[0] != 0 )
	{
		*pstr = '.';
		pstr = pstr + 1;
	}
	/* put the ext - could have been in the if (clause above */
	for (i = 0; i <= 2; i++) {
		if ((*pstr = pinfo->ext[i]) != 0 )
			pstr = pstr + 1;
	}
	*pstr = ' ';	/* space at end */
} /* MakeFullName */


void StrUpr(pointer pch)
{

	while (*pch != '\r') {
		*pch = ToUpper(*pch);
		pch = pch + 1;
	}
} /* StrUpr */

