/****************************************************************************
 *  main4.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "../shared/os.h"
#include "../shared/cmdline.h"
#include "plm.h"

// static char copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";
static byte objEOF[] = { 0xe, 1, 0, 0xf1 };

static byte b4304[]  = { 0x24, 0x24, 0x24, 0x24, 0x13, 0x13, 0x18, 0x18, 0x18, 0x18, 0x16, 0x2C,
                         0x15, 0x1F, 0x1F, 0x20, 0x20, 0x19, 0x19, 0x19, 0x19, 8,    8,    9,
                         9,    6,    7,    0x25, 0x25, 0x25, 0x25, 0x25, 0xA,  0xA,  0xB,  0xB,
                         0x14, 0x14, 0x14, 0x14, 0x14, 0x39, 0x1A, 0x1A, 0x1A, 0x1A };

static void Sub_3FC8() {
    if (PRINT) {
        EjectNext();
        lstStr("PL/M-80 " VERSION " COMPILATION OF MODULE ");
        curSym = procInfo[1]->sym;
        if (curSym)
            lstStr(curSym->name->str);
        NewLineLst();
        if (OBJECT)
            lprintf("OBJECT MODULE PLACED IN %s\n", objFile.fNam);
        else
            lstStr("NO OBJECT MODULE REQUESTED\n");

        if (cmdLineCaptured == 1) {
            lstStr("COMPILER INVOKED BY: ");
            linLft -= (byte)printCmdLine(lstFile.fp, PWIDTH, 23);
            col = 0;
        }
    }
}

static void Sub_408B() {
    Sub_3FC8();
    stmtNo   = 0;
    baseAddr = putWord(&recExec[CONTENT_OFF], procInfo[1]->addr);
    SetSkipLst(3);
    SetMarkerInfo(11, 15);
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
    if (!standAlone)
        return;
    for (byte helperMod = 0; helperMod < 46; helperMod++) {
        helperId    = modHelperId[helperMod];          // helper module first helper id
        byte endHelperId = helperId + modHelperIdCnt[helperMod]; // helper module last helpers id (+1)
        while (helperId < endHelperId) {
            if (helperAddr[helperId]) {
                baseAddr = helperAddr[helperId];
                b969C    = b4304[helperMod];
                b969D    = b4273[b969C];
                EmitCodeSeq(helperStart[helperId], helperLen[helperId]);
                break;
            }
            helperId++;
        }
    }
} /* Sub_4162() */

static void Sub_4208() {
    if (haveModuleLevelUnit) {
        recEnd[MODEND_TYPE] = 1;
        putWord(&recEnd[MODEND_OFF], procInfo[1]->addr);
    } else {
        recEnd[MODEND_TYPE] = 0;
        putWord(&recEnd[MODEND_OFF], 0);
    }
    WriteRec(recEnd, 0);
}

static void Sub_423C() {
    linesRead = lineNo;
    Sub_4208();

    if (OBJECT) {
        if (fwrite(objEOF, 1, 4, objFile.fp) != 4)
            IoError(objFile.fNam, "Write error");
        CloseF(&objFile);
    }
} /* Sub_423C() */

word Start4() {
    // rec24_2 is has different seg c.f. plm3a.c
    recCodeFixup[REC_DATA] = 2; // data seg
    recDataFixup[REC_DATA] = 3; // stack seg

 //   dump(&utf1, "utf1_main4"); // diagnostic dump
    vfRewind(&utf1);
    if (setjmp(exception) == 0) {
        Sub_408B();

        while (morePass4)
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
