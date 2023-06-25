/****************************************************************************
 *  loc.h: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

/*
 * vim:ts=4:shiftwidth=4:expandtab:
 */
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "error.h"
#include <stdint.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#define stricmp strcasecmp
#endif
typedef unsigned char byte;
typedef unsigned short word;
typedef byte *pointer;
typedef word *wpointer;
#define VERSION "V3.0"


// ISIS uses MemCk to return the top of memory
// this is used to calculate a size for MEMORY seg
// as MemCk returns the top of memory for other uses
// defined a MEMCK value to use in calculation
#define MEMCK	0xf7ff

// accessor macros for HIGH and LOW
// word cast is to avoid 64bit warnings
#define High(n) ((word)((n) >> 8))
#define Low(n)  ((word)((n) & 0xff))

#pragma pack(push, 1)
typedef union {
    word w;
    struct {
        byte lb, hb;
    };
} word_t;

typedef union {
    word w;
    struct {
        byte lb, hb;
    };
    byte b[2];
    byte *bp;
    char *cp;
    word *ap;
} address;

typedef struct { // pascal generic string
    byte len;
    char str[0];
} pstr_t;

typedef struct { // pascal string for file names
    byte len;
    char str[15];
} psFileName_t;

typedef struct { // pascal string for module names
    byte len;
    char str[31];
} psModName_t;



typedef struct {
    word saddr;
    word eaddr;
} dataFrag_t;

typedef struct {
    byte state;
    byte mpage;
} page1_t;

typedef struct {
    byte pageIdx;
    byte fileIdx;
} page2_t;

//typedef struct {
//    byte rectyp;
//    word reclen;
//    byte record;
//} record_t;
#define RECORD_rectyp   0
#define RECORD_reclen   1
#define RECORD_record   3

typedef struct {
    byte flags;
    byte seg;
    word start;
    word len;
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

typedef struct {
    byte deviceId;
    byte name[6];
    byte ext[3];
    byte deviceType;
    byte driveType;
} spath_t;



// record fragments
//typedef struct {
//    word namIdx;
//    word offset;
//} extfix_t;
#define EXTFIX_namIdx   0
#define EXTFIX_offset   2

//typedef struct {
//    byte segId;
//    word offset;
//    byte dat[1];
//} moddat_t;

#define MODDAT_segId    0
#define MODDAT_offset   1
#define MODDAT_dat      3

//typedef struct {
//    byte modType;
//    byte segId;
//    word offset;
//} modend_t;
#define MODEND_modType  0
#define MODEND_segId    1
#define MODEND_offset   2

//typedef struct {
//    word offset;
//    word linNum;
//} line_t;
#define LINE_offset 0
#define LINE_linNum 2
#define LINE_sizeof 4

//typedef struct {
//    word offset;
//    byte name[1];
//} def_t;
#define DEF_offset  0
#define DEF_name    2

//typedef struct {
//    byte segId;
//    byte name[1];
//} comnam_t;
#define COMNAM_segId    0
#define COMNAM_name     1
#define COMNAM_sizeof   2   // excludes the name itself

//typedef struct {
//    byte segId;
//    word len;
//    byte combine;
//} segdef_t;
#define SEGDEF_segId    0
#define SEGDEF_len      1
#define SEGDEF_combine  3
#define SEGDEF_sizeof   4

#pragma pack(pop)

#define CTHEAD  0
#define CTPUB   1
#define CTSYM   2
#define CTLIN   3
#define CTMOD   4

#define ERR2    2 /* ILLEGAL AFTN ARGUMENT */
#define ERR3    3 /* TOO MANY OPEN FILES */
#define ERR4    4 /* INCORRECTLY SPECIFIED FILE */
#define ERR5    5 /* UNRECOGNIZED DEVICE NAME */
#define ERR9    9 /* DISK DIRECTORY FULL */
#define ERR12   12 /* FILE IS already OPEN */
#define ERR13   13 /* NO SUCH FILE */
#define ERR14   14 /* WRITE PROTECTED */
#define ERR17   17 /* NOT A DISK FILE */
#define ERR19   19 /* ATTEMPTED SEEK ON NON-DISK FILE */
#define ERR20   20 /* ATTEMPTED BACK SEEK TOO FAR */
#define ERR21   21 /* CANT RESCAN */
#define ERR22   22 /* ILLEGAL ACCESS MODE TO OPEN */
#define ERR23   23 /* MISSING FILENAME */
#define ERR27   27 /* ILLEGAL SEEK COMMAND */
#define ERR28   28 /* MISSING EXTENSION */
#define ERR31   31 /* CANT SEEK ON WRITE ONLY FILE */
#define ERR32   32 /* CANT DELETE OPEN FILE */
#define ERR35   35 /* SEEK PAST EOF */
#define ERR203  203 /* INVALID SYNTAX */
#define ERR204  204 /* PREMATURE EOF */
#define ERR208  208 /* CHECKSUM ERROR */
#define ERR210  210 /* INSUFFICIENT MEMORY */
#define ERR211  211 /* RECORD TOO LONG */
#define ERR212  212 /* ILLEGAL RELO RECORD */
#define ERR213  213 /* FIXUP BOUNDS ERROR */
#define ERR218  218 /* ILLEGAL RECORD FORMAT */
#define ERR224  224 /* BAD RECORD SEQUENCE */
#define ERR225  225 /* INVALID NAME */
#define ERR226  226 /* NAME TOO LONG */
#define ERR227  227 /* LEFT PARENTHESIS EXPECTED */
#define ERR228  228 /* RIGHT PARENTHESIS EXPECTED */
#define ERR229  229 /* UNRECOGNIZED CONTROL */
#define ERR233  233 /* 'TO' EXPECTED */
#define ERR237  237 /* COMMON NOT FOUND */
#define ERR238  238 /* ILLEGAL STACK CONTENT RECORD */
#define ERR239  239 /* NO MODULE HEADER RECORD */
#define ERR240  240 /* PROGRAM EXCEEDS 64K */

/* Record types */
#define R_MODHDR 2
#define R_MODEND 4
#define R_MODDAT 6
#define R_LINNUM 8
#define R_MODEOF 0xE
#define R_ANCEST 0x10
#define R_LOCDEF 0x12
#define R_PUBDEF 0x16
#define R_EXTNAM 0x18
#define R_FIXEXT 0x20
#define R_FIXLOC 0x22
#define R_FIXSEG 0x24
#define R_LIBLOC 0x26
#define R_LIBNAM 0x28
#define R_LIBDIC 0x2A
#define R_LIBHDR 0x2C
#define R_COMDEF 0x2E

/* Segments */
#define SABS    0
#define SCODE   1
#define SDATA   2
#define SSTACK  3
#define SMEMORY 4
#define SRESERVED   5
#define SNAMED  6   /* through 254 */
#define SBLANK  255

/* Alignments  & Flags*/
#define AMASK   0xf
#define AABS    0
#define AUNKNOWN    0
#define AINPAGE 1
#define APAGE   2
#define ABYTE   3
#define FHASADDR    0x80
#define FWRAP0  0x40
#define FSEGSEEN    0x10

/* Fixup type */
#define FLOW    1
#define FHIGH   2
#define FBOTH   3


#define	LIBDICT	0x2A
#define	LIBHDR	0x2C
#define	LIBLOC	0x26
#define	NAMES	0x28
#define	OBJEND	4
#define	OBJHDR	2
#define	PUBLICS	0x16
#define	readOnly	1
#define	writeOnly	2

/* file open modes */
#define READ_MODE   1
#define WRITE_MODE  2
#define UPDATE_MODE 3

/* seek modes */
#define SEEKTELL    0
#define SEEKBACK    1
#define SEEKABS 2
#define SEEKFWD 3
#define SEEKEND 4
#define TRUE    0xff
#define FALSE   0

#define REC_TYPE    0
#define REC_LEN     1 // word
#define REC_DATA    3 // various


/* controls seen tags */
//#define seen.start   seen(0)
//#define seen.stackSize   seen(1)
//#define seen.restart0    seen(2)
//#define seen.map     seen(3)
//#define seen.publics seen(4)
//#define seen.symbols seen(5)
//#define seen.lines   seen(6)
//#define seen.purge   seen(7)
//#define seen.name  seen(8)


extern word columns;
extern char crlf[];
extern pointer inEnd;
extern char *inName;
extern dataFrag_t inBlock;
extern pointer inP;
extern pointer inRec;
extern bool isMain;
extern byte tranId;
extern byte tranVn;
extern pstr_t const *moduleName;
extern char *outName;
extern pointer outP;
extern FILE *outFp;
extern byte outRec[];

extern FILE *lstFp;
extern char *lstName;
extern FILE *inFp;
extern word recLen;
extern word recNum;
extern seen_t seen;
extern word segBases[256];
extern byte segFlags[256];
extern byte segOrder[255];
extern word segSizes[256];
extern word startAddr;
extern word unsatisfiedCnt;
extern word targetBase;
extern char const *segNames[];
extern char *cmdP;

extern char *commandLine; // users command line
extern char *tokenLine;   // copy of the line, modified to create C string tokens
extern byte inType;
extern bool echoToStderr;
extern dataFrag_t *dataFrags;
extern int dataFragCnt;
extern segFrag_t *segFrags;
extern int segFragCnt;
extern int warningMask;
extern int warnings;
extern char *invokeName;

void AddDataFrag(word saddr, word eaddr);
pointer AddrInMem(word addr);
void AddSegFrag(byte flags, byte seg, word start, word len);
void BadRecordSeq();
void BinAsc(word number, byte base, byte pad, char *bufp, byte ndigits);

_Noreturn void FatalCmdLineErr(char const *errMsg);
void EmitModDat(dataFrag_t *curDataFragP);
void EndRecord();
_Noreturn void Exit(int retCode);
void ExpectChar(byte ch, char const *msg);
void ExpectLP();
void ExpectRP();
void ExpectSlash();
void ForceSOL();
byte GetCommonSegId(char *token);
pstr_t const *GetModuleName();
void GetRecord();
void IllegalRecord();
void IllegalReloc();
void InitRecord(byte rectyp);
void ResetSegOrder();
void AddSegOrder(byte seg);
void FixSegOrder();
void LoadModdat(byte segId);
void LocateFile();
void Rewind();
word ParseLPNumRP();
word ParseSimpleNumber();
int ParseNumber(char const *token);
char *PastAFN(char *pch);
char *Delimit(char *pch);
void PrintColumn(byte ctype,...);
void PrintListingHeader(char const *heading);
void PrintMemoryMap();
void ProcAncest();
void ProcArgsInit();
void ProcComdef();
void ProcDefs(byte list, byte ctype);
void ProcessControls();
void ProcExtnam();
void ProcHdrAndComDef();
void ProcLinNum();
void ProcModdat();
void ProcModend();
void ProcModhdr();
void pstrcpy(pstr_t const *psrc, pstr_t *pdst);
// pascal string helper macros
#define PStrcpy(psrc, pdst) pstrcpy((pstr_t *)(psrc), (pstr_t *)(pdst))
#define PSLen(pstr)         ((pstr_t *)(pstr))->len
#define PSStr(pstr)         ((pstr_t *)(pstr))->str



byte SetTargetSeg(byte seg);
void SkipNonArgChars(char *pch);
char *SkipSpc(char *pch);
void Start();


word getWord(pointer buf);
word putWord(pointer buf, word val);

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
_Noreturn void IoError(char const *path, char const *msg);
_Noreturn void FatalError(char const *fmt, ...);
_Noreturn void usage();
void printCmdLine(FILE *fp);
void printDriveMap(FILE *fp);
char *basename(char *path);
FILE *Fopen(char const *pathP, char *access);
uint32_t ReadLocation();
uint16_t ReadWord();
uint8_t ReadByte();
pstr_t *ReadName();
char *GetToken();
pstr_t const *pstrdup(pstr_t const *pstr);
pstr_t const *toPstr(char const *s);
void AssignAddress();
void CopyRecord();
char *xstrdup(char const *str);
void Printf(char const *fmt, ...);
void PrintfAndLog(char const *fmt, ...);
void Prints(char const *s);
_Noreturn void RecError(char const *errMsg);
_Noreturn void usage();
void WriteByte(uint8_t val);
void WriteWord(uint16_t val);
void WriteName(pstr_t const *name);
