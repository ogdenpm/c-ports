/****************************************************************************
 *  plm80types.h: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#pragma once


typedef unsigned char byte;
typedef unsigned short word;
typedef byte *pointer;
typedef word *wpointer;
typedef byte leword[2];     // used to force word matches OMF definition

#pragma pack(push, 1)


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


// the record formats
// to remove need for pack, the notional
// structures are replaced with offsets into a
// byte array. The defines below give the offset into
// the array. Unless fixed crc location is determined
// at runtime

// header:
#define HDR_TYPE    0
#define HDR_LEN     1   // word

// content:
//typedef struct {
//    byte type;
//    leword len;
//    byte segid;
//    leword offset;
//    byte dta[121];
//    byte crc;
//} content_t;

#define CONTENT_SEGID   3
#define CONTENT_OFFSET  (CONTENT_SEGID + 1)
#define CONTENT_DATA(n) (CONTENT_OFFSET + 2 + (n))
#define CONTENT_MAX    124

// eof: 
//typedef struct {
//    byte type;
//    leword len;
//    byte crc;
//} eof_t;

// extnames:
//typedef struct {
//    byte type;
//    leword len;
//    byte dta[124];
//    byte crc;
//} extnames_t;
#define EXTNAMES_DATA(n) (3 + (n))
#define EXTNAMES_MAX    126

// extref:
//typedef struct {
//    byte type;
//    leword len;
//    byte hilo;
//    leword dta[30];
//    byte crc;
//} extref_t;
#define EXTREF_HILO 3
#define EXTREF_DATA(n) (EXTREF_HILO + 1 + 2 * (n))
#define EXTREF_MAX  61

// interseg:
//typedef struct {
//    byte type;
//    leword len;
//    byte segid;
//    byte hilo;
//    leword dta[29];
//    byte crc;
//} interseg_t;
#define INTERSEG_SEGID  3
#define INTERSEG_HILO   (INTERSEG_SEGID + 1)
#define INTERSEG_DATA(n)   (INTERSEG_HILO + 1 + 2 * (n))
#define INTERSEG_MAX    60

// modend:
//typedef struct {
//    byte type;
//    leword len;
//    byte modtyp;
//    byte segid;
//    leword offset;
//    byte crc;
//} modend_t;
#define MODEND_TYPE     3
#define MODEND_SEGID    (MODEND_TYPE + 1)
#define MODEND_OFFSET   (MODEND_SEGID + 1)
#define MODEND_MAX      4

// modhdr:
typedef struct {
    byte type;
    leword len;
    byte dta[26];
} modhdr_t;
#define MODHDR_DATA(n)  (3 + (n))
#define MODHDR_MAX      26

// publics:
//typedef struct {
//    byte type;
//    leword len;
//    byte segid;
//    byte dta[124];
//    byte crc;
//} publics_t;
#define PUBLICS_SEGID   3
#define PUBLICS_DATA    (PUBLICS_SEGID + 1)
#define PUBLICS_MAX     125

// reloc:
//typedef struct {
//    byte type;
//    leword len;
//    byte hilo;
//    leword dta[62];
//} reloc_t;
#define RELOC_HILO      3
#define RELOC_DATA(n)   (RELOC_HILO + 1 + 2 * (n))
#define RELOC_MAX       125

typedef struct {
    char *name;
    word blk, byt;
    byte b19;
    FILE *fp;
} file_t;

typedef struct {
    byte condSP;
    byte ifDepth;
    byte mtype;
    byte localsCnt;
    word localIdBase;
    pointer bufP;
    word blk;
    word savedBlk;
    pointer pCurArg;	// pointer to current IRPC char or IRP arg
    word cnt;		// IRPC -> char count, IRP -> num args, DoRept -> count
} macro_t;

typedef struct {
    word tok[2];
    union {
        struct {
            byte base;
            byte delta;
        };
        word addr;
        word line;
        word value;
        word offset;
        word paramId;
        word blk;
    };
    byte type;
    union {
        byte flags;
        byte nlocals;
    };
} tokensym_t;

typedef struct {
    word tok[2];
    byte base;
    byte delta;
    byte type;
    byte flags;
} keyword_t;


typedef union {
    byte	all[20];
    struct {
        bool debug;
        bool macroDebug;
        bool xref;
        bool symbols;
        bool paging;
        bool tty;
        bool mod85;
        bool print;
        bool object;
        bool macroFile;
        byte pageWidth;
        byte pageLength;
        byte inlcude;
        bool title;
        byte save;
        byte restore;
        byte eject;
        /* SAVE/RESTORE act on next 3 opts */
        bool list;
        bool gen;
        bool cond;
    };
} controls_t;

#pragma pack(pop)
typedef struct {
    pointer start;
    tokensym_t *symbol;
    byte type;
    byte size;
    byte attr;
    word symId;   
} token_t;

