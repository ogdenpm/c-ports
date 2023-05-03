/****************************************************************************
 *  loc8.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "loc.h"

static byte devices[] = "F0F1F2F3F4F5TITO"
					    "VIVOI1O1TRHRR1R2"
		                "TPHPP1P2LPL1BBCI"
		                "COF6F7F8F9";

/* convert the spath info into a standard file name :xx:file.ext */
void MakeFullName(spath_t *pinfo, char *pstr)
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


void StrUpr(char *pch)
{

	while (*pch != '\r') {
		*pch = ToUpper(*pch);
        pch++;
	}
} /* StrUpr */

