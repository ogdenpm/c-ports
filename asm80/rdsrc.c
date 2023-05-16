/****************************************************************************
 *  rdsrc.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"

bool pendingInclude = false;
bool includeOnCmdLine = false;
byte fileIdx = 0;
FILE *srcfp;
byte lineChCnt = 0; 
file_t files[6];


void CloseSrc(void) /* close current source file. Revert to any parent file */
{
    if (fileIdx == 0) /* if it the original file we had no end statement so error */
        IoError(files[0].name, "EOF - missing 'end'");
    if (fclose(srcfp) == EOF)
        IoError(files[fileIdx].name, "Close Error");
    free(files[fileIdx].name);
    srcfp = files[--fileIdx].fp;
}

/*
    Until the main code is modified to use '\n' as the end of a line
    the code below squashes '\r' in the input stream and replaces
    '\n' with '\r\n'. Note a single '\r' is also treated as '\r\n'
    This will allow the assembler to handle Linux/Unix created source
*/
byte GetSrcCh(void) /* get next source character */
{
    int c;
    static bool retLF = false;

    if (retLF) {
        retLF = false;
        return '\n';
    }
    // get the next real character
    while ((c = getc(srcfp)) == EOF) {
        if (ferror(srcfp))
            IoError(files[fileIdx].name, "Read error");
        if (lineChCnt) {
            c = '\n';       // terminate the line
            break;
        }
        CloseSrc(); // un-nest file if needed
    }
    // as the source is now opened in text mode
    // for Windows a '\r' is mapped to '\n;
    // for Linux, it could be start of '\r\n'
    if (c == '\r') {
#ifndef _WIN32
        if ((c = getc(srcfp)) != '\n' && c != EOF)
            ungetc(c, srcfp);
#endif
        c = '\n';
    }
    if (c == '\n') {
        retLF = true;
        if (lineChCnt > MAXLINE)
            strcpy(inBuf + MAXLINE - 2, "..\r\n"); // indicate truncated
        else
            strcpy(inBuf + lineChCnt, "\r\n");
        lineChCnt += 2;
        return '\r';
    }
    if (lineChCnt++ < MAXLINE)
        inBuf[lineChCnt - 1] = c;
    return c & 0x7f;
}

void OpenSrc(void) {
    pendingInclude = false;
    files[fileIdx].fp = srcfp = SafeOpen(files[fileIdx].name, "rt"); /* Open() the file */
}
