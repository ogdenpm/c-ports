/****************************************************************************
 *  main.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"
// static byte copyRight[] = "[C] 1976, 1977, 1982 INTEL CORP";

void Start() {
    SignOnAndGetSourceName();
    InitKeywordsAndBuiltins();

    Start0(); // lex pass
    if (Start1() == 2) {
        Start2();
        if (Start3() == 4)  // only returns if listing/object or XREF needed
            Start4(); // listing/object generation only returns if XREF needed
    } else
        Start6(); // Error module, only returns if XREF needed
    Start5();     // XREF module
}

void usage() {
}