/****************************************************************************
 *  fatlio.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include "os.h"

static struct {
    byte errCode;
    char const *errStr;
} errStrTable[] = { { 0x4, "ILLEGAL FILENAME SPECIFICATION" },
                    { 0x5, "ILLEGAL OR UNRECOGNIZED DEVICE SPECIFICATION IN FILENAME" },
                    { 0xC, "ATTEMPT TO OPEN AN ALREADY OPEN FILE" },
                    { 0xD, "NO SUCH FILE" },
                    { 0xE, "FILE IS WRITE PROTECTED" },
                    { 0x13, "FILE IS NOT ON A DISKETTE" },
                    { 0x16, "DEVICE TYPE NOT COMPATIBLE WITH INTENDED FILE USE" },
                    { 0x17, "FILENAME REQUIRED ON DISKETTE FILE" },
                    { 0x1C, "NULL FILE EXTENSION" },
                    { 0xFE, "ATTEMPT TO READ PAST EOF" } };

static char const *xFindErrStr(word errNum) {
    for (int i = 0; i < sizeof(errStrTable) / sizeof(errStrTable[0]); i++)
        if (errStrTable[i].errCode == errNum)
            return errStrTable[i].errStr;
    return NULL;
}

void FatlIO(file_t *fileP, word errNum) {

    printf("\n\nPL/M-80 I/O ERROR --\n  FILE: %s\n  NAME: %s\n ERROR: %d", fileP->sNam, fileP->fNam,
           errNum);
    char const *errStr = xFindErrStr(errNum);
    if (errStr)
        printf("--%s", errStr);
    puts("\nCOMPILATION TERMINATED\n");
    Exit(1);
}
