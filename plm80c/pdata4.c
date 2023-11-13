/****************************************************************************
 *  pdata4.c: part of the C port of Intel's ISIS-II plm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

// byte helperModId; made into local var
// byte endHelperId;
bool listing;
bool listOff;
bool codeOn;
line_t line;

char locPStr[33]; /* used to hold symbol name + '\0' */
err_t errData;
bool bo812B = true;
word baseAddr;
byte cfCode;
word lineNo;
word stmtCnt;
word blkCnt;
word stmtNo;
bool linePrefixChecked = true;
bool linePrefixEmitted = true;
uint16_t lstLineLen;
char lstLine[MAXLINE + 1]; // allow for trailing '\0'

byte recExec[1024]      = { 6, 0, 0, S_CODE };
byte recSelfFixup[1022] = { 0x22, 0, 0, FIX_HILO };
byte recCodeFixup[1022] = { 0x24, 0, 0, S_CODE, FIX_HILO };
byte recExtFixup[1022]  = { 0x20, 0, 0, FIX_HILO };
byte recLineNum[1024]   = { 8, 0, 0, S_CODE };
byte recEnd[8]          = { 4, 4, 0, 0, S_CODE };
byte b9692;
byte helperId;
byte b969C;
byte b969D;
word wValAry[4];
pstr_t *sValAry[4];
char valPStr[38];
byte fixupType;
word locLabelNum;
word curExtId;
char commentPStr[42] = "\0; ";

byte opByteCnt;
byte opBytes[3];
byte fixupSelector;
