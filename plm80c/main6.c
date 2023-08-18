/****************************************************************************
 *  main6.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";

static void Sub_3F96() {
    if (PRINT) {
        EjectNext();
        lstStr("ISIS-II PL/M-80 " VERSION " COMILATION OF MODULE ");
        SetInfo(procInfo[1]);
        curSym  = info->sym;
        if (curSym)
            lprintf("%s", symtab[curSym].name->str);
        lprintf("\nNO OBJECT MODULE %s\nCOMPILER INVOKED BY:\n",
                OBJECT ? "GENERATED" : "REQUESTED");
        // replace with PrintCmdLine code
        linLft -= (byte)printCmdLine(lstFile.fp, PWIDTH);
        col = 0;
        SetSkipLst(3);
    }
}

static void Sub_404A() {
    b7AD9 = PRINT | OBJECT;

    vfReset(&utf1);
    vfRewind(&utf2);
    stmtNo = 0;
    if (PRINT)
        unwindInclude(); // probably not needed
    SetInfo(procInfo[1]);
    SetSkipLst(3);
    SetMarkerInfo(11, '-', 15);
    if (fatalErrorCode > 0) {
        errData.stmt = 0;
        errData.info = 0;
        errData.num  = fatalErrorCode;
        EmitError();
        SetSkipLst(2);
    }
    listing       = PRINT;
    listOff       = false;
    codeOn        = false;
    programErrCnt = linesRead = csegSize = 0;
}

static void Sub_41B6() {
    vfReset(&atf);
    vfReset(&utf2);

    linesRead = lineCnt;
}

word Start6() {
    if (setjmp(exception) == 0) {
        Sub_404A();
        // if (b7AD9 || IXREF) {
        //     ReloadSymbols();
        // }
        Sub_3F96();
        while (b7AE4) {
            Sub_42E7();
        }

        EmitLinePrefix();
    }
    Sub_41B6();
    if (PRINT || IXREF) {
        if (XREF || SYMBOLS || IXREF)
            return 5; // Chain(&overlay5);
        else
            LstModuleInfo();
    }
    EndCompile();
    Exit(programErrCnt != 0);
}
/* split file */