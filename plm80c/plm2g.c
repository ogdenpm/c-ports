/****************************************************************************
 *  plm2g.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

static byte curParamCnt;
static byte reg;
static byte bC2D3;
static word wC2D4;

#define R_BC 1
#define R_DE 2

void FindParamInfo(byte arg1b) {
    SetInfo(blk[blkSP].info);
    while (arg1b-- != 0)
        AdvNxtInfo();
}

static void saveParam(byte paramNo) {
    if (info->type == ADDRESS_T) {
        wC1DC[0] = reg;
        EncodeFragData(CF_MOVMRPR);
        codeSize += 3;
    } else {
        wC1DC[0] = reg;
        EncodeFragData(CF_MOVMLR);
        codeSize++;
    }
    if (paramNo != curParamCnt) {
        EncodeFragData(CF_DCXH);
        codeSize++;
    }
}

void Sub_9560() {
    byte paramOrder;

    if ((paramOrder = curParamCnt) == 1)
        reg = R_BC;
    else
        reg = R_DE;
    for (byte paramNo = 1; paramNo <= curParamCnt; paramNo++) {
        FindParamInfo(paramOrder); // popping stack in reverse parameter order
        if (paramNo == 2)
            reg = R_BC;
        else if (paramNo == 3) {
            wC1DC[0] = R_DE; /*  pop d */
            wC1DC[1] = 8;
            EncodeFragData(CF_POP);
            wC1DC[0] = R_BC; /*  pop b */
            wC1DC[1] = 8;
            EncodeFragData(CF_POP);
            codeSize += 2;
        } else if (paramNo > 3) {
            wC1DC[0] = R_BC; /*  pop b */
            wC1DC[1] = 8;
            EncodeFragData(CF_POP);
            codeSize++;
        }
        saveParam(paramNo);
        paramOrder--;
    }
    if (curParamCnt > 2) {
        wC1DC[0] = 2; /*  push d */
        wC1DC[1] = 8;
        EncodeFragData(CF_PUSH);
        codeSize++;
    }
}

void Sub_9624(word arg1w) {
    wC1DC[0] = 9;
    wC1DC[1] = arg1w;
    EncodeFragData(CF_6);
    codeSize += 4;
}

void Sub_9646(word arg1w) {
    if ((arg1w >> 1) + (arg1w & 1) <= 5) {
        if (arg1w & 1) {
            EncodeFragData(CF_DCXSP);
            codeSize++;
        }
        while (arg1w > 1) {
            wC1DC[0] = 3; /*  push h */
            wC1DC[1] = 8;
            EncodeFragData(CF_PUSH);
            codeSize = codeSize + 1;
            arg1w -= 2;
        }
    } else {
        Sub_9624(-arg1w);
        EncodeFragData(CF_SPHL);
        codeSize++;
    }
}

void Inxh() {
    wC1DC[0] = 3;
    EncodeFragData(CF_INX);
    codeSize++;
}

void OpB(byte arg1b) {
    wC1DC[0] = 1;
    EncodeFragData(arg1b);
    codeSize++;
}

void OpD(byte arg1b) {
    wC1DC[0] = 2;
    EncodeFragData(arg1b);
    codeSize++;
}

void Sub_9706() {
    Inxh();
    if (info->type == ADDRESS_T) {
        OpB(CF_MOVLRM);
        if (bC2D3 == 1)
            OpD(CF_MOVMLR);

        Inxh();
        OpB(CF_MOVHRM);
    } else {
        OpB(CF_MOVHRM);
        if (bC2D3 == 1)
            OpD(CF_MOVMLR);
        Inxh();
    }
    if (bC2D3 == 1)
        OpD(CF_MOVMHR);
}

void MovDem() {
    OpD(CF_MOVRPM);
    codeSize += 2;
}

void Sub_975F() {
    wC1DC[0] = reg;
    wC1DC[1] = 8;
    EncodeFragData(CF_PUSH);
    codeSize++;
    if (info->type == BYTE_T) {
        EncodeFragData(CF_INXSP);
        codeSize++;
    }
}

void Sub_978E() {
    if ((bC2D3 = curParamCnt) > 2)
        Sub_9624(wC2D4);
    if (curParamCnt == 1)
        reg = 1;
    else
        reg = 2;
    for (byte paramNo = 1; paramNo <= curParamCnt; paramNo++) {
        FindParamInfo(bC2D3);
        if (paramNo > 3)
            Sub_9706();
        else if (paramNo == 3) {
            MovDem();
            Sub_9706();
        } else if (info->type == BYTE_T) {
            wC1DC[0] = reg;
            EncodeFragData(CF_MOVHRLR);
            codeSize++;
        }
        Sub_975F();
        reg = 1;
        bC2D3--;
    }
}

void Sub_981C() {
    byte i, j;
    curParamCnt = info->paramCnt;
    if ((info->flag & F_INTERRUPT)) {
        for (j = 0; j <= 3; j++) {
            wC1DC[0] = 3 - j;
            wC1DC[1] = 8; /*  push h, push d, push b, push psw */
            EncodeFragData(CF_PUSH);
            codeSize++;
        }
    }
    if ((info->flag & F_REENTRANT)) {
        wC1C7 = info->totalSize;
        ;
        if (curParamCnt > 0) {
            FindParamInfo(curParamCnt);
            wC2D4 = wC1C7 - info->linkVal - 1;
            if (info->type == ADDRESS_T)
                wC2D4--;
            Sub_9646(wC2D4);
            Sub_978E();
        } else
            Sub_9646(wC1C7);

        if (curParamCnt > 2)
            wC1C7 += (curParamCnt - 2) * 2;

        stackUsage = 0;
    } else {
        if (curParamCnt > 0) {
            FindParamInfo(curParamCnt); /*  locate info for first param */
            if (info->type == ADDRESS_T)
                i = 1;
            else
                i = 0;
            wC1DC[0] = 3;
            wC1DC[1] = 0xb;
            wC1DC[2] = i;
            wC1DC[3] = infoIdx; /*  info for first param */
            EncodeFragData(CF_7);
            Sub_9560();
            codeSize += 3;
        }
        wC1C7 = 0;
        if (curParamCnt > 2)
            stackUsage = (curParamCnt - 2) * 2;
        else
            stackUsage = 0;
    }
}

void Sub_994D() {
    byte opc;

    if (curOp == T2_LABELDEF) {
        boC1CC = false;
        SetInfo(tx2[tx2qp].op1);
        info->linkVal = codeSize;
    } else if (curOp == T2_LOCALLABEL) {
        boC1CC                     = false;
        localLabels[tx2[tx2qp].op1] = codeSize;
        procIds[tx2[tx2qp].op1]     = curExtProcId;
    } else if (curOp == T2_CASELABEL) {
        localLabels[tx2[tx2qp].op1] = codeSize;
        procIds[tx2[tx2qp].op1]     = curExtProcId;
        newCase(tx2[tx2qp].op1);
    } else if (curOp == T2_JMP || curOp == T2_JNC || curOp == T2_JNZ || curOp == T2_GOTO) {
        opc = tx2[tx2qp - 1].opc;
        if (opc == T2_RETURN || opc == T2_RETURNBYTE || opc == T2_RETURNWORD || opc == T2_GOTO)
            return;
        Sub_5795(0);
    } else if (curOp == T2_INPUT || (T2_SIGN <= curOp && curOp <= T2_CARRY)) {
        bC0B7[0] = 0;
        bC0B7[1] = 0;
        bC0B5[0] = 8;
        bC0B5[1] = 8;
        Sub_597E();
        Sub_5D6B(0);
        bC045[0]        = 0;
        bC04E[0]        = tx2qp;
        boC057[0]       = 0;
        bC0A8[0]        = 0;
        tx2[tx2qp].aux1 = 0;
        tx2[tx2qp].aux2 = 9;
    } else if (curOp == T2_STMTCNT) {
        bool found = false;
        for (int j = tx2qp + 1; tx2[j].opc != T2_STMTCNT && tx2[j].opc != T2_EOF && j < 255; j++) {
            if ((b5124[tx2[j].opc] & 0x20) == 0 || tx2[j].opc == T2_MODULE) {
                found = true;
                break;
            }
        }
        if (!found) {
            curOp         = CF_134;
            tx2[tx2qp].opc = CF_134;
        }
    }
    EmitTopItem();
    codeSize += (b43F8[curOp] & 0x1f);
}
