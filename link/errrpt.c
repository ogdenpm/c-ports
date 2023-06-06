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
#include <stdio.h>

void FileError(word errCode, char const *file, bool errExit) {
    if (errCode != 0) {
        file = Deblank(file);
        fprintf(stderr, " %.*s,", (int)(ScanBlank(file) - file), file);

        ReportError(errCode);
        if (errExit)
            Exit(1);
    }
} /* FileError() */
