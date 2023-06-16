/****************************************************************************
 *  link.h: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef _WIN32
#include <io.h>
#define DIRSEP "/\\"
#define namecmp stricmp // case insensitive file name compare
#else
#include <errno.h>
#include <unistd.h>
#define _MAX_PATH 4096
#define O_BINARY  0
#define DIRSEP    "/"
#define namecmp   strcmp
#endif

#ifndef _MSC_VER
#define stricmp strcasecmp
#endif

typedef unsigned char byte;
typedef unsigned short word;
typedef byte *pointer;
typedef word *wpointer;

#define High(n)    ((n) >> 8)
#define Low(n)     ((n)&0xff)
#define Shr(v, n)  ((word)(v) >> (n))
#define Shl(v, n)  ((word)(v) << (n))
#define RorB(v, n) ((byte)(((v) | ((v) << 8)) >> (n)))
#define RolB(v, n)	((byte)(Ror((v), (8 - (n))))
#define Move(s, d, c) memcpy(d, s, c)

typedef struct { // generic pascal string
    byte len;
    char str[0];
} pstr_t;

typedef struct { // pascal string for module names
    byte len;
    char str[31];
} psModName_t;

//typedef struct {
//    byte rectyp;
//    word reclen;
//    byte record[1];
//} record_t;

#define REC_TYPE 0
#define REC_LEN  1 // word
#define REC_DATA 3    // various

typedef struct _segfrag {
    struct _segfrag *next;
    word bot;
    word top;
} segFrag_t;

typedef struct _symbol {
    struct _symbol *hashChain;
    byte flags;
    byte seg;
    union {
        word offsetOrSym;
        word len;
        word symId;
    };
    struct _symbol *nxtSymbol;
    pstr_t name;
} symbol_t;

typedef struct _module {
    struct _module *next;
    uint32_t location;
    symbol_t *symbols;
    word cbias;
    word dbias;
    pstr_t name;
} module_t;

typedef struct _library {
    struct _library *next;
    module_t *modules;
    bool publics;
    bool isLib;
    char *name;
} objFile_t;


// record fragments
// typedef struct {
//	word symId;
//	word offset;
//} extref_t;
#define EXTREF_symId     0
#define EXTREF_offset    2

// typedef struct {
//	byte segId;
//	byte fixType;
// } interseg_t;

#define INTERSEG_segId   0
#define INTERSEG_fixType 1

// typedef struct {
//	byte segId;
//	word offset;
//	byte dat[1];
// } moddat_t;

#define MODDAT_segId     0
#define MODDAT_offset    1
#define MODDAT_dat       3

// typedef struct {
//	byte modType;
//	byte segId;
//	word offset;
// } modend_t;

#define MODEND_modType   0
#define MODEND_segId     1
#define MODEND_offset    2
#define MODEND_sizeof    4

// typedef struct {
//	word offset;
//	word linNum;
// } line_t;
#define LINE_offset      0
#define LINE_linNum      2

// typedef struct {
//	word offset;
//	pstr_t name;
// } def_t;
#define DEF_offset       0
#define DEF_name         2

// typedef struct {
//	byte segId;
//	byte name[1];
// } comnam_t;
#define COMNAM_segId     0
#define COMNAM_name      1
#define COMNAM_sizeof    2 // + name chars

// typedef struct {
//	byte segId;
//	word len;
//	byte combine;
// } segdef_t;

#define SEGDEF_segId     0
#define SEGDEF_len       1
#define SEGDEF_combine   3
#define SEGDEF_sizeof    4

typedef struct {
    byte combine;
    word lenOrLinkedSeg;
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

/* segment types */
#define SEG_ABS     0
#define SEG_CODE    1
#define SEG_DATA    2
#define SEG_STACK   3
#define SEG_MEMORY  4
#define SEG_RESERVE 5 /* reserved for future Intel use */
#define SEG_NAMCOM  6
#define SEG_BLANK   255

/* module type */
#define MT_NOTMAIN  0
#define MT_MAIN     1

/* flags */
#define F_ALNMASK   0xf
#define F_SCOPEMASK 0xf0
#define F_EXTERN    0x40
#define F_PUBLIC    0x80


// link.plm
extern byte alignType[6];
extern module_t *module;
extern objFile_t *objFile;
extern bool mapWanted;
extern word entryOffset;
extern byte entrySeg;
extern pointer inEnd;
extern symbol_t *hashTab[128];
extern symbol_t *headCommSym;
extern symbol_t *unresolvedList;
extern char *objName;
extern pointer inP;
extern pointer inRecord;
extern word maxExternCnt;
extern byte tranId;
extern byte tranVn;
extern byte moduleType;
extern psModName_t moduleName;
extern objFile_t *objFileList;
extern pointer outP;
extern char *lstName;
extern FILE *lstFp;
extern FILE *objFp;
extern word recLen;
extern word recNum;
extern word segLen[6];
extern byte segmap[256];
extern FILE *outFp;
extern char *outName;
extern bool echoToStderr;


extern word unresolved;
#define VERSION "V3.0"

// linkov.plm
extern psModName_t modName;
extern byte outRec[];

extern char *commandLine;
extern char *tokenLine; // a copy of command line that is used for tokens
extern char const *cmdP;
extern byte inType;
extern bool warnOK;     // if true warnings are errors and out file is deleted
extern int warned;
extern char const *invokeName;

void AddExtMap(symbol_t *symP);
void AddFileToInputList(char *token);
void BadRecordSeq();
void ChainUnresolved();
void ChkLP();
void ChkRP();
void Close(word conn, wpointer statusP);
void CloseObjFile();
void CreateFragment(byte seg, word bot, word top);
char const *Deblank(char const *pch);
char const *Delimit(char const *pch);
void EmitANCESTOR();
void EmitCOMDEF();
void EmitEnding();
void EmitEXTNAMES();
void EmitMODHDR();
void EmitPUBLICS();
void EndRecord();
void ExpectChar(byte ch, char const *errMsg);
void ExpectComma();
void ExpectRecord(byte type);
bool ExtendRec(word cnt);
_Noreturn void FatalCmdLineErr(char const *errMsg);
_Noreturn void RecError(char const *errMsg);
void GetInputListItem();
void GetModuleName(char *token, psModName_t *pstr);
void GetRecord();
symbol_t *GetSymbolP(word symId);
byte HashF(pstr_t *pstr);
void IllegalRelo();
_Noreturn void IllFmt();
void InitExternsMap();
void InitRecord(byte type);
// void Load(address pathP, address LoadOffset, address switch, address entryP, address statusP);
bool Lookup(pstr_t *pstr, symbol_t **pitemRef, byte mask);
// void MemMov(address cnt, address srcp, address dstp);
void OpenObjFile();
void P1CommonSegments();
void P1LibScan();
void P1LibUserModules();
void P1ModEnd();
void P1ModHdr();
void P1Records(byte newModule);
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
void Position(uint32_t location);
#define Pstrcpy(psrc, pdst) pstrcpy((pstr_t *)(psrc), (pstr_t *)(pdst))
void pstrcpy(pstr_t const *psrc, pstr_t *pdst);
uint32_t ReadLocation();
uint16_t ReadWord();
uint8_t ReadByte();

void ReadCmdLine();
pstr_t *ReadName();
void Rescan(word conn, wpointer statusP);
byte SelectInSeg(byte seg);
byte SelectSeg(byte seg);
void SkipNonArgChars(char const *arg1w);
void Start();
bool PStrequ(pstr_t *s, pstr_t *t);
void PrintAndEcho(char const *fmt, ...);
void PrintBaseSizeAlign(word baddr, word bsize, byte align);
void Printf(char const *fmt, ...);
void ModuleWarning(char const *msg);
void WriteStats();

// io.c support functions
word putWord(pointer buf, word val);
word getWord(pointer buf);

_Noreturn void IoError(char const *path, char const *msg);
FILE *Fopen(char const *pathP, char *access);
_Noreturn void FatalError(char const *fmt, ...);
char *basename(char *path);
void printCmdLine(FILE *fp);
void printDriveMap();

char *p2cstr(pstr_t *p);
void WriteName(pstr_t *name);
void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);

_Noreturn void Exit(int result);
_Noreturn void usage();