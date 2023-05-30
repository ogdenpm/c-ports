/****************************************************************************
 *  errrpt.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"
void FileError(word errCode, char const *file, bool errExit)
{
	word status;

	if (errCode != 0 )
	{
		file = Deblank(file);
		Write(CO_DEV," ", 1, &status);
		Write(CO_DEV, file , (word)(ScanBlank(file) - file), &status);
		Write(CO_DEV, ",", 1, &status);
		ReportError(errCode);
		if (errExit )
			Exit(1);
	}
} /* FileError() */

