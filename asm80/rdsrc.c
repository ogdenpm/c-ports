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
    '\n' with '\r\n'.
    This will allow the assembler to handle Linux/Unix created source
*/

static char *getLine() {
    int c;
    
    int i = 0;


    while ((c = getc(srcfp)) != EOF && c != '\n') {
            if (c != '\r' && i++ < MAXLINE)
                inBuf[i - 1] = c;
    }
    if (c == EOF) {
        if (ferror(srcfp))
            IoError(files[fileIdx].name, "Read error");
        if (i == 0)
                return NULL;
        fprintf(stderr, "Warning: unterminated line at end of %s\n", files[fileIdx].name);
    }
    if (i >= MAXLINE) {
            fprintf(stderr, "Warning: line truncated\n");
            strcpy(inBuf + MAXLINE - 3, "...\r\n");
    } else
            strcpy(inBuf + i, "\r\n");
    return inBuf;
}


byte GetSrcCh(void) /* get next source character */
{
    if (!inPtr || !*inPtr) {
        while (!(inPtr = getLine()))
            CloseSrc(); // unnest file
    }     
    return *inPtr++ & 0x7f;
}

void OpenSrc(void) {
    pendingInclude = false;
    files[fileIdx].fp = srcfp = SafeOpen(files[fileIdx].name, "rt"); /* Open() the file */
}
