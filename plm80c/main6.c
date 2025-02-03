/****************************************************************************
 *  main6.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "../shared/os.h"
#include "../shared/cmdline.h"
#include "plm.h"

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";
bool moreLines = true;
word lineCnt;

// lifted to file scope
static word itemArgs[4];

static void UpdateLineInfo() {
    word i;
    if (itemArgs[1] > 0 || itemArgs[2] == 0)
        itemArgs[3] = itemArgs[0];
    else {
        itemArgs[3] = itemArgs[2];
        itemArgs[2] = 0;
    }
    for (i = itemArgs[0]; i <= itemArgs[3]; i++) {
        EmitLinePrefix();
        lineCnt = i;
        stmtCnt = itemArgs[1];
        blkCnt  = itemArgs[2];
        GetSourceLine();
    }
}

static void SyntaxError_6() {
    errData.num  = itemArgs[0];
    errData.info = 0;
    errData.stmt = stmtNo;
    EmitError();
}

static void TokenError_6() {
    errData.num  = itemArgs[0];
    errData.info = itemArgs[1];
    errData.stmt = stmtNo;
    EmitError();
}

static void Error_6() {
    errData.num  = itemArgs[0];
    errData.info = itemArgs[1];
    errData.stmt = itemArgs[2];
    EmitError();
}

void MiscControl(vfile_t *txFile) {
    switch (cfCode) {
    case T2_LIST:
        listOff = false;
        break;
    case T2_NOLIST:
        listOff = true;
        break;
    case T2_CODE:
        codeOn = PRINT;
        break;
    case T2_NOCODE:
        codeOn = false;
        break;
    case T2_EJECT:
        if (listing)
            EjectNext();
        break;
    case T2_INCLUDE:
        EmitLinePrefix();
        srcFileTable[srcFileIdx++] = srcFil; // push current file
        OpenF(&srcFil, "SOURCE", includes[vfRword(txFile)], "rt");
        break;
    }
}

void T2Phase6() {
    itemArgs[0] = itemArgs[1] = itemArgs[2] = 0;
    cfCode                                  = Rd2Byte();

    if (cfCode != T2_INCLUDE)
        Rd2Buf(&itemArgs, (nodeControlMap[cfCode] & 3) * sizeof(itemArgs[0]));
    if (cfCode == T2_LINEINFO)
        UpdateLineInfo();
    else if (cfCode == T2_STMTCNT)
        stmtNo = itemArgs[0];
    else if (cfCode == T2_SYNTAXERROR)
        SyntaxError_6();
    else if (cfCode == T2_TOKENERROR)
        TokenError_6();
    else if (cfCode == T2_ERROR)
        Error_6();
    else if (T2_LIST <= cfCode && cfCode <= T2_INCLUDE)
        MiscControl(&utf2);
    else if (cfCode == T2_EOF)
        moreLines = false;
}





static void NoObjSignOn() {
    if (PRINT) {
        EjectNext();
        lstStr("INTEL PL/M-80 " VERSION " COMPILATION OF MODULE ");
        curSym = procInfo[1]->sym;
        if (curSym)
            lprintf("%s", curSym->name->str);
        lprintf("\nNO OBJECT MODULE %s\nCOMPILER INVOKED BY: ",
                OBJECT ? "GENERATED" : "REQUESTED");
        // replace with PrintCmdLine code
        linLft -= (byte)printCmdLine(lstFile.fp, PWIDTH, 23);
        col = 0;
        SetSkipLst(3);
    }
}

static void initPhase6() {
    vfRewind(&utf2);
    stmtNo = 0;
    if (PRINT)
        unwindInclude(); // probably not needed
    //info = procInfo[1];
    SetSkipLst(3);
    SetMarkerInfo(11, 15);
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

word Start6() {
    if (setjmp(exception) == 0) {
        initPhase6();
        NoObjSignOn();
        while (moreLines) {
            T2Phase6();
        }

        EmitLinePrefix();
    }
    linesRead = lineCnt;
    if (PRINT || IXREF) {
        if (XREF || SYMBOLS || IXREF)
            return 5; // generate xref
        else
            LstModuleInfo();
    }
    EndCompile();
    Exit(programErrCnt != 0);
}

