/****************************************************************************
 *  rdsrc.c: part of the C port of Intel's ISIS-II asm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"
#include "os.h"

bool pendingInclude   = false;
bool includeOnCmdLine = false;
byte fileIdx          = 0;
FILE *srcfp;
file_t files[6];

void CloseSrc(void) /* close current source file. Revert to any parent file */
{
    if (fileIdx == 0) { /* if it the original file we had no end statement so error */
        fprintf(stderr, "%s: EOF - missing 'end'\n", files[0].name);
        exit(1);
    }
    if (fclose(srcfp) == EOF)
        IoError(files[fileIdx].name, "Close Error");
    srcfp = files[--fileIdx].fp;
}

/*
    Strip '\r' from source and handle unterminated last line of file
    This will allow the assembler to handle Linux/Unix created source
*/
char *inBuf;
char *inPtr;
int inBufSize;

static char *getLine() {
    int c;
    int i = 0;
    if (!inBuf)
        inBuf = safeRealloc(inBuf, inBufSize += 256);
    while ((c = getc(srcfp)) != '\n' && c != EOF) {
        if (c >= ' ' || c == '\t' ||
            c == '\f') {            // only allow tab or FF as a control char, others are stripped
            if (i >= inBufSize - 2) // allow room for "\n\0"
                inBuf = safeRealloc(inBuf, inBufSize += 256); // auto grow to allow very long lines
            inBuf[i++] = c;
        }
    }
    if (c == EOF) {
        if (ferror(srcfp))
            IoError(files[fileIdx].name, "Read error");
        if (i == 0)
            return NULL;
        fprintf(stderr, "Warning: Unterminated line at end of %s\n", files[fileIdx].name);
    }
    inBuf[i]     = '\n';
    inBuf[i + 1] = '\0';
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
    pendingInclude    = false;
    files[fileIdx].fp = srcfp = SafeOpen(files[fileIdx].name, "rt"); /* Open() the file */
}