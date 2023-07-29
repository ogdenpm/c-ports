/****************************************************************************
 *  main4.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include "os.h"

// static char copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";
static byte objEOF[] = { 0xe, 1, 0, 0xf1 };

static byte b4304[]  = { 0x24, 0x24, 0x24, 0x24, 0x13, 0x13, 0x18, 0x18, 0x18, 0x18, 0x16, 0x2C,
                         0x15, 0x1F, 0x1F, 0x20, 0x20, 0x19, 0x19, 0x19, 0x19, 8,    8,    9,
                         9,    6,    7,    0x25, 0x25, 0x25, 0x25, 0x25, 0xA,  0xA,  0xB,  0xB,
                         0x14, 0x14, 0x14, 0x14, 0x14, 0x39, 0x1A, 0x1A, 0x1A, 0x1A };

static void Sub_3FC8() {
    if (PRINT) {
        EjectNext();
        lstStr("ISIS-II PL/M-80 " VERSION " COMPILATION OF MODULE ");
        infoIdx   = procInfo[1];
        curSym = GetSymbol();
        if (curSym != 0)
            lstStr(symtab[curSym].name->str);
        NewLineLst();
        if (OBJECT)
            lprintf("OBJECT MODULE PLACED IN %s\n", objFile.fNam);
        else
            lstStr("NO OBJECT MODULE REQUESTED\n");

        if (cmdLineCaptured == 1) {
            lstStr("COMPILER INVOKED BY:  ");
            printCmdLine(lstFile.fp); // modify to include TabLst(-23)
            linLft--;
            col = 0;
        }
    }
}

static void Sub_408B() {

    Sub_3FC8();
    stmtNo = 0;
    infoIdx = procInfo[1];
    baseAddr = putWord(&rec6_4[CONTENT_OFF], GetLinkVal());
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
    programErrCnt = linesRead = 0;
}

static void Sub_4162() {
    byte helperModId, endHelperId;

    if (!standAlone)
        return;
    for (helperModId = 0; helperModId <= 45; helperModId++) {
        helperId    = b42D6[helperModId];
        endHelperId = helperId + b42A8[helperModId];
        while (helperId < endHelperId) {
            if (helpers[helperId] != 0) {
                baseAddr = helpers[helperId];
                b969C    = b4304[helperModId];
                b969D    = b4273[b969C];
                Sub_5FE7(w4919[helperId], b4A03[helperId]);
                break;
            }
            helperId = helperId + 1;
        }
    }
} /* Sub_4162() */

static void Sub_4208() {
    if (haveModuleLevelUnit) {
        rec4[MODEND_TYPE] = 1;
        infoIdx                  = procInfo[1];
        putWord(&rec4[MODEND_OFF], GetLinkVal());
    } else {
        rec4[MODEND_TYPE] = 0;
        putWord(&rec4[MODEND_OFF], 0);
    }
    WriteRec(rec4, 0);
}

static void Sub_423C() {
    linesRead = lineNo;
    Sub_4208();
    vfReset(&utf1);

    if (OBJECT) {
        if (fwrite(objEOF, 1, 4, objFile.fp) != 4)
            IoError(objFile.fNam, "Write error");
        CloseF(&objFile);
    }
} /* Sub_423C() */

word Start4() {
    // rec24_2 is has different seg c.f. plm3a.c
    rec24_1[REC_DATA] = 2; // data seg
    rec24_2[REC_DATA] = 3; // stack seg

    dump(&utf1, "utf1_main4");
    vfRewind(&utf1);
    if (setjmp(exception) == 0) {
        Sub_408B();

        while (bo812B)
            Sub_54BA();
        Sub_4162();
        FlushRecs();
        EmitLinePrefix();
    }
    Sub_423C();
    if (IXREF)
        return 5; // Chain(overlay5);
    if (PRINT) {
        if (XREF || SYMBOLS)
            return 5; // Chain(overlay5);
        else
            LstModuleInfo();
    }
    EndCompile();
    Exit(0);
}
