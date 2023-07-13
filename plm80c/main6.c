/****************************************************************************
 *  main6.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include "os.h"

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";

void Sub_3F96() {
    if (PRINT) {
        EjectNext();
        lstStr("ISIS-II PL/M-80 " VERSION " COMILATION OF MODULE ");
        curInfoP   = botInfo + procInfo[1];
        curSymbolP = GetSymbol();
        sym_t *sym = SymbolP(curSymbolP);
        if (curSymbolP)
            lprintf("%.*s", sym->name.len, sym->name.str);
        lprintf("\nNO OBJECT MODULE %s\nCOMPILER INVOKED BY:", OBJECT ? "GENERATED" : "REQUESTED");
        lstStr("COMPILER INVOKED by:  ");
        // replace with PrintCmdLine code
        cmdLineP = startCmdLineP;
        while (cmdLineP != 0) {
            TabLst(-23);
            lstStrCh(CmdP(cmdLineP)->pstr.str, '\r');
            cmdLineP = CmdP(cmdLineP)->link;
        }
        NewLineLst();
        SetSkipLst(3);
    }
}



byte lstBuf_6[2048];

void Sub_404A() {
    b7AD9 = PRINT | OBJECT;

    vfReset(&utf1);
    vfRewind(&utf2);
    if (b7AD9 || IXREF)
        vfRewind(&nmsf);
    stmtNo = 0;
    if (PRINT) {
        srcFileIdx = 0;
        InitF(&srcFil, "SOURCE", srcFileTable[srcFileIdx].fNam);
    }
    curInfoP = procInfo[1] + botInfo;
    SetSkipLst(3);
    SetMarkerInfo(11, '-', 15);
    if (fatalErrorCode > 0) {
        errData.stmt = errData.info = 0;
        errData.num                 = fatalErrorCode;
        EmitError();
        SetSkipLst(2);
    }
    listing       = PRINT;
    listOff       = false;
    codeOn        = false;
    programErrCnt = linesRead = csegSize = 0;
}

void Sub_41B6() {
    vfReset(&atf);
    vfReset(&utf2);

    if (b7AD9 || IXREF)
        vfReset(&nmsf);

    linesRead = lineCnt;
}

word Start6() {
    if (setjmp(exception) == 0) {
        Sub_404A();
        if (b7AD9 || IXREF) {
            ReloadSymbols();
        }
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
