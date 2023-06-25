/****************************************************************************
 *  loc1.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

/*
    vim:ts=4:shiftwidth=4:expandtab:
*/
#include "loc.h"
#include <stdarg.h>
#include <stdlib.h>

// static byte copyright[] = "(C) 1976, 1977, 1979 INTEL CORP";
#define VERSION "V3.0"

FILE *inFp, *outFp, *lstFp, *tmpFp;
char *inName, *outName;
char *lstName;
word columns = 1;
seen_t seen; /* START, STACK, RESTART0, MAP, PUBLICS, SYMBOLS, LINES, PURGE, NAME */
pstr_t const *moduleName;
bool isMain;
byte tranId;
byte tranVn;
word startAddr;
byte segOrder[255];
word segBases[256]; /* abs, code, data */
word segSizes[256];
byte segFlags[256];
pointer inRec;
pointer inEnd;
pointer inP;
byte inType;
word recNum;
word recLen;
int maxRecLen;
byte outRec[1060]; // sized to allow for unchecked name write at end
pointer outP;
dataFrag_t inBlock;

byte image[0x10000];

// helper functions to allow portability
word getWord(byte *buf) {
    return buf[0] + buf[1] * 256;
}

void IllegalRecord() {
    RecError("unknown record type");
}

void IllegalReloc() {
    RecError("illegal record format");
}

void BadRecordSeq() {
    RecError("unexpected record");
}

void pstrcpy(pstr_t const *psrc, pstr_t *pdst) { /* copy pascal style string */
    memcpy(pdst, psrc, psrc->len + 1);
}

void Printf(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(lstFp, fmt, args);
    va_end(args);
}

void Prints(char const *s) {
    fputs(s, lstFp);
}

void PrintfAndLog(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(lstFp, fmt, args);
    va_end(args);

    if (echoToStderr) {
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
    }
}

pstr_t *ReadName() {
    pstr_t *pname = (pstr_t *)inP;
    if (inP >= inEnd || (inP += pname->len + 1) > inEnd)
        IllegalReloc();
    return pname; /* read a name */
} /* ReadName() */

/* read in 4 byte block num and byte num inP points to the data, return a uint32_t offset */
uint32_t ReadLocation() {
    uint32_t offset = ReadWord() * 128;
    return offset + ReadWord();
}
uint16_t ReadWord() {
    if ((inP += 2) > inEnd)
        IllegalReloc();
    return getWord(inP - 2);
}

uint8_t ReadByte() {
    if (inP >= inEnd)
        IllegalReloc();
    return *inP++;
}

void GetRecord() {
    if (!inRec)
        inRec = xmalloc(maxRecLen = 2048);
    if (fread(inRec, 1, 3, inFp) != 3)
        FatalError("%s: Premature EOF", inName);
    recLen = getWord(inRec + REC_LEN);
    if (maxRecLen < recLen + 3) {
        while (maxRecLen <= recLen + REC_DATA)
            maxRecLen += 4096;
        inRec = xrealloc(inRec, maxRecLen);
    }
    inP   = inRec + REC_DATA;
    inEnd = inRec + recLen + 2; // exclude CRC
    if (fread(inP, 1, recLen, inFp) != recLen)
        FatalError("%s: Premature EOF", inName);
    recNum++;

    byte inCRC = 0;
    for (int i = 0; i < recLen + REC_DATA; i++)
        inCRC += inRec[i];
    if (inCRC)
        RecError("Checksum error");
    inType = inRec[0];
    if (inType == 0 || inType > R_COMDEF || (inType & 1)) /* other invalid caught elsewhere */
        IllegalReloc();
    /* check for special case handling */
    if (recLen > 1025 && inType != R_MODDAT && (inType < R_LIBNAM || inType == R_COMDEF))
        RecError("Record length > 1025");
}

void Rewind() {

    fseek(inFp, 0, SEEK_SET);
    IoError(inName, "Rewind error");

    recNum = 0; /* reset vars */
}

/* convert a target address into its current location in cache */
pointer AddrInMem(word addr) {
    return image + addr;
}

segFrag_t *segFrags;
int segFragCnt;
int segFragSize;
#define SFCHUNK 256
/*
    Create a list of segment memory usage
    For abs segments this merges blocks
    but assumes no overlap!!
*/
void AddSegFrag(byte flags, byte seg, word start, word len) {
    int i;
    for (i = 0; i < segFragCnt; i++) {
        if (segFrags[i].start > start) {
            if (start + len == segFrags[i].start && segFrags[i].seg == seg) {
                // merge
                segFrags[i].start = start;
                segFrags[i].len += len;
                return;
            } else
                break; // insert new one
        } else if (segFrags[i].seg == seg && segFrags[i].start + segFrags[i].len == start) {
            segFrags[i].len += len;
            /* check for merge with next fragment if one exists */
            if (++i < segFragCnt && segFrags[i].start == start + len && segFrags[i].seg == seg) {
                segFrags[i - 1].len += segFrags[i].len;
                memcpy(&segFrags[i], &segFrags[i + 1], (--segFragCnt - i) * sizeof(segFrag_t));
            }
            return;
        }
    }
    if (segFragCnt >= segFragSize)
        segFrags = xrealloc(segFrags, (segFragSize += SFCHUNK) * sizeof(segFrag_t));
    memmove(&segFrags[i + 1], &segFrags[i], (segFragCnt++ - i) * sizeof(segFrag_t));
    segFrags[i].flags = flags; /* set all of the values */
    segFrags[i].seg   = seg;
    segFrags[i].start = start;
    segFrags[i].len   = len;
}

dataFrag_t *dataFrags;
int dataFragCnt;
int dataFragSize;
#define DFCHUNK 256

void AddDataFrag(word saddr, word eaddr) {
    int i;
    for (i = 0; i < dataFragCnt; i++) {
        if (dataFrags[i].saddr > saddr) {          // higher block found
            if (eaddr >= dataFrags[i].saddr - 1) { // join or overlap
                dataFrags[i].saddr = saddr;        // new start
                if (eaddr > dataFrags[i].eaddr) {  // overlap completely
                    dataFrags[i].eaddr = eaddr;
                    /* check for overlap with later blocks */
                    // original code assumed eaddr + 1 == start of next block
                    // now prevents data loss when it is mid next block
                    while (i < dataFragCnt - 1 && eaddr + 1 >= dataFrags[i + 1].saddr) {
                        dataFrags[i].eaddr = dataFrags[i + 1].eaddr;
                        memcpy(&dataFrags[i + 1], &dataFrags[i + 2],
                               (--dataFragCnt - i) * sizeof(dataFrag_t));
                    }
                }
                return;
            } else
                break;
        } else if (dataFrags[i].saddr == saddr || saddr - 1 <= dataFrags[i].eaddr) {
            if (dataFrags[i].eaddr < eaddr) { // handle when not full overlap
                dataFrags[i].eaddr = eaddr;
                /* check for overlap of later blocks */
                // now handles case where eaddr is mid next block
                while (i < dataFragCnt - 1 && eaddr + 1 >= dataFrags[i + 1].saddr) {
                    if (eaddr < dataFrags[i + 1].eaddr) // update new end
                        dataFrags[i].eaddr = dataFrags[i + 1].eaddr;
                    memcpy(&dataFrags[i + 1], &dataFrags[i + 2],
                           (--dataFragCnt - i) * sizeof(dataFrag_t));
                }
            }
            return;
        }
    }
    if (dataFragCnt >= dataFragSize)
        dataFrags = xrealloc(dataFrags, (dataFragSize += DFCHUNK) * sizeof(dataFrag_t));
    memmove(&dataFrags[i + 1], &dataFrags[i], (dataFragCnt++ - i) * sizeof(dataFrag_t));
    dataFrags[i].saddr = saddr;
    dataFrags[i].eaddr = eaddr;
}

void LoadModdat(byte segId) {
    /* when called the segId and offset have already been read */
    memcpy(AddrInMem(inBlock.saddr), inP, recLen - 4);
}

void Start() {
    ProcArgsInit();
    LocateFile();
    Exit(warnings != 0);
}

_Noreturn void usage() {
    printf("Usage: %s inputFile [(TO|-o) targetFile] [locate option]*\n"
           "or:    %s (-h | -v | -V)\n",
           invokeName, invokeName);
    printf("Where:\n"
           "-h                          Show this help\n"
           "-v / -V                     Show simple / extended version information\n"
           "Locate options are:\n"
           "segmentName(address)        Set the start address for a specific segment\n"
           "COLUMNS(num)    or -c num   Number of columns in symbol table (1-3)\n"
           "LINES           or -ll      Show line information in listing\n"
           "MAP             or -lm      Show memory map in listing\n"
           "NAME(name)      or -n name  Override main module name\n"
           "NOOVERLAP       or -no      Treat overlaps as errors\n"
           "NOEXTERN        or -ne      Treat unresolved externals as errors\n"
           "ORDER(segment order)        Override the default segment ordering\n"
           "PRINT(file)     or -p file  Use file rather than stdout for listing\n"
           "PUBLICS         or -lp      Show public symbols in listing\n"
           "PURGE           or -pu      Purge symbols and debug information\n"
           "RESTART0        or -r       Add jmp instruction at address 0 to start of code\n"
           "STACKSIZE(size) or -ss size Override stack size\n"
           "START(addr )    or -s addr  Override main module start address\n"
           "SYMBOLS         or -ls      Show local symbols in listing\n"
           "See Intel locate documentation for more details\n"
           "Notes:\n"
           "* File names are of the format [:Fx:]path, where x is a digit and path\n"
           "  can contain directory components but not spaces, commas, ampersand or parenthesis.\n"
           "  The :Fx: maps to a directory prefix from the same named environment variable\n"
           "  It can be used to work around directory character limitations\n"
           "* Response file input for linking is supported by using \"%s <file\"\n"
           "* targetFile is deleted on error, which helps with make builds\n",
           invokeName);
    exit(0);
}
