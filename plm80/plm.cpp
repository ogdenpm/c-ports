/****************************************************************************
 *  plm.cpp: part of the C port of Intel's ISIS-II plm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C++ by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// $Id: plm.cpp,v 1.2 2004/11/30 23:48:08 Mark Exp $
#include <cstdio>
#include "plm.hpp"
#include "common.hpp"
#include "trace.hpp"
#include <showVersion.h>

Byte *address::memory = new Byte[0x10000];

#ifndef _MSC_VER
#define _stricmp	strcasecmp
#endif

address topMem;
address botMem;
address botInfo;
address topInfo;
address topSymbol;
symbol_pt botSymbol;
symbol_pt curSymbol_p;
info_pt curInfo_p;
word offNextCmdChM1;
word LEFTMARGIN;
word localLabelCnt;
word srcFileIdx;
symbol_pt *hashChains_p;
word blockDepth;
address localLabels_p;
address word_381E;
address helpers_p;
address word_3822;
word linesRead;
word programErrCnt;
word procCnt;
word word_382A;
word uninitDataSize;
word initDataSize;
word objBlk;
word objByte;

srcFiletbl_t srcFileTable[6];

file_t srcFile, lstFile, objFile, conFile, tx1File;
file_t tx2File, atFile, nmsFile, xrfFile, ixiFile;

word procChains[35];
word procInfo[255];
word blk1Used = 400;
word blk2Used = 400;
address word_3C34 = 0x9f00;
word blkSize1 = 0xc400;		// = topmem - 0xc400 - 256 after initialisation 
word blkSize2 = 0xa400;		// = topmem - 0xa400 - 256 after initialisation 
Byte srcStemLen;
Byte byte_3C3B = 0xff;
Byte IXREFSet = 0xff;
Byte PRINTSet = 0xff;
Byte OBJECTSet = 0xff;
Byte debugFlag;
Byte unexpectedEOF;
Byte haveModule;
Byte fatalErrorCode;
Byte pad3C43 = 1;
address word_3C44 = 0xA000;
Byte CONTROLS[8];	// PRINT etc

Byte pad_3C4E[2];
Byte srcStemName[10];
Byte debugSwitches[26];
cmdLine_pt cmdLine_p;
cmdLine_pt startCmdLine_p;
//char aF0Plm80_ov1[/*15*/] = ":F0:PLM80 .OV1 ";
//char aF0Plm80_ov2[/*15*/] = ":F0:PLM80 .OV2 ";
//char aF0Plm80_ov3[/*15*/] = ":F0:PLM80 .OV3 ";
//char aF0Plm80_ov4[/*15*/] = ":F0:PLM80 .OV4 ";
//char aF0Plm80_ov5[/*15*/] = ":F0:PLM80 .OV5 ";
char ixiFileName[15];
char lstFileName[15];
char objFileName[15];
word pageNo;
char byte_3CF2;
char *lstBufPtr = &byte_3CF2;     // 3CF3
word lstChCnt;
word lstBufSize;
Byte lstFileOpen;	// 3CF9

Byte linesLeft;
Byte byte_3CFB;
Byte byte_3CFC;
Byte byte_3CFD;
Byte col;
Byte skipCnt;
Byte tabWidth;
Byte TITLELEN = 1;
Byte PAGELEN = 60;
word PAGEWIDTH = 120;
Byte listingMargin = 0xff;
char DATE[9];
char aPlm80Compiler[/*20*/] = "PL/M-80 COMPILER    ";
char TITLE[60] = {" "};
word ISISVECTOR = 0x40;
word REBOOTVECTOR = 0;

Byte intVecNum = 8;
word intVecLoc;
Byte hasErrors;
//char aOverlay6[/*10*/] = ":F0:PLM80 " ;
//char a_ov6[/*5*/] = ".OV6 ";
char version[/*5*/] = "X000";
char invokeName[/*10*/] = ":F0:PLM80 ";
//char a_ov0[/*5*/] = ".OV0 ";
Byte pad_3DAC[239];

char aC197619771982I[/*31*/] = "(C) 1976, 1977, 1982 INTEL CORP";

char aProgram_version[] = "program_version_number=";
char progVersion[/*4*/] = "V4.0";


Byte state;
char *cmdText_p;
file_t loadFile;
Byte ioBuffer[2048];
char builtins[] = {
        5, 'C', 'A', 'R', 'R', 'Y', 0,  0,  2,  
        3, 'D', 'E', 'C', 1,  1,  2,  
        6, 'D', 'O', 'U', 'B', 'L', 'E', 2,  1,  3,  
        4, 'H', 'I', 'G', 'H', 3,  1,  2,  
        5, 'I', 'N', 'P', 'U', 'T', 4,  1,  2,  
        4, 'L', 'A', 'S', 'T', 5,  1,  3,  
        6, 'L', 'E', 'N', 'G', 'T', 'H', 6,  1,  3,  
        3, 'L', 'O', 'W', 7,  1,  2,  
        4, 'M', 'O', 'V', 'E', 8,  3,  0,  
        6, 'O', 'U', 'T', 'P', 'U', 'T', 9,  1,  0,  
        6, 'P', 'A', 'R', 'I', 'T', 'Y', 10,  0,  2,  
        3, 'R', 'O', 'L', 11,  2,  2,  
        3, 'R', 'O', 'R', 12,  2,  2,  
        3, 'S', 'C', 'L', 13,  2,  2,  
        3, 'S', 'C', 'R', 14,  2,  2,  
        3, 'S', 'H', 'L', 15,  2,  2,  
        3, 'S', 'H', 'R', 16,  2,  2,  
        4, 'S', 'I', 'G', 'N', 17,  0,  2,  
        4, 'S', 'I', 'Z', 'E', 18,  1,  2,  
        8, 'S', 'T', 'A', 'C', 'K', 'P', 'T', 'R', 19,  0,  3,  
        4, 'T', 'I', 'M', 'E', 20,  1,  0,  
        4, 'Z', 'E', 'R', 'O', 21,  0,  2,  
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

char keywords[] = {
        7, 'A', 'D', 'D', 'R', 'E', 'S', 'S', T_ADDRESS, 
        3, 'A', 'N', 'D', T_AND,  
        2, 'A', 'T', T_AT, 
        5, 'B', 'A', 'S', 'E', 'D', T_BASED, 
        2, 'B', 'Y', T_BY, 
        4, 'B', 'Y', 'T', 'E', T_BYTE, 
        4, 'C', 'A', 'L', 'L', T_CALL,  
        4, 'C', 'A', 'S', 'E', T_CASE, 
        4, 'D', 'A', 'T', 'A', T_DATA, 
        7, 'D', 'E', 'C', 'L', 'A', 'R', 'E', T_DECLARE,  
        7, 'D', 'I', 'S', 'A', 'B', 'L', 'E', T_DISABLE,  
        2, 'D', 'O', T_DO,  
        4, 'E', 'L', 'S', 'E', T_ELSE,
        6, 'E', 'N', 'A', 'B', 'L', 'E', T_ENABLE,  
        3, 'E', 'N', 'D', T_END, 
        3, 'E', 'O', 'F', T_EOF, 
        8, 'E', 'X', 'T', 'E', 'R', 'N', 'A', 'L', T_EXTERNAL, 
        2, 'G', 'O', T_GO, 
        4, 'G', 'O', 'T', 'O', T_GOTO, 
        4, 'H', 'A', 'L', 'T', T_HALT, 
        2, 'I', 'F', T_IF, 
        7, 'I', 'N', 'I', 'T', 'I', 'A', 'L', T_INITIAL, 
        9, 'I', 'N', 'T', 'E', 'R', 'R', 'U', 'P', 'T', T_INTERRUPT, 
        5, 'L', 'A', 'B', 'E', 'L', T_LABEL, 
        9, 'L', 'I', 'T', 'E', 'R', 'A', 'L', 'L', 'Y', T_LITERALLY, 
        5, 'M', 'I', 'N', 'U', 'S', T_MINUS,  
        3, 'M', 'O', 'D', T_MOD,  
        3, 'N', 'O', 'T', T_NOT,  
        2, 'O', 'R', T_OR,  
        4, 'P', 'L', 'U', 'S', T_PLUS,  
        9, 'P', 'R', 'O', 'C', 'E', 'D', 'U', 'R', 'E', T_PROCEDURE, 
        6, 'P', 'U', 'B', 'L', 'I', 'C', T_PUBLIC, 
        9, 'R', 'E', 'E', 'N', 'T', 'R', 'A', 'N', 'T', T_REENTRANT, 
        6, 'R', 'E', 'T', 'U', 'R', 'N', T_RETURN, 
        9, 'S', 'T', 'R', 'U', 'C', 'T', 'U', 'R', 'E', T_STRUCTURE, 
        4, 'T', 'H', 'E', 'N', T_THEN, 
        2, 'T', 'O', T_TO, 
        5, 'W', 'H', 'I', 'L', 'E', T_WHILE, 
        3, 'X', 'O', 'R', T_XOR,  
        0
};


void getCmdLine (void);
void installBuiltins (void);
void installKeywords (void);
address MEMCHK (void);
void parseInvokeName (void);
void parseSrcFile (void);
void setInfoSymValue(word val);
void setTitle (const char *buf, Byte len);
void sub_40AC (void);
void sub_45F6 (void);
void sub_4767 (void);
void sub_4845 (void);
void setMarkerInfo (Byte b1, Byte b2, Byte b3);
void setPageNo (word val);
void setMarginAndTabW (Byte b1, Byte b2);
//extern int strncmp (const char *, const char *, size_t);
//void ei();
//void hlt();


void (*fatalError)(Byte errcode) = fatalError_main;


int main(int argc, char **argv) {
    int pass = 0;
    
    CHK_SHOW_VERSION(argc, argv);

    gargc = argc;
    gargv = argv;
    state = 10;
    sub_40AC();
    state = 15;
    sub_4767();
    while (pass >= 0) {
        switch (pass) {
        case 0:
            fatalError = fatalError_ov0;
            pass = overlay0();
            break;
        case 1:
            fatalError = fatalError_ov1;
            pass = overlay1();
            break;
        case 2:
            pass = overlay2();
            break;
        case 3:
            pass = overlay3();
            break;
        case 4:
            pass = overlay4();
            break;
        case 5:
            pass = overlay5();
            break;
        case 6:
            pass = overlay6();
            break;
        }
    }
    return 0;
}



void fatalError_main(Byte errcode)
{
    if (errcode == ERR83)
        fatal("DYNAMIC STORAGE OVERFLOW", 0x18);
    fatal("UNKNOWN FATAL ERROR", 0x13);
    exit(errcode);
}



void sub_40AC(void) {
    movemem(4, progVersion, version);
    initFile(&conFile, "CONSOL", ":CI: ");
    openFile(&conFile, 1);
    topMem = MEMCHK() - 12;
    if (topMem < 0xc000) 
        fatal("NOT ENOUGH MEMORY FOR A COMPILATION", 35);
    getCmdLine();
    printf("\r\nISIS-II PL/M-80 COMPILER %.4s\r\n", version);
    cmdText_p = cmdLine_p->text;
    blkSize1 = (word)(topMem - blkSize1 - 256);	// blkSize1 = 0xc400 at start
    blkSize2 = (word)(topMem - blkSize2 - 256);	// blkSize2 = 0xa400 at start
    parseInvokeName();
    parseSrcFile();
    sub_45F6();
}


void skipSpace(void) {
    while (*cmdText_p == ' ' || *cmdText_p == '&') 
        if (*cmdText_p == ' ')
            cmdText_p++;
            else if (cmdLine_p->link != 0) {
                    cmdLine_p = cmdLine_p->link;
            cmdText_p = cmdLine_p->text;
            }
}

Byte testToken(const char *buf, Byte len) {
    char *p;

    p = cmdText_p;
    while (len != 0) {
        if ((*cmdText_p & 0x5f) != *buf) {
            cmdText_p = p;
            return 0;
        } else {
            cmdText_p++;
            buf++;
            len--;
        }
    }
    return 0xff;
}


void skipAlphaNum(void) {
    while (  ('A' <= *cmdText_p && *cmdText_p <= 'Z')
          || ('a' <= *cmdText_p && *cmdText_p <= 'z')
          || ('0' <= *cmdText_p && *cmdText_p <= '9'))
        cmdText_p++;
}

void getCmdLine(void) {
    word status, actual;
    Byte quote, i;

    Rescan(1, &status);
    if (status != 0)
        fatalIO(&conFile, status);
    startCmdLine_p = 0;
    cmdLine_p = topMem;
    for (;;) {
        readFile(&conFile, ioBuffer, 128, &actual);
        if (ioBuffer[actual - 1] != '\n' || ioBuffer[actual - 2] != '\r')
            fatal("INVOCATION COMMAND DOES NOT END WITH <CR><LF>", 45);
        topMem = (Byte *)cmdLine_p - actual - 3;
        if (startCmdLine_p == 0)
            startCmdLine_p = topMem;
        else
            cmdLine_p->link = topMem;
        (cmdLine_p = topMem)->len = (Byte)actual;
        movemem(actual, ioBuffer, cmdLine_p->text);	// copy user text over
        quote = 0;
        for (i = 0; i < actual; i++) {
            if (ioBuffer[i] == '\'')
                quote = ~quote;
            else if (ioBuffer[i] == '&' && !quote)
                goto extend;
        }
        cmdLine_p->link = 0;	// clear link in last line
        cmdLine_p = startCmdLine_p;
        topMem--;
        return;

extend:	printf("**");
    }
}

void parseInvokeName(void) {
    char *start_p /*, *p */;
    word len;

    skipSpace();
    debugFlag = testToken("DEBUG", 5) ? 0xff : 0;

    skipSpace();
    if (*(start_p = cmdText_p) == ':')
        cmdText_p += 4;		// past the :xx:
    skipAlphaNum();
    if ((len = word(cmdText_p - start_p)) > 10)	// limit file name
        len = 10;
    //for (p = aF0Plm80_ov1; p <= aF0Plm80_ov5; p += /*15*/sizeof(aF0Plm80_ov1))
    //	movemem(len, start_p, p);	// replace the invoke name for overlays
    movemem(len, start_p, invokeName);
    //movemem(len, start_p, aOverlay6);
    //for (p = aF0Plm80_ov1; p <= aF0Plm80_ov5; p += /*15*/sizeof(aF0Plm80_ov1))
    //	movemem(5, p + 10, p + len);	// move the .ov? to end of name
    //movemem(5, ".OV0 ", invokeName + len);
    //movemem(5, ".OV6 ", aOverlay6 + len);
}

void parseSrcFile(void) {
    char *fullName_p;
    char *filename_p;
    word nameLen;

    // skip to end of token
    while (*cmdText_p != ' ' && *cmdText_p != '\r' && *cmdText_p != '&')
        cmdText_p++;
    skipSpace();
    if (*(fullName_p = cmdText_p) == ':') {	// check for :xx:
        if (cmdText_p[3] != ':')
            fatal("INCORRECT DEVICE SPEC", 21);
        if ('a' <= cmdText_p[1])
            cmdText_p[1] &= 0x5f;
        if (cmdText_p[1] != 'F')
            fatal("SOURCE FILE NOT A DISKETTE FILE", 31);
        cmdText_p += 4;
    }
    filename_p = cmdText_p;
    skipAlphaNum();
    if ((nameLen = word(cmdText_p - filename_p)) == 0 || nameLen > 6)
        fatal("SOURCE FILE NAME INCORRECT", 26);
    srcStemLen = Byte(cmdText_p - fullName_p); 
    FILL(10, srcStemName, ' ');
    movemem(srcStemLen, fullName_p, srcStemName);
    if (*cmdText_p == '.') {
        filename_p = ++cmdText_p;
        skipAlphaNum();
        if ((nameLen = word(cmdText_p - filename_p)) == 0 || nameLen > 3)
            fatal("SOURCE FILE EXTENSION INCORRECT", 31);
    }
    nameLen = word(cmdText_p - fullName_p);
    srcFileIdx = 0;
    FILL(16, srcFileTable[0].filename, ' ');
    movemem(nameLen, fullName_p, srcFileTable[0].filename);
    FILL(4, (Byte *)&srcFileTable[0].block, 0);
    skipSpace();
    if (*cmdText_p == '$')
        fatal("ILLEGAL COMMAND TAIL SYNTAX", 27);
    if (*cmdText_p == '\r') 
        offNextCmdChM1 = 0;	// no more command line
    else
        offNextCmdChM1 = word(cmdText_p - (char *)cmdLine_p - 1);
}



void sub_45F6(void) {
    LEFTMARGIN = 1;
    FILL(15, ixiFileName, ' ');
    movemem(srcStemLen, srcStemName, ixiFileName);
    movemem(4, ".IXI", ixiFileName + srcStemLen);
    initFile(&ixiFile, "IXREF ", ixiFileName);
    objBlk = 0;
    objByte = 0;
    FILL(15, objFileName, ' ');
    movemem(srcStemLen, srcStemName, objFileName);
    movemem(4, ".OBJ", objFileName + srcStemLen);
    initFile(&objFile, "OBJECT", objFileName);
    FILL(15, lstFileName, ' ');
    movemem(srcStemLen, srcStemName, lstFileName);
    movemem(4, ".LST", lstFileName + srcStemLen);
    initFile(&lstFile, "LIST ", lstFileName);
    initFile(&tx1File, "UT1 ", ":F1:PLMTX1.TMP ");
    initFile(&tx2File, "UT2 ", ":F1:PLMTX2.TMP ");
    initFile(&atFile, "AT  ", ":F1:PLMAT.TMP ");
    initFile(&nmsFile, "NAMES ", ":F1:PLMNMS.TMP ");
    initFile(&xrfFile, "XREF ", ":F1:PLMXRF.TMP ");
    IXREF = 0;
    IXREFSet = 0;
    PRINT = 0xff;
    PRINTSet = 0xff;
    XREF = 0;
    SYMBOLS = 0;
    DEBUG = 0;
    PAGING = 0xff;
    OBJECT = 0xff;
    OBJECTSet = 0xff;
    OPTIMIZE = 0xff;
    setDATE(" ", 1);
    setPAGELEN(57);
    setMarkerInfo(20, 45, 21);
    setPageNo(0);
    setMarginAndTabW(0xff, 4);
    setTitle(" ", 1);
    setPAGEWIDTH(120);
}


void sub_4767(void) {
    sub_4845();
    installKeywords();
    installBuiltins();
}

void installBuiltins(void) {
    static char aMemory[/*7*/] = "\6MEMORY";
    unsigned char *p;

    for (p = (unsigned char *)builtins; *p != 0; p += *p + 4) {
        lookup(p);
        createInfo(0, BUILTIN_T);
        setBuiltinId(p[*p+1]);
        setParamCnt(p[*p+2]);
        setDataType(p[*p+3]);
    }
    lookup((unsigned char *)aMemory);
    createInfo(0, BYTE_T);
    setInfoFlag(F_LABEL);
    setInfoFlag(F_MEMORY);
    setInfoFlag(F_ARRAY);
}


void installKeywords(void) {
    unsigned char *p;

    for (p = (unsigned char *)keywords; *p; p += *p + 2) {
        lookup(p);
        curSymbol_p->infoChain = (p[*p + 1] | 0xff00);
    }
}


void sub_4845(void) {
    word i;

    botMem = (word_3C34 < word_3C44) ? word_3C44 : word_3C34;
    botMem += 256;
    hashChains_p = (symbol_pt *)(topMem + 1 - 128);
    topSymbol = (Byte *)hashChains_p - 1;
    botSymbol = topSymbol + 1;
    topInfo = (botInfo = botMem) + 1;

    for (i = 0; i <= 63; i++)
        hashChains_p[i] = 0;
    setPageNo(1);
    localLabelCnt = 0;
    word_382A++;
    blockDepth = procChains[1] = procChains[0] = 0;
}


address MEMCHK(void) {
    return 0xf6c0;
}



void setMarkerInfo(Byte b1, Byte b2, Byte b3) {
    byte_3CFB = b1;
    byte_3CFC = b2;
    byte_3CFD = b3;
}

void setPageNo(word val) {
    pageNo = val - 1;
}


void setMarginAndTabW(Byte b1, Byte b2) {
    listingMargin = b1 - 1;
    tabWidth = b2;
}

void setTitle(const char *buf, Byte len) {
    if (len > 60)
        len = 60;
    movemem(len, buf, TITLE);
    TITLELEN = len;
}



