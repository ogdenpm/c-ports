/****************************************************************************
 *  loc1.c: part of the C port of Intel's ISIS-II locate                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
    vim:ts=4:shiftwidth=4:expandtab:
*/
#include "loc.h"
#include <stdarg.h>

// static byte copyright[] = "(C) 1976, 1977, 1979 INTEL CORP";
#define VERSION "V3.0"

char const help[] =
    "Usage: %s inputFile [TO targetFile] [locate option]*\n"
    "Where locate options are:\n"
    "segmentName(address)   Set the start address for a specific segment\n"
    "COLUMNS(num)           Number of columns in symbol table (1-3)\n"
    "LINES                  Show line information in listing\n"
    "MAP                    Show memory map in listing\n"
    "NAME(name)             Override main module name\n"
    "OVERLAPOK              Allow overlaps\n"
    "EXTERNOK               Allow unresolved externals\n"
    "ORDER(segment_list)    Override the default segment ordering\n"
    "PRINT(file)            Use file rather than stdout for listing\n"
    "PUBLICS                Show public symbols in listing\n"
    "PURGE                  Purge symbols and debug information\n"
    "RESTART0               Add jmp instruction at address 0 to start of code\n"
    "STACKSIZE(size)        Override stack size\n"
    "START(addr)            Override main module start address\n"
    "SYMBOLS                Show local symbols in listing\n"
    "See Intel locate documentation for more details\n"
    "Notes:\n"
    "* For ORDER, space or comma can be used as a separator between segment names\n"
    "* File names are of the format [:Fx:]path, where x is a digit and path\n"
    "  The :Fx: maps to a directory prefix from the same named environment variable\n"
    "* Response file input for locating is supported by using \"%s <file\"\n"
    "* targetFile is deleted on error, which helps with make builds\n";

uint16_t columns = 1;
seen_t seen; /* START, STACK, RESTART0, MAP, PUBLICS, SYMBOLS, LINES, PURGE, NAME */
pstr_t const *moduleName;
bool isMain;
uint8_t tranId;
uint8_t tranVn;
uint16_t startAddr;
uint8_t segOrder[255];
uint16_t segBases[256]; /* abs, code, data */
uint16_t segSizes[256];
uint8_t segFlags[256];
dataFrag_t inBlock;

uint8_t image[0x10000];

/* convert a target address into its current location in cache */
uint8_t *AddrInMem(uint16_t addr) {
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
void AddSegFrag(uint8_t flags, uint8_t seg, uint16_t start, uint16_t len) {
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
        segFrags = safeRealloc(segFrags, (segFragSize += SFCHUNK) * sizeof(segFrag_t));
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

void AddDataFrag(uint16_t saddr, uint16_t eaddr) {
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
        dataFrags = safeRealloc(dataFrags, (dataFragSize += DFCHUNK) * sizeof(dataFrag_t));
    memmove(&dataFrags[i + 1], &dataFrags[i], (dataFragCnt++ - i) * sizeof(dataFrag_t));
    dataFrags[i].saddr = saddr;
    dataFrags[i].eaddr = eaddr;
}

void LoadModdat(uint8_t segId) {
    (void)segId; // suppress unused warning
    /* when called the segId and offset have already been read */
    memcpy(AddrInMem(inBlock.saddr), inP, recLen - 4);
}

void Start(void) {
    ProcArgsInit();
    LocateFile();
    Exit(warnings != 0);
}
