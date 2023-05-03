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
//static byte padb6C23;
byte fileIdx = {0};
char *endInBufP = inBuf;
bool missingEnd = false;
word srcfd;
word rootfd;
char *inChP = inBuf - 1;
char *startLineP = inBuf;
byte lineChCnt = 0; 
file_t files[6];
static word seekIBlk;
static word seekIByte;
//static byte pad6CAD;
static char *savInBufP;
static char *savEndInBufP;
//static word pad6CB2[4];
static word readFActual;
//static word pad6CBC;


void ReadF(byte conn, char *buffP, word count)
{
    Read(conn, buffP, count, &readFActual, &statusIO);
    IoErrChk();
}

void SeekI(byte seekOp)
{
    Seek(srcfd, seekOp, &seekIBlk, &seekIByte, &statusIO);
    IoErrChk();
}


void ReadSrc(char *bufLoc)
{
 //   byte pad;

    ReadF((byte)srcfd, bufLoc, (word)(&inBuf[sizeInBuf] - bufLoc));
    endInBufP = bufLoc + readFActual;
}



void CloseSrc(void) /* close current source file. Revert to any parent file */
{
    Close(srcfd, &statusIO);
    IoErrChk();
    if (fileIdx == 0) {		/* if it the original file we had no end statement so error */
        missingEnd = true;
        IoError(files[0].name);
        return;
    }
    fileIdx--;
    /* Open() the previous file */
    if (fileIdx == 0)		/* original source is kept open across include files */
        srcfd = rootfd;
    else
        srcfd = SafeOpen(files[fileIdx].name, READ_MODE);

    seekIByte = files[fileIdx].byt;    /* move to saved location */
    seekIBlk = files[fileIdx].blk;
    SeekI(SEEKABS);
    endInBufP = inBuf;        /* force Read() */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
    inChP = inBuf - 1;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}


byte GetSrcCh(void) /* get next source character */
{
    char *insertPt;
    while (1) {
        inChP++;

        if (inChP == endInBufP) {   /* buffer all used */
            savInBufP = startLineP;
            savEndInBufP = endInBufP;
            /* copy the current line down to start of buffer */
            if (savEndInBufP - savInBufP > 0)
                memcpy(inBuf, startLineP, savEndInBufP - savInBufP);
            startLineP = inBuf;
            /* Read() in  characters to rest of inBuf */
            ReadSrc(insertPt = startLineP + (savEndInBufP - savInBufP));
            inChP = insertPt;
        }

        if (readFActual == 0) {		/* end of file so close this one*/
            CloseSrc();
            continue;
        }
        break;
    }

    lineChCnt++;			// track chars on this line
    return *inChP & 0x7F;	// remove parity
}


void OpenSrc(void) {
    byte curByteLoc;
    word curBlkLoc;

    pendingInclude = false;
    SeekI(SEEKTELL);
    if (seekIByte == 128) {        /* adjust for 128 boundary */
        seekIBlk++;
        seekIByte = 0;
    }

    curBlkLoc = (word)(endInBufP - startLineP);    /* un-used characters */
//x:                        /* forces code alignment */
    if ((curByteLoc = curBlkLoc % 128) > seekIByte) {
        seekIByte += 128;	    /* adjust to allow for un-used chars */
        seekIBlk--;
    }
    /* save the current file location */
    files[fileIdx - 1].byt = seekIByte - curByteLoc;
    files[fileIdx - 1].blk = seekIBlk - curBlkLoc / 128;
    if (srcfd != rootfd)        /* close if include file */
    {
        Close(srcfd, &statusIO);
        IoErrChk();
    }

    endInBufP = inBuf;            /* force Read() */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
    inChP = endInBufP - 1;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    startLineP = inBuf;
    files[fileIdx].blk = 0;            /* record at start of file */
    files[fileIdx].byt = 0;    
    srcfd = SafeOpen(files[fileIdx].name, READ_MODE);    /* Open() the file */
}
