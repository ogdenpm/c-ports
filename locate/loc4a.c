/****************************************************************************
 *  loc4a.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "loc.h"

void ErrChkReport(word errCode, char *file, bool errExit)
{
	word status;
	
	if (errCode != 0 )
	{
		file = SkipSpc((char *)file);	// safe to remove const
		Write(0, " ", 1, &status);
		Write(0, file, (word)(PastAFN(file) - file), &status);	/* Write() file name */
		Write(0, ",", 1, &status);
		Errmsg(errCode);
		if (errExit )
			Exit(1);
	}
} /* ErrChkReport */
