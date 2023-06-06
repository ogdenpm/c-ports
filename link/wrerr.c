/****************************************************************************
 *  wrerr.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"
#include <stdio.h>

static struct {
    byte errCode;
    char *errStr;
} errStrTable[] = { { 0x2, "ILLEGAL AFTN ARGUMENT" },
                    { 0x3, "TOO MANY OPEN FILES" },
                    { 0x4, "INCORRECTLY SPECIFIED FILE" },
                    { 0x5, "UNRECOGNIZED DEVICE NAME" },
                    { 0x9, "DISK DIRECTORY FULL" },
                    { 0xC, "FILE IS ALREADY OPEN" },
                    { 0xD, "NO SUCH FILE" },
                    { 0xE, "WRITE PROTECTED" },
                    { 0x11, "NOT A DISK FILE" },
                    { 0x13, "ATTEMPTED SEEK ON NON-DISK FILE" },
                    { 0x14, "ATTEMPTED BACK SEEK TOO FAR" },
                    { 0x15, "CAN'T RESCAN" },
                    { 0x16, "ILLEGAL ACCESS MODE TO OPEN" },
                    { 0x17, "MISSING FILENAME" },
                    { 0x1B, "ILLEGAL SEEK COMMAND" },
                    { 0x1C, "MISSING EXTENSION" },
                    { 0x1F, "CAN'T SEEK ON WRITE ONLY FILE" },
                    { 0x20, "CAN'T DELETE OPEN FILE" },
                    { 0x22, "ILLEGAL LOAD COMMAND" },
                    { 0x23, "SEEK PAST EOF" },
                    { 0xCB, "INVALID SYNTAX" },
                    { 0xCC, "PREMATURE EOF" },
                    { 0xD0, "CHECKSUM ERROR" },
                    { 0xD2, "INSUFFICIENT MEMORY" },
                    { 0xD3, "RECORD TOO LONG" },
                    { 0xD4, "ILLEGAL RELO RECORD" },
                    { 0xD5, "FIXUP BOUNDS ERROR" },
                    { 0xDA, "ILLEGAL RECORD FORMAT" },
                    { 0xDB, "PHASE ERROR" },
                    { 0xDC, "NO EOF" },
                    { 0xDD, "SEGMENT TOO LARGE" },
                    { 0xE0, "BAD RECORD SEQUENCE" },
                    { 0xE1, "INVALID NAME" },
                    { 0xE2, "NAME TOO LONG" },
                    { 0xE3, "LEFT PARENTHESIS EXPECTED" },
                    { 0xE4, "RIGHT PARENTHESIS EXPECTED" },
                    { 0xE5, "UNRECOGNIZED CONTROL" },
                    { 0xE9, "'TO' EXPECTED" },
                    { 0xEA, "DUPLICATE FILE NAME" },
                    { 0xEB, "NOT A LIBRARY" },
                    { 0xEC, "TOO MANY COMMON SEGMENTS" },
                    { 0xEE, "ILLEGAL STACK CONTENT RECORD" },
                    { 0xEF, "NO MODULE HEADER RECORD" }};

void ReportError(word errCode) {
    if (Low(errCode) != 0) {
        for (int i = 0; i < sizeof(errStrTable) / sizeof(errStrTable[0]); i++) {
            if (errStrTable[i].errCode == Low(errCode)) {
                fprintf(stderr, " %s\n", errStrTable[i].errStr);
                return;
            }
        }
        Error(errCode); /* pass to ISIS */
    }
} /* ReportError() */
