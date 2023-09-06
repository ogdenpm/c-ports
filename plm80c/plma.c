/****************************************************************************
 *  plma.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"
#include <ctype.h>
extern char *cmdP; // command line current position

char *cmdTextP;

static void InitFilesAndDefaults() {
    LEFTMARGIN      = 1;
    char const *src = srcFileTable[0].fNam;
    char *s         = (char *)basename(src);
    char *t         = strrchr(s, '.');
    if (!t || t == s)
        t = strchr(s, '\0');
    int prefixLen = (int)(t - s);
    bool uc       = true;
    for (char *r = s; *r && uc; r++)
        if (islower(*r))
            uc = false;

    ixiFileName = xmalloc(prefixLen + 5); // allow for .xxx\0
    memcpy(ixiFileName, s, prefixLen);
    strcpy(ixiFileName + prefixLen, uc ? ".IXI" : ".ixi");

    lstFileName = xmalloc(prefixLen + 5); // allow for .xxx\0
    memcpy(lstFileName, s, prefixLen);
    strcpy(lstFileName + prefixLen, uc ? ".LST" : ".lst");

    objFileName = xmalloc(prefixLen + 5); // allow for .xxx\0
    memcpy(objFileName, s, prefixLen);
    strcpy(objFileName + prefixLen, uc ? ".OBJ" : ".obj");

    depFileName = xmalloc(prefixLen + 9); // .deps/ {prefix} .d\0;
    strcpy(depFileName, ".deps/");
    memcpy(depFileName + 6, s, prefixLen);
    strcpy(depFileName + 6 + prefixLen, ".d");

    IXREF    = false;

    PRINT    = true;

    XREF     = false;
    SYMBOLS  = false;
    DEBUG    = false;
    PAGING   = true;
    OBJECT   = true;
    DEPEND   = false;

    OPTIMIZE = true;

    SetMarkerInfo(20, 21);
    SetPageNo(0);
} /* InitFilesAndDefaults() */

void SignOnAndGetSourceName() {
    puts("\nISIS-II PL/M-80 COMPILER " VERSION);
    GetNSToken();
    srcFileTable[0].fNam = GetNSToken();
    if (!*srcFileTable[0].fNam)
        FatalCmdLineErr("No file to compile");
    cmdTextP    = cmdP;
    moreCmdLine = *cmdTextP != '\n';
    InitFilesAndDefaults();
} /* SignOnAndGetSourceName() */
