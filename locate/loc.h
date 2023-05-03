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
#include "error.h"
typedef unsigned char byte;
typedef unsigned short word;
typedef byte *pointer;
typedef word *wpointer;

// ISIS uses MemCk to return the top of memory
// this is used to calculate a size for MEMORY seg
// as MemCk returns the top of memory for other uses
// defined a MEMCK value to use in calculation
#define MEMCK	0xff00

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
    byte all[9];
    struct {
        byte start;
        byte stackSize;
        byte restart0;
        byte map;
        byte publics;
        byte symbols;
        byte lines;
        byte purge;
        byte name;
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



/* Seek modes */
#define SEEKTELL 0
#define SEEKBCK 1
#define SEEKABS 2
#define SEEKFWD 3
#define SEEKEND 4

#define ERR2    2 /* ILLEGAL AFTN ARGUMENT */
#define ERR3    3 /* TOO MANY OPEN FILES */
#define ERR4    4 /* INCORRECTLY SPECIFIED FILE */
#define ERR5    5 /* UNRECOGNIZED DEVICE NAME */
#define ERR9    9 /* DISK DIRECTORY FULL */
#define ERR12   12 /* FILE IS ALREADY OPEN */
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

extern pointer MEMORY;

extern word actRead;
extern char aRecordType[];
extern pointer baseMemImage;
extern pointer botHeap;
extern byte columns;
extern char crlf[];
extern dataFrag_t *curDataFragP;
extern segFrag_t *curSegFragP;
extern pointer eiBufP;
extern pointer eoutP;
extern pointer epbufP;
extern pointer erecP;
extern byte havePagingFile;
extern pointer iBufP;
extern word inBlk;
extern pointer inbP;
extern word inByt;
extern byte inCRC;
extern psFileName_t inFileName;
extern dataFrag_t inFragment;
extern pointer inP;
extern pointer inRecordP;
extern byte isMain;
extern byte modhdrX1;
extern byte modhdrX2;
extern psModName_t moduleName;
extern dataFrag_t *nextDataFragP;
extern segFrag_t *nextSegFragP;
extern word npbuf;
extern byte nxtPageNo;
extern psFileName_t outFileName;
extern pointer outP;
extern word outputfd;
extern bool outRealFile;
extern pointer outRecordP;
extern byte pageCacheSize;
extern byte pageIndexTmpFil;
extern page1_t *pageTab1P;
extern page2_t *pageTab2P;
extern pointer pbufP;
extern word printfd;
extern psFileName_t printFileName;
extern word readfd;
extern word recLen;
extern word recNum;
extern byte roundRobinIndex;
extern seen_t seen;
extern word segBases[256];
extern byte segFlags[256];
extern byte segOrder[255];
extern word segSizes[256];
extern pointer sibufP;
extern pointer spbufP;
extern word startAddr;
extern word statusIO;
extern word tmpfd;
extern psFileName_t tmpFileName;
extern pointer topDataFrags;
extern pointer topHeap;
extern bool usePrintBuf;
extern char version[];
extern char alin[];
extern char aMod[];
extern char aPub[];
extern char aReadFromFile[];
extern char aReferenceToUns[];
extern char aSym[];
extern char aSymbol[];
extern char aSymbolTableOfM[];
extern char aUnsatisfiedExt[];
extern char aValueType[];
extern char aWrittenToFile[];
extern byte curcol;
extern address curColumn;
extern char const *curListField;
extern byte loHiBoth;
extern pointer lsoutP;
extern byte nameLen;
extern byte outSegType;
extern char spc32[];
extern word unsatisfiedCnt;
extern word workingSegBase;
extern char x5[];
extern char a0LengthSegment[];
extern char aAddresses[];
extern char alignNames[];
extern char aMemoryMapOfMod[];
extern char aMemOverlap[];
extern char aModuleIsNotAMa[];
extern char aModuleStartAdd[];
extern char aRestartControl[];
extern char aStartControlIg[];
extern char aStartStopLengt[];
extern char segNames[];
extern char aCommandTailErr[];
extern char aInvokedBy[];
extern char cin[];
extern char *cmdP;
extern byte controls[];
extern char cout[];
extern char mdebug[];
extern char mstar2[];
extern char mto[];
extern char mtoand[];
extern char *scmdP;
extern char signonMsg[];
extern spath_t spathInfo;
extern char tmpFileInfo[];
extern char aInpageSegment2[];
extern byte nxtSegOrder;
extern byte pad7935[];
extern byte segId;

void AddDataFrag(word saddr, word eaddr);
pointer AddrInCache(word addr);
void AddSegFrag(byte flags, byte seg, word start, word len);
//static void Alloc(word cnt);
pointer AllocNewPage(byte page);
void AnotherPage(byte page);
void BadRecordSeq();
void BinAsc(word number, byte base, byte pad, char *bufp, byte ndigits);
void ChkRead(word cnt);
void Close(word conn, wpointer statusP);
void CmdErr(word err);
void ConAndPrint(char const *buf, word cnt);
void ConStrOut(char const *buf, word cnt);
void Delete(char const *pathP, wpointer statusP);
void EmitModDat(dataFrag_t *curDataFragP);
void EndRecord();
void ErrChkReport(word errCode, char *file, bool errExit);
void Errmsg(word errCode);
void ErrNotADisk();
void Error(word ErrorNum);
void Exit(int retCode);
void ExpectChar(byte ch, byte err);
void ExpectLP();
void ExpectRP();
void ExpectSlash();
void FatalErr(byte errCode);
void FixupBoundsChk(word addr);
void FlushOut();
void FlushPrintBuf();
void ForceSOL();
pointer GetCommonName(byte segid);
byte GetCommonSegId();
void GetFile();
void GetPstrName(pstr_t *pstr);
void GetRecord();
void IllegalRecord();
void IllegalReloc();
void InitRecOut(byte rectyp);
void InitSegOrder();
void InsSegIdOrder(byte seg);
void LoadModdat(byte segId);
void LocateFile();
void MakeFullName(spath_t *pinfo, char *pstr);
pointer MemCk();
void ObjSeek(word blk, word byt);
void Open(wpointer connP, char const *pathP, word access, word echo, wpointer statusP);
void PageOut(byte page, pointer bufp);
word ParseLPNumRP();
word ParseNumber(char **ppstr);
char *PastAFN(char *pch);
char *PastFileName(char *pch);
void PrintColumn(char const *field, pstr_t const *pstr);
void PrintCrLf();
void PrintListingHeader(char const *buf, word len);
void PrintMemoryMap();
void PrintString(char const *bufp, word cnt);
void ProcAncest();
void ProcArgsInit();
void ProcComdef();
void ProcDefs(byte list, char *template);
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

void Read(word conn, pointer buffP, word count, wpointer actualP, wpointer statusP);
void ReadCmdLine();
void Rescan(word conn, wpointer statusP);
void Seek(word conn, word mode, wpointer blockP, wpointer byteP, wpointer statusP);
void SeekOutFile(word mode, wpointer pblk, wpointer pbyt);
void SeekPagingFile(byte para);
byte SetWorkingSeg(byte seg);
void SkipNonArgChars(char *pch);
char *SkipSpc(char *pch);
void Spath(char const *pathP, spath_t *infoP, wpointer statusP);
void Start();
bool Strequ(char const *pstr1, char const *pstr2, byte len);
void StrUpr(char *pch);
byte ToUpper(byte ch);
void Write(word conn, void const *buffP, word count, wpointer statusP);
void WriteBytes(pointer bufP, word cnt);


word getWord(pointer buf);
word putWord(pointer buf, word val);