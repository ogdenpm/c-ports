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

static uint8_t curParamCnt;
static uint8_t remainingParamCount;
static uint16_t firstParamStackOffset;

void FindParamInfo(uint8_t arg1b) {
    info = blk[activeGrpCnt].info;
    while (arg1b-- != 0)
        AdvNxtInfo();
}

static void SaveParameterToMemory(uint8_t paramNo, uint8_t irReg) {
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

void CopyParametersToMemory() {
    uint8_t paramOrder;
    uint8_t currentReg = (paramOrder = curParamCnt) == 1 ? IR_B : IR_D;

    for (uint8_t paramNo = 1; paramNo <= curParamCnt; paramNo++) {
        FindParamInfo(paramOrder); // popping stack in reverse parameter order
        if (paramNo == 2)
            currentReg = IR_B;
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
        SaveParameterToMemory(paramNo, currentReg);
        paramOrder--;
    }
    if (curParamCnt > 2) {
        iCodeArgs[0] = IR_D; /*  push d */
        iCodeArgs[1] = LOC_REG;
        EncodeFragData(CF_PUSH);
        codeSize++;
    }
}

void LoadEffectiveAddressToHL(uint16_t stackOffset) {
    iCodeArgs[0] = IR_SR;    // get effective address into HL
    iCodeArgs[1] = stackOffset;    // -ve offset from stack
    EncodeFragData(CF_SA2HL);
    codeSize += 4;
}

void AllocateLocalVariables(uint16_t localSize) {
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
        LoadEffectiveAddressToHL(-localSize);
        EncodeFragData(CF_SPHL);
        codeSize++;
    }
}

void IncrementHL() {
    iCodeArgs[0] = IR_H;
    EncodeFragData(CF_INX);
    codeSize++;
}

void EmitRegisterB(uint8_t fragmentCode) {
    iCodeArgs[0] = IR_B;
    EncodeFragData(fragmentCode);
    codeSize++;
}

void EmitRegisterD(uint8_t fragmentCode) {
    iCodeArgs[0] = IR_D;
    EncodeFragData(fragmentCode);
    codeSize++;
}

void CopyParameterFromHL() {
    IncrementHL();
    if (info->type == ADDRESS_T) {
        EmitRegisterB(CF_MOVLRM);
        if (remainingParamCount == 1)
            EmitRegisterD(CF_MOVMLR);

        IncrementHL();
        EmitRegisterB(CF_MOVHRM);
    } else {
        EmitRegisterB(CF_MOVHRM);
        if (remainingParamCount == 1)
            EmitRegisterD(CF_MOVMLR);
        IncrementHL();
    }
    if (remainingParamCount == 1)
        EmitRegisterD(CF_MOVMHR);
}

void LoadDEFromMemory() {
    EmitRegisterD(CF_MOVRPM);
    codeSize += 2;
}

static void PushParameterValue(uint8_t irReg) {
    iCodeArgs[0] = irReg;
    iCodeArgs[1] = LOC_REG;
    EncodeFragData(CF_PUSH);
    codeSize++;
    if (info->type == BYTE_T) {
        EncodeFragData(CF_INXSP);
        codeSize++;
    }
}

void PushParametersToStack() {
    if ((remainingParamCount = curParamCnt) > 2)
        LoadEffectiveAddressToHL(firstParamStackOffset);
    uint8_t irReg = curParamCnt == 1 ? IR_B : IR_D;

    for (uint8_t paramNo = 1; paramNo <= curParamCnt; paramNo++) {
        FindParamInfo(remainingParamCount);
        if (paramNo > 3)
            CopyParameterFromHL();
        else if (paramNo == 3) {
            LoadDEFromMemory();
            CopyParameterFromHL();
        } else if (info->type == BYTE_T) {
            iCodeArgs[0] = irReg;
            EncodeFragData(CF_MOVHRLR);
            codeSize++;
        }
        PushParameterValue(irReg);
        irReg = IR_B;
        remainingParamCount--;
    }
}

void GenerateProcedureEntry() {
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
        localVariableSize = info->totalSize;
        ;
        if (curParamCnt > 0) {
            FindParamInfo(curParamCnt);
            firstParamStackOffset = localVariableSize - info->linkVal - 1;
            if (info->type == ADDRESS_T)
                firstParamStackOffset--;
            AllocateLocalVariables(firstParamStackOffset);
            PushParametersToStack();
        } else
            AllocateLocalVariables(localVariableSize);

        if (curParamCnt > 2)
            localVariableSize += (curParamCnt - 2) * 2;

        stackUsage = 0;
    } else {
        if (curParamCnt > 0) {
            FindParamInfo(curParamCnt); /*  locate info for first param */
            iCodeArgs[0] = IR_H;
            iCodeArgs[1] = LOC_VAR;
            iCodeArgs[2] = info->type == ADDRESS_T ? 1 : 0; // offset
            iCodeArgs[3] = ToIdx(info); /*  info for first param */
            EncodeFragData(CF_LXI);
            CopyParametersToMemory();
            codeSize += 3;
        }
        localVariableSize = 0;
        if (curParamCnt > 2)
            stackUsage = (curParamCnt - 2) * 2;
        else
            stackUsage = 0;
    }
}

void ProcessSpecialNodes() {
    uint8_t nodeType;

    if (curNodeType == T2_LABELDEF) {
        returnGenerated = false;
        info = FromIdx(tx2[tx2qp].left);
        info->linkVal = codeSize;
    } else if (curNodeType == T2_LOCALLABEL) {
        returnGenerated                     = false;
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
        AdjustStackOnReturn(0);
    } else if (curNodeType == T2_INPUT || (T2_SIGN <= curNodeType && curNodeType <= T2_CARRY)) {
        curExprLoc[0] = 0;
        curExprLoc[1] = 0;
        exprLoc[0] = 8;
        exprLoc[1] = 8;
        AnalyzeRegisterUsage();
        SaveOrRedirectRegister(0);
        registerDataType[0]        = 0;
        registerContents[0]        = tx2qp;
        registerIsDirect[0]       = 0;
        registerOffset[0]        = 0;
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
    codeSize += (fragmentCodeLength[curNodeType] & 0x1f);
}
