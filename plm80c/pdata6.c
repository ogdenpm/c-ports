/****************************************************************************
 *  pdata6.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
bool b7AD9;
byte b7ADA;
// bool listing; in pdata4.c
// bool listOff;
// bool codeOn;
bool b7AE4 = true;
word lineCnt;
// word stmtCnt; in pdata4.c
// word blkCnt;	in pdata4.c
// word stmtNo;	in pdata4.c
// byte lstLineLen; in pdata4.c
// byte lstbuf[130]; use larger version in main5.c
byte srcbuf[2048];

