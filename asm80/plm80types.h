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

typedef struct {
    byte type;
    leword len;
    byte segid;
    leword offset;
    byte dta[121];
    byte crc;
} content_t;

typedef struct {
    byte type;
    leword len;
    byte crc;
} eof_t;

typedef struct {
    byte type;
    leword len;
    byte dta[124];
    byte crc;
} extnames_t;

typedef struct {
    byte type;
    leword len;
    byte hilo;
    leword dta[30];
    byte crc;
} extref_t;

typedef struct {
    byte type;
    leword len;
    byte segid;
    byte hilo;
    leword dta[29];
    byte crc;
} interseg_t;

typedef struct {
    byte type;
    leword len;
    byte modtyp;
    byte segid;
    leword offset;
    byte crc;
} modend_t;

typedef struct {
    byte type;
    leword len;
    byte dta[26];
} modhdr_t;

typedef struct {
    byte type;
    leword len;
    byte segid;
    byte dta[124];
    byte crc;
} publics_t;

typedef struct {
    byte type;
    leword len;
    byte hilo;
    leword dta[62];
} reloc_t;

typedef struct {
    char name[15];
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

typedef union {
    macro_t stack[10];
    macro_t top;
} macroStk_t;
#pragma pack(pop)

typedef struct {
    pointer start;
    tokensym_t *symbol;
    byte type;
    byte size;
    byte attr;
    word symId;   
} token_t;

