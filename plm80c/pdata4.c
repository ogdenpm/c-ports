/****************************************************************************
 *  pdata4.c: part of the C port of Intel's ISIS-II plm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

// uint8_t helperModId; made into local var
// uint8_t endHelperId;
bool listing;
bool listOff;
bool codeOn;
line_t line;

char locPStr[33]; /* used to hold symbol name + '\0' */
err_t errData;
bool morePass4 = true;
uint16_t baseAddr;
uint8_t cfCode;
uint16_t lineNo;
uint16_t stmtCnt;
uint16_t blkCnt;
uint16_t stmtNo;
bool linePrefixChecked = true;
bool linePrefixEmitted = true;
uint16_t lstLineLen;
char lstLine[MAXLINE + 1]; // allow for trailing '\0'

uint8_t recExec[1024]      = { 6, 0, 0, S_CODE };
uint8_t recSelfFixup[1022] = { 0x22, 0, 0, FIX_HILO };
uint8_t recCodeFixup[1022] = { 0x24, 0, 0, S_CODE, FIX_HILO };
uint8_t recExtFixup[1022]  = { 0x20, 0, 0, FIX_HILO };
uint8_t recLineNum[1024]   = { 8, 0, 0, S_CODE };
uint8_t recEnd[8]          = { 4, 4, 0, 0, S_CODE };
uint8_t b9692;
uint8_t helperId;
uint8_t b969C;
uint8_t b969D;
uint16_t wValAry[4];
pstr_t *sValAry[4];
char valPStr[38];
uint8_t fixupType;
uint16_t locLabelNum;
uint16_t curExtId;
char commentPStr[42] = "\0; ";

uint8_t opByteCnt;
uint8_t opBytes[3];
uint8_t fixupSelector;
