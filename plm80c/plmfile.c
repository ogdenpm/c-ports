/****************************************************************************
 *  plmfile.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include "os.h"

void CloseF(file_t *fileP)
{

    if (fclose(fileP->fp))
        IoError(fileP->fNam, "Close error");
}

void InitF(file_t *fileP, char const *sNam, char const *fNam)
{
    fileP->fp = NULL;
    fileP->sNam = sNam;     // all instances are fixed so ok
    fileP->fNam  = fNam;     // all allocations are malloced

} /* InitF() */


void OpenF(file_t *fileP, char *access)
{

    if (!(fileP->fp = Fopen(fileP->fNam, access)))
        IoError(fileP->fNam, "Cannot %s", *access == 'w' ? "create" : "open");

}


