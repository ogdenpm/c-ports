/****************************************************************************
 *  plm6a.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include "os.h"

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
        srcFileTable[srcFileIdx++] = srcFil;    // push current file
        OpenF(&srcFil, "SOURCE", includes[vfRword(txFile)], "rt");
        break;
    }
}

void Sub_42E7() {

    itemArgs[0] = itemArgs[1] = itemArgs[2] = 0;
    cfCode = Rd2Byte();

    if (cfCode != T2_INCLUDE)
        Rd2Buf(&itemArgs, (b5124[cfCode] & 3) * sizeof(itemArgs[0]));
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
        b7AE4 = false;
}
