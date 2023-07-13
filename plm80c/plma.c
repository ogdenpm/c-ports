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

static char noMemMsg[] = "NOT ENOUGH MEMORY FOR A COMPILATION";
static char aIxi[] = ".IXI";
static char aObj[] = ".OBJ";
static char aLst[] = ".LST";
static char aInvocationComm[] = "INVOCATION COMMAND DOES NOT END WITH <CR><LF>";
static char aIncorrectDevice[] = "INCORRECT DEVICE SPEC";
static char aSourceFileNotDisk[] = "SOURCE FILE NOT A DISKETTE FILE";
static char aSourceFileName[] = "SOURCE FILE NAME INCORRECT";
static char aSourceFileBadExt[] = "SOURCE FILE EXTENSION INCORRECT";
static char aIllegalCommand[] = "ILLEGAL COMMAND TAIL SYNTAX";

static byte  ioBuffer[128];     // only 128 bytes read so 2048 was overkill
char *cmdTextP;


static void SkipSpace()
{
    while (*cmdTextP == ' ' || *cmdTextP == '\r') {
        cmdTextP++;
        }
} /* SkipSpace() */

#if 0
static bool TestToken(char *str, byte len)
{
    char *p;

    p = cmdTextP;
    while (len-- != 0) {
        if ((*cmdTextP++ & 0x5F) != *str++) {
            cmdTextP = p;
            return false;
        }
    }
    return true;
} /* TestToken() */
#endif


static void SkipAlphaNum()
{
    while (('A' <= *cmdTextP && *cmdTextP <= 'Z') || ('a' <= *cmdTextP && *cmdTextP <= 'z')
                || ('0' <= *cmdTextP && *cmdTextP <= '9'))
        cmdTextP++;
} /* SkipAlphaNum() */



    
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
    InitF(&conFile, "CONSOL", "stdin");
    conFile.fp = stdin;
    topMem = MemCk() - 12;
    if (topMem < 0xC000)
        Fatal(noMemMsg, Length(noMemMsg));
    puts("\nISIS-II PL/M-80 COMPILER " VERSION);
    cmdTextP = cmdP;
    blkSize1 = topMem - blkSize1 - 256;
    blkSize2 = topMem - blkSize2 - 256;
    ParseSrcFileName();
    InitFilesAndDefaults();
} /* SignOnAndGetSourceName() */
