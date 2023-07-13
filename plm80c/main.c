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
#include "os.h"
//static byte copyRight[] = "[C] 1976, 1977, 1982 INTEL CORP";

static byte stateMain;

void FatalError_main(byte err)
{
    if (err == ERR83)
        Fatal("DYNAMIC STORAGE OVERFLOW", 24);
    Fatal("UNKNOWN FATAL ERROR", 19);
    Exit(1);
}

int ovl = 255;

void Start()
{
    initMem();
    stateMain = 10;
    SignOnAndGetSourceName();
    stateMain = 15;
    InitKeywordsAndBuiltins();

    ovl = 0;
    while (1) {
        switch (ovl) {
        case 0:
            ovl = Start0();
            break;
        case 1:
            ovl = Start1();
            break;
        case 2:
            ovl = Start2();
            break;
        case 3:
            ovl = Start3();
            break;
        case 4:
            ovl = Start4();
            break;
        case 5:
            ovl = Start5();
            break;
        case 6:
            ovl = Start6();
            break;
        default:
            fprintf(stderr, "unexpected overlay %d\n", ovl);
            Exit(1);
        }
    }
}

void PFatalError(byte errNum) {
    switch (ovl) {
    case 255:
        FatalError_main(errNum);
        break;
    case 0:
        Wr1FatalError(errNum);
        break;
    case 1:
        FatalError_ov1(errNum);
        break;
    case 2:
    case 3:
    case 5:
    case 4:
    case 6:
        FatalError_ov46(errNum);
        break;
    }
}


void usage() {
}