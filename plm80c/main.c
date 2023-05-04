/****************************************************************************
 *  main.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
//static byte copyRight[] = "[C] 1976, 1977, 1982 INTEL CORP";

static byte stateMain;

void FatalError_main(byte err)
{
    if (err == ERR83)
        Fatal("DYNAMIC STORAGE OVERFLOW", 24);
    Fatal("UNKNOWN FATAL ERROR", 19);
    Exit();
}

word Start()
{
    stateMain = 10;
    SignOnAndGetSourceName();
    stateMain = 15;
    InitKeywordsAndBuiltins();
    return 0;   // Chain(invokeName);
}

