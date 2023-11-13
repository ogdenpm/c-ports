/****************************************************************************
 *  link.h: part of the C port of Intel's ISIS-II link                      *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define DIRSEP  "/\\"
#define namecmp stricmp // case insensitive file name compare
#else
#include <errno.h>
#include <unistd.h>
#define _MAX_PATH 4096
#define O_BINARY  0
#define DIRSEP    "/"
#define namecmp   strcmp
#endif
#include "../shared/lst.h"
#include "../shared/omf.h"
#include "../shared/os.h"

#ifndef _MSC_VER
#define stricmp strcasecmp
#endif

#define High(n)    ((n) >> 8)
#define Low(n)     ((n)&0xff)
#define Shr(v, n)  ((uint16_t)(v) >> (n))
#define Shl(v, n)  ((uint16_t)(v) << (n))
#define RorB(v, n) ((uint8_t)(((v) | ((v) << 8)) >> (n)))
#define RolB(v, n)	((uint8_t)(Ror((v), (8 - (n))))
#define Move(s, d, c) memcpy(d, s, c)

typedef struct { // pascal string for module names
    uint8_t len;
    char str[31];
} psModName_t;

// typedef struct {
//     byte rectyp;
//     word reclen;
//     byte record[1];
// } record_t;

#define REC_TYPE 0
#define REC_LEN  1 // word
#define REC_DATA 3 // various

typedef struct _segfrag {
    struct _segfrag *next;
    uint16_t bot;
    uint16_t top;
} segFrag_t;

typedef struct _symbol {
    struct _symbol *hashChain;
    uint8_t flags;
    uint8_t seg;
    union {
        uint16_t offsetOrSym;
        uint16_t len;
        uint16_t symId;
    };
    struct _symbol *nxtSymbol;
    pstr_t const *name;
} symbol_t;

typedef struct _module {
    struct _module *next;
    uint32_t location;
    symbol_t *symbols;
    uint16_t cbias;
    uint16_t dbias;
    pstr_t const *name;
} module_t;

typedef struct _library {
    struct _library *next;
    module_t *modules;
    bool publics;
    bool isLib;
    char const *name;
} objFile_t;

#define SEGDEF_sizeof 4

typedef struct {
    uint8_t combine;
    uint16_t lenOrLinkedSeg;
} comseginfo_t;

/* Record types */
#define R_MODHDR    2
#define R_MODEND    4
#define R_MODDAT    6
#define R_LINNUM    8
#define R_MODEOF    0xE
#define R_ANCEST    0x10
#define R_LOCDEF    0x12
#define R_PUBDEF    0x16
#define R_EXTNAM    0x18
#define R_FIXEXT    0x20
#define R_FIXLOC    0x22
#define R_FIXSEG    0x24
#define R_LIBLOC    0x26
#define R_LIBNAM    0x28
#define R_LIBDIC    0x2A
#define R_LIBHDR    0x2C
#define R_COMDEF    0x2E

/* Segments */
#define SABS        0
#define SCODE       1
#define SDATA       2
#define SSTACK      3
#define SMEMORY     4
#define SRESERVED   5
#define SNAMED      6 /* through 254 */
#define SBLANK      255

/* Alignments  & Flags*/
#define AMASK       0xf
#define AABS        0
#define AUNKNOWN    0
#define AINPAGE     1
#define APAGE       2
#define ABYTE       3
#define ANONE       255
#define FHASADDR    0x80
#define FWRAP0      0x40
#define FSEGSEEN    0x10

/* Fixup type */
#define FLOW        1
#define FHIGH       2
#define FBOTH       3

/* module type */
#define MT_NOTMAIN  0
#define MT_MAIN     1

/* flags */
#define F_ALNMASK   0xf
#define F_SCOPEMASK 0xf0
#define F_EXTERN    0x40
#define F_PUBLIC    0x80

// link.plm
extern uint8_t alignType[6];
extern module_t *module;
extern objFile_t *objFile;
extern bool mapWanted;
extern uint16_t entryOffset;
extern uint8_t entrySeg;
extern symbol_t *hashTab[128];
extern symbol_t *headCommSym;
extern symbol_t *unresolvedList;

extern uint16_t maxExternCnt;
extern uint8_t tranId;
extern uint8_t tranVn;
extern uint8_t moduleType;
extern pstr_t const *moduleName;
extern objFile_t *objFileList;

extern uint16_t segLen[6];
extern uint8_t segmap[256];

extern uint16_t unresolved;
#define VERSION "V3.0"

// linkov.plm

extern bool externOk; // if true warnings are errors and out file is deleted
extern int warnings;
extern char *omfOutName;

void AddExtMap(symbol_t *symP);
void AddFileToInputList(char *token);
void ChainUnresolved();
void ChkLP();
void ChkRP();
void closeOMFIn();
void CreateFragment(uint8_t seg, uint16_t bot, uint16_t top);
void EmitANCESTOR();
void EmitCOMDEF();
void EmitEnding();
void EmitEXTNAMES();
void EmitMODHDR();
void EmitPUBLICS();
void EndRecord();
void ExpectChar(uint8_t ch, char const *errMsg);
void ExpectComma();
void ExpectRecord(uint8_t type);
bool ExtendRec(uint16_t cnt);
symbol_t *GetSymbolP(uint16_t symId);
uint8_t HashF(pstr_t const *pstr);
void InitExternsMap();
// void Load(address pathP, address LoadOffset, address switch, address entryP, address statusP);
bool Lookup(pstr_t const *pstr, symbol_t **pitemRef, uint8_t mask);
// void MemMov(address cnt, address srcp, address dstp);
void P1CommonSegments();
void P1LibScan();
void P1LibUserModules();
void P1ModEnd();
void P1ModHdr();
void P1Records(uint8_t newModule);
void P1StdSegments();
void ParseCmdLine();
void ParseControl();
void Pass1COMDEF();
void Pass1CONTENT();
void Pass1EXTNAMES();
void Pass1PUBNAMES();
void Pass2ANCESTOR();
void Pass2COMDEF();
void Pass2CONTENT();
void Pass2EXTNAMES();
void Pass2LINENO();
void Pass2LOCALS();
void Pass2MODHDR();
void Phase1();
void Phase2();
uint8_t SelectInSeg(uint8_t seg);
uint8_t SelectSeg(uint8_t seg);

void PrintBaseSizeAlign(uint16_t baddr, uint16_t bsize, uint8_t align);
void ModuleWarning(char const *msg);
void WriteStats();
