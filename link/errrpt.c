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
		Write(0," ", 1, &status);
		Write(0, file , (word)(ScanBlank(file) - file), &status);
		Write(0, ",", 1, &status);
		ReportError(errCode);
		if (errExit )
			Exit(1);
	}
} /* FileError() */

