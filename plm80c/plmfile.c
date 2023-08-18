/****************************************************************************
 *  plmfile.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"
#include <stdlib.h>

void CloseF(file_t *fileP) {
    if (fileP->fp) {
        if (fclose(fileP->fp))
            IoError(fileP->fNam, "Close error");
        fileP->fp = NULL;
    }
}
void OpenF(file_t *fileP, char const *sNam, char const *fNam, char *access) {
    fileP->sNam = sNam;
    fileP->fNam = fNam;
    if (!(fileP->fp = Fopen(fNam, access)))
        IoError(fNam, "Cannot %s", *access == 'w' ? "create" : "open");
}