/****************************************************************************
 *  plma.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include <ctype.h>
#include "os.h"
extern char *cmdP;      // command line current position


static char aIllegalCommand[] = "ILLEGAL COMMAND TAIL SYNTAX";

char *cmdTextP;


static void SkipSpace()
{
    while (*cmdTextP == ' ' || *cmdTextP == '\r') {
        cmdTextP++;
        }
} /* SkipSpace() */

    
static void ParseSrcFileName()
{
    char *fileName;
    word nameLen;

    while (*cmdTextP != ' ' && *cmdTextP != '\r')   // skip invoke name
        cmdTextP++;
    SkipSpace();
    char *endChrs = " \r\n";
    if (*cmdTextP == '\'') {
        endChrs = "\'\r\n";
        cmdTextP++;
    }
    fileName = cmdTextP;
    cmdTextP            = strpbrk(fileName, endChrs);
    if (!cmdTextP)
        cmdTextP = strchr(fileName, '\0');
    nameLen = (word)(cmdTextP - fileName);
    if (*endChrs == '\'') {
        if (*cmdTextP != '\'')
            Fatal("Missing quote at end of file name", (byte)strlen("Missing quote at end of file name"));
        cmdTextP++;
        nameLen--;
    }
    if (nameLen == 0)
        Fatal("No source file", (byte)strlen("No source file"));
    char *fNam           = xmalloc(nameLen + 1);
    memcpy(fNam, fileName, nameLen);
    fNam[nameLen]        = '\0';
    srcFileTable[0].fNam = fNam;
    SkipSpace();
    if (*cmdTextP == '$')
        Fatal(aIllegalCommand, Length(aIllegalCommand));
    moreCmdLine = *cmdTextP != '\n';
} /* ParseSrcFileName() */

static void InitFilesAndDefaults()
{
    LEFTMARGIN = 1;
    char const *src  = srcFileTable[0].fNam;
    char *s    = (char *)basename(src);
    char *t    = strrchr(s, '.');
    if (!t || t == s)
        t = strchr(s, '\0');
    int stemLen = (int)(t - src);
    bool uc     = true;
    for (char *r = s; *r && uc; r++)
        if (islower(*r))
            uc = false;

    ixiFileName  = xmalloc(stemLen + 5);    //allow for .xxx\0
    memcpy(ixiFileName, src, stemLen);
    strcpy(ixiFileName + stemLen, uc ? ".IXI" : ".ixi");

    lstFileName = xmalloc(stemLen + 5); // allow for .xxx\0
    memcpy(lstFileName, src, stemLen);
    strcpy(lstFileName + stemLen, uc ? ".LST" : ".lst");

    objFileName = xmalloc(stemLen + 5); // allow for .xxx\0
    memcpy(objFileName, src, stemLen);
    strcpy(objFileName + stemLen, uc ? ".OBJ" : ".obj");


    IXREF = false;
  
    PRINT = true;

    XREF = false;
    SYMBOLS = false;
    DEBUG = false;
    PAGING = true;
    OBJECT = true;

    OPTIMIZE = true;
    SetDate(" ", 1);
    SetPageLen(57);
    SetMarkerInfo(20, '-', 21);
    SetPageNo(0);
    SetMarginAndTabW(0xFF, 4);
    SetTitle(" ", 1);
    SetPageWidth(120);
} /* InitFilesAndDefaults() */

void SignOnAndGetSourceName()
{
    puts("\nISIS-II PL/M-80 COMPILER " VERSION);
    cmdTextP = cmdP;
    ParseSrcFileName();
    InitFilesAndDefaults();
} /* SignOnAndGetSourceName() */
