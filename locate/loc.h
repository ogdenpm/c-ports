/****************************************************************************
 *  loc.h: part of the C port of Intel's ISIS-II locate                     *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
 * vim:ts=4:shiftwidth=4:expandtab:
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#define stricmp strcasecmp
#endif

#include "../shared/lst.h"
#include "../shared/omf.h"
#include "../shared/os.h"
#include "../shared/cmdline.h"
#define VERSION "V3.0"
// ISIS uses MemCk to return the top of memory
// this is used to calculate a size for MEMORY seg
// defined a MEMCK value to use in calculation
#define MEMCK   0xf7ff

// accessor macros for HIGH and LOW
// word cast is to avoid 64bit warnings
#define High(n) ((uint16_t)((n) >> 8))
#define Low(n)  ((uint16_t)((n)&0xff))

typedef struct {
    uint16_t saddr;
    uint16_t eaddr;
} dataFrag_t;

typedef struct {
    uint8_t flags;
    uint8_t seg;
    uint16_t start;
    uint16_t len;
} segFrag_t;

typedef union {
    bool all[9];
    struct {
        bool start;
        bool stackSize;
        bool restart0;
        bool map;
        bool publics;
        bool symbols;
        bool lines;
        bool purge;
        bool name;
    };
} seen_t;

// types of column information
#define CTHEAD   0 // heading
#define CTPUB    1 // publics
#define CTSYM    2 // local symbols
#define CTLIN    3 // lines
#define CTMOD    4 // modules

/* Alignments  & Flags*/
#define AMASK    0xf
#define AABS     0
#define AUNKNOWN 0
#define AINPAGE  1
#define APAGE    2
#define ABYTE    3
#define FHASADDR 0x80
#define FWRAP0   0x40
#define FSEGSEEN 0x10

/* Fixup type */
#define FLOW     1
#define FHIGH    2
#define FBOTH    3

/* controls seen tags */
// #define seen.start   seen(0)
// #define seen.stackSize   seen(1)
// #define seen.restart0    seen(2)
// #define seen.map     seen(3)
// #define seen.publics seen(4)
// #define seen.symbols seen(5)
// #define seen.lines   seen(6)
// #define seen.purge   seen(7)
// #define seen.name  seen(8)

extern uint16_t columns;

extern dataFrag_t inBlock;

extern bool isMain;
extern uint8_t tranId;
extern uint8_t tranVn;
extern pstr_t const *moduleName;

extern FILE *lstFp;
extern char *lstName;
extern seen_t seen;
extern uint16_t segBases[256];
extern uint8_t segFlags[256];
extern uint8_t segOrder[255];
extern uint16_t segSizes[256];
extern uint16_t startAddr;
extern uint16_t unsatisfiedCnt;
extern uint16_t targetBase;
extern char const *segNames[];

extern dataFrag_t *dataFrags;
extern int dataFragCnt;
extern segFrag_t *segFrags;
extern int segFragCnt;
extern int warningMask;
extern int warnings;

/* loc1.c */

uint8_t *AddrInMem(uint16_t addr);
void AddSegFrag(uint8_t flags, uint8_t seg, uint16_t start, uint16_t len);
void AddDataFrag(uint16_t saddr, uint16_t eaddr);
void LoadModdat(uint8_t segId);

/* loc2b.c */
void EmitModDat(dataFrag_t *block);
void ForceSOL(void);
void PrintColumn(uint8_t ctype, ...);
uint8_t SetTargetSeg(uint8_t seg);
void ProcModend(void);
void ProcModdat(void);

/* loc3.c */
void ProcLinNum(void);
void ProcAncest(void);
void ProcDefs(uint8_t list, uint8_t ctype);
void ProcExtnam(void);
void PrintListingHeader(char const *heading);
void LocateFile(void);

/* loc4.c */
void PrintMemoryMap(void);

/* loc6.c */
void ExpectLP(void);
void ExpectRP(void);
void ExpectSlash(void);
void ResetSegOrder(void);
void AddSegOrder(uint8_t seg);
void FixSegOrder(void);
void ProcessControls(void);

/* loc7.c */
uint16_t ParseLPNumRP(void);
uint8_t GetCommonSegId(char *token);
void ProcArgsInit(void);
uint16_t AlignAddress(uint8_t align, uint16_t size, uint16_t laddr);
void ProcModhdr(void);
void AssignAddress(void);
void ProcComdef(void);
void ProcHdrAndComDef(void);
