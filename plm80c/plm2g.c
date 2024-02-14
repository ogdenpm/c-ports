/****************************************************************************
 *  plm2g.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"
#include "int.h"

static byte curParamCnt;
static byte bC2D3;
static word wC2D4;

void FindParamInfo(byte arg1b) {
    info = blk[activeGrpCnt].info;
    while (arg1b-- != 0)
        AdvNxtInfo();
}

static void saveParam(byte paramNo, byte irReg) {
    if (info->type == ADDRESS_T) {
        iCodeArgs[0] = irReg;
        EncodeFragData(CF_MOVMRPR);
        codeSize += 3;
    } else {
        iCodeArgs[0] = irReg;
        EncodeFragData(CF_MOVMLR);
        codeSize++;
    }
    if (paramNo != curParamCnt) {
        EncodeFragData(CF_DCXH);
        codeSize++;
    }
}

void paramsToMem() {
    byte paramOrder;
    byte reg = (paramOrder = curParamCnt) == 1 ? IR_B : IR_D;

    for (byte paramNo = 1; paramNo <= curParamCnt; paramNo++) {
        FindParamInfo(paramOrder); // popping stack in reverse parameter order
        if (paramNo == 2)
            reg = IR_B;
        else if (paramNo == 3) {
            iCodeArgs[0] = IR_D; /*  pop d */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_POP);
            iCodeArgs[0] = IR_B; /*  pop b */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_POP);
            codeSize += 2;
        } else if (paramNo > 3) {
            iCodeArgs[0] = IR_B; /*  pop b */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_POP);
            codeSize++;
        }
        saveParam(paramNo, reg);
        paramOrder--;
    }
    if (curParamCnt > 2) {
        iCodeArgs[0] = IR_D; /*  push d */
        iCodeArgs[1] = LOC_REG;
        EncodeFragData(CF_PUSH);
        codeSize++;
    }
}

void GetEAtoHL(word arg1w) {
    iCodeArgs[0] = IR_SR;    // get effective address into HL
    iCodeArgs[1] = arg1w;    // -ve offset from stack
    EncodeFragData(CF_SA2HL);
    codeSize += 4;
}

void ReserveStkLocals(word localSize) {
    if (localSize <= 10) {
        if (localSize & 1) {
            EncodeFragData(CF_DCXSP);
            codeSize++;
        }
        while (localSize > 1) {
            iCodeArgs[0] = IR_H; /*  push h */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_PUSH);
            codeSize++;
            localSize -= 2;
        }
    } else {
        GetEAtoHL(-localSize);
        EncodeFragData(CF_SPHL);
        codeSize++;
    }
}

void Inxh() {
    iCodeArgs[0] = IR_H;
    EncodeFragData(CF_INX);
    codeSize++;
}

void OpB(byte icode) {
    iCodeArgs[0] = IR_B;
    EncodeFragData(icode);
    codeSize++;
}

void OpD(byte icode) {
    iCodeArgs[0] = IR_D;
    EncodeFragData(icode);
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

static void Sub_975F(byte irReg) {
    iCodeArgs[0] = irReg;
    iCodeArgs[1] = LOC_REG;
    EncodeFragData(CF_PUSH);
    codeSize++;
    if (info->type == BYTE_T) {
        EncodeFragData(CF_INXSP);
        codeSize++;
    }
}

void Sub_978E() {
    if ((bC2D3 = curParamCnt) > 2)
        GetEAtoHL(wC2D4);
    byte irReg = curParamCnt == 1 ? IR_B : IR_D;

    for (byte paramNo = 1; paramNo <= curParamCnt; paramNo++) {
        FindParamInfo(bC2D3);
        if (paramNo > 3)
            Sub_9706();
        else if (paramNo == 3) {
            MovDem();
            Sub_9706();
        } else if (info->type == BYTE_T) {
            iCodeArgs[0] = irReg;
            EncodeFragData(CF_MOVHRLR);
            codeSize++;
        }
        Sub_975F(irReg);
        irReg = IR_B;
        bC2D3--;
    }
}

void Sub_981C() {
    curParamCnt = info->paramCnt;
    if ((info->flag & F_INTERRUPT)) {
        for (int8_t irReg = IR_H; irReg >= IR_PSW; irReg--) {
            iCodeArgs[0] = irReg;
            iCodeArgs[1] = LOC_REG; /*  push h, push d, push b, push psw */
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
            ReserveStkLocals(wC2D4);
            Sub_978E();
        } else
            ReserveStkLocals(wC1C7);

        if (curParamCnt > 2)
            wC1C7 += (curParamCnt - 2) * 2;

        stackUsage = 0;
    } else {
        if (curParamCnt > 0) {
            FindParamInfo(curParamCnt); /*  locate info for first param */
            iCodeArgs[0] = IR_H;
            iCodeArgs[1] = LOC_VAR;
            iCodeArgs[2] = info->type == ADDRESS_T ? 1 : 0; // offset
            iCodeArgs[3] = ToIdx(info); /*  info for first param */
            EncodeFragData(CF_LXI);
            paramsToMem();
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
    byte nodeType;

    if (curNodeType == T2_LABELDEF) {
        boC1CC = false;
        info = FromIdx(tx2[tx2qp].left);
        info->linkVal = codeSize;
    } else if (curNodeType == T2_LOCALLABEL) {
        boC1CC                     = false;
        localLabels[tx2[tx2qp].left] = codeSize;
        procIds[tx2[tx2qp].left]     = curExtProcId;
    } else if (curNodeType == T2_CASELABEL) {
        localLabels[tx2[tx2qp].left] = codeSize;
        procIds[tx2[tx2qp].left]     = curExtProcId;
        newCase(tx2[tx2qp].left);
    } else if (curNodeType == T2_JMP || curNodeType == T2_JNC || curNodeType == T2_JNZ || curNodeType == T2_GOTO) {
        nodeType = tx2[tx2qp - 1].nodeType;
        if (nodeType == T2_RETURN || nodeType == T2_RETURNBYTE || nodeType == T2_RETURNWORD || nodeType == T2_GOTO)
            return;
        Sub_5795(0);
    } else if (curNodeType == T2_INPUT || (T2_SIGN <= curNodeType && curNodeType <= T2_CARRY)) {
        curExprLoc[0] = 0;
        curExprLoc[1] = 0;
        exprLoc[0] = 8;
        exprLoc[1] = 8;
        Sub_597E();
        Sub_5D6B(0);
        bC045[0]        = 0;
        bC04E[0]        = tx2qp;
        boC057[0]       = 0;
        bC0A8[0]        = 0;
        tx2[tx2qp].exprAttr = BYTE_A;
        tx2[tx2qp].exprLoc = LOC_SPECIAL;
    } else if (curNodeType == T2_STMTCNT) {
        bool found = false;
        for (int j = tx2qp + 1; tx2[j].nodeType != T2_STMTCNT && tx2[j].nodeType != T2_EOF && j < 255; j++) {
            if ((nodeControlMap[tx2[j].nodeType] & 0x20) == 0 || tx2[j].nodeType == T2_MODULE) {
                found = true;
                break;
            }
        }
        if (!found) {
            curNodeType         = CF_134;
            tx2[tx2qp].nodeType = CF_134;
        }
    }
    EmitTopItem();
    codeSize += (codeAttrLen[curNodeType] & 0x1f);
}
