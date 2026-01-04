/****************************************************************************
 *  plm2f.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

static uint8_t operandNodeType;
static uint16_t calculatedOffset, first, last;

static bool CanUseRegisterDirect(uint8_t  lrIdx) {
    for (currentFragmentCode = first; currentFragmentCode <= last; currentFragmentCode++) {
        uint8_t i = GetFragmentOperandType(lrIdx);
        if (i <= 3 || (12 <= i && i <= 14))
            return true;
    }
    return false;
} /* Sub_8861() */

static void TryUseExistingRegister(uint8_t exprIdx, uint8_t lrIdx) {
    if (CanUseRegisterDirect(lrIdx)) {
        for (int regIdx = 0; regIdx < 4; regIdx++) {
            if (registerContents[regIdx] == exprIdx) {
                if (registerDataType[regIdx] == 0 || registerDataType[regIdx] == 1 || registerDataType[regIdx] == 6) {
                    exprAttr[lrIdx] = registerDataType[regIdx];
                    exprLoc[lrIdx] = regIdx;
                    if (exprLoc[1 - lrIdx] != regIdx)
                        return;
                }
            }
        }
    }
} /* Sub_88C1() */

static void TryUseIndirectRegister(uint8_t exprIdx, uint8_t lrIdx) {
    if (exprLoc[lrIdx] > 3) {
        for (int regIdx = 1; regIdx < 4; regIdx++) {
            if (registerContents[regIdx] == exprIdx && (registerDataType[regIdx] == 2 || registerDataType[regIdx] == 3)) {
                exprAttr[lrIdx] = registerDataType[regIdx];
                exprLoc[lrIdx] = regIdx;
                if (exprLoc[1 - lrIdx] != regIdx)
                    return;
            }
        }
    }
} /* TryUseIndirectRegister() */

static void AnalyzeStackOperand(uint8_t exprIdx, uint8_t lrIdx) {
    uint8_t activeRegCount;
    uint16_t stackLevel;

    if (exprLoc[lrIdx] == 0xA)
        calculatedOffset = tx2[exprIdx].right;
    else if (exprLoc[lrIdx] == 9) {
        calculatedOffset = tx2[exprIdx].extra;
        if ((!registerInExpression[Left] && registerNeedsSave[Left]) || savedRegisterCount > 0 || calculatedOffset != currentStackDepth) {
            activeRegCount = savedRegisterCount + exprRegisterCount;
            for (stackLevel = calculatedOffset; stackLevel <= currentStackDepth; stackLevel++) {
                if (stackNodeContents[stackLevel] != 0)
                    activeRegCount++;
            }
            if (activeRegCount < 4)
                boC1D8 = true;
            else
                exprLoc[lrIdx] = 0xA;
        }
        calculatedOffset = -(calculatedOffset * 2);
    }
} /* AnalyzeStackOperand() */

static void FindOptimalRegister(uint8_t lrIdx) {
    uint16_t operandOffset, storageFlags;
    uint8_t minDistance, targetAttr;
    uint16_t offsetDelta;

    if (exprLoc[lrIdx] == 0xA) {
        operandOffset  = calculatedOffset;
        storageFlags  = 0x100;
        minDistance = 4;
        targetAttr  = IndirectAddr(exprAttr[lrIdx]);
    } else if (exprLoc[lrIdx] == 8 && exprAttr[lrIdx] == 1) {
        GetOperandValue(curExprLoc[lrIdx], &operandOffset, &storageFlags);
        minDistance = 2;
        targetAttr  = ADDRESS_A;
    } else if (exprLoc[lrIdx] == 4 && (exprAttr[lrIdx] == 0 || exprAttr[lrIdx] == 8 || !CanUseRegisterDirect(lrIdx))) {
        GetOperandValue(curExprLoc[lrIdx], &operandOffset, &storageFlags);
        minDistance = 2;
        targetAttr  = IndirectAddr(exprAttr[lrIdx]);
    } else
        return;

    for (int regIdx = 1; regIdx < 4; regIdx++) {
        if (registerInExpression[regIdx]) {
            if (curExprLoc[Left] == curExprLoc[Right] && curNodeType != T2_COLONEQUALS)
                if (exprLoc[lrIdx] > 3)
                    exprLoc[lrIdx] = regIdx;
        } else if (!registerNeedsSave[regIdx] && registerStorageClass[regIdx] == storageFlags && registerIsDirect[regIdx] && 1 <= registerDataType[regIdx] && registerDataType[regIdx] <= 6) {
            offsetDelta = registerStackOffset[regIdx] + registerOffset[regIdx] - operandOffset;
            if (offsetDelta > 0xff)
                offsetDelta = -offsetDelta;
            if (offsetDelta < minDistance) {
                exprLoc[lrIdx] = regIdx;
                minDistance           = (uint8_t)offsetDelta;
            }
        }
    }
    if (exprLoc[lrIdx] <= 3) {
        int regIdx    = exprLoc[lrIdx];
        registerDataType[regIdx] = exprAttr[lrIdx] = targetAttr;
        registerContents[regIdx]                = curExprLoc[lrIdx];
        registerOffset[regIdx]                = registerStackOffset[regIdx] + registerOffset[regIdx] - operandOffset;
        registerStackOffset[regIdx]                = operandOffset;
    }
} /* FindOptimalRegister() */

static void AnalyzeOperandLocations() {
    uint8_t exprIdx, lrIdx;
    exprLoc[Left] = 8;
    exprLoc[Right] = 8;
    for (lrIdx = 0; lrIdx <= 1; lrIdx++) {
        if ((exprIdx = curExprLoc[lrIdx]) == 0)
            exprAttr[lrIdx] = 0xC;
        else if ((operandNodeType = tx2[exprIdx].nodeType) == T2_STACKPTR)
            exprAttr[lrIdx] = 0xA;
        else if (operandNodeType == T2_LOCALLABEL)
            exprAttr[lrIdx] = 9;
        else {
            exprAttr[lrIdx] = tx2[exprIdx].exprAttr;
            exprLoc[lrIdx] = tx2[exprIdx].exprLoc;
            TryUseExistingRegister(exprIdx, lrIdx);
            TryUseIndirectRegister(exprIdx, lrIdx); /*  checked */
        }
    }
    for (lrIdx = 0; lrIdx <= 1; lrIdx++) {
        exprIdx = curExprLoc[lrIdx];
        AnalyzeRegisterUsage();
        AnalyzeStackOperand(exprIdx, lrIdx);
        FindOptimalRegister(lrIdx);
        UpdateExpressionLookup(lrIdx);
    }
} /* AnalyzeOperandLocations() */

static uint8_t GetTypeConversion(uint8_t lrIdx) {

    if (curExprLoc[lrIdx] == 0 || curExprLoc[lrIdx] == 1)
        return 1;
    return typeConversionTable[iCodeArgsIndex[lrIdx]][GetFragmentOperandType(lrIdx)];
} /* GetTypeConversion() */

static void SetOperandCosts(uint8_t lrIdx, uint8_t srcIdx) {
    operandComplexity[lrIdx] = exprComplexity[srcIdx];
    operandRegisterCost[lrIdx] = exprRegisterCost[srcIdx];
    operandCategory[lrIdx] = nodeExprCategory[srcIdx];
} /* SetOperandCosts() */

static void SelectOptimalFragment() {
    uint8_t bestLeftType, bestRightType, minCost, leftConvType, rightConvType, totalCost;
    bestLeftType = bestRightType = 0; // set to avoid compiler warning

    minCost     = 198;
    for (currentFragmentCode = first; currentFragmentCode <= last; currentFragmentCode++) {
        leftConvType = GetTypeConversion(Left);
        rightConvType = GetTypeConversion(Right);
        totalCost = exprComplexity[leftConvType] + exprComplexity[rightConvType] + (fragmentCodeLength[opcodeEncodingMap[currentFragmentCode]] & 0x1f);
        if (totalCost < minCost) {
            minCost        = totalCost;
            bestLeftType        = leftConvType;
            bestRightType        = rightConvType;
            cfrag1   = opcodeEncodingMap[currentFragmentCode];
            selectedOperatorIdx    = fragmentOpcodeIndex[currentFragmentCode];
            operandFragmentType[Left] = GetFragmentOperandType(Left);
            operandFragmentType[Right] = GetFragmentOperandType(Right);
        }
    }
    SetOperandCosts(Left, bestLeftType);
    SetOperandCosts(Right, bestRightType);
} /* SelectOptimalFragment() */

static void AdjustOperandOffsets() {
    if (curExprLoc[Left] != 0)
        AdjustRegisterOffset(exprLoc[Left]);

    if (curExprLoc[Right] != 0)
        AdjustRegisterOffset(exprLoc[Right]);
} /* AdjustOperandOffsets() */

static void HandleSpecialStatements() {
    uint16_t p;

    if (curNodeType == T2_STKARG || curNodeType == T2_STKBARG || curNodeType == T2_STKWARG) {
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        callStackBase[procCallDepth]++;
        currentStackDepth++;
    } else if (curNodeType == T2_CALL) {
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        info = FromIdx(tx2[tx2qp].extra);
        if ((info->flag & F_EXTERNAL))
            p = (callStackBase[procCallDepth] + 1) * 2;
        else
            p = (callStackDepth[procCallDepth] + 1) * 2 + info->stackUsage;
        if (p > stackUsage)
            stackUsage = p;
    } else if (curNodeType == T2_CALLVAR) {
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        if (stackUsage < currentStackDepth * 2)
            stackUsage = currentStackDepth * 2;
    } else if (curNodeType == T2_RETURN || curNodeType == T2_RETURNBYTE ||
               curNodeType == T2_RETURNWORD) {
        nextReturnState = true;
        GenerateReturnSequence();
    } else if (curNodeType == T2_JMPFALSE) {
        AdjustStackOnReturn(0);
        if (boC20F) {
            cfrag1 = CF_JMPTRUE;
            boC20F = false;
        }
    } else if (curNodeType == T2_CASEBLOCK)
        AdjustStackOnReturn(0);
    else if (curNodeType == T2_MOVE) {
        if (callStackBase[procCallDepth] != currentStackDepth) {
            AdjustStackOnReturn(-((callStackBase[procCallDepth] + 1) * 2));
            PopRegisterFromStack(3);
        }
        if (registerDataType[3] == 1)
            cfrag1 = CF_MOVE_HL;
    }
} /* HandleSpecialStatements() */

static void InvalidateTargetRegisters() {
    for (int regIdx = 0; regIdx < 4; regIdx++) {
        if (registerContents[regIdx] == curExprLoc[Left])
            if (registerDataType[regIdx] < 2 || 5 < registerDataType[regIdx])
                registerContents[regIdx] = 0;
    }
}

static void UpdateRegisterState() {

    uint16_t precedenceBits = operatorPrecedence[selectedOperatorIdx] * 16;
    uint16_t typeRuleBits = operatorTypeRules[selectedOperatorIdx];
    uint8_t resultRegIdx = 0;
    if (curNodeType == T2_COLONEQUALS) {
        InvalidateTargetRegisters();
        if (tx2[curExprLoc[Right]].cnt == 0)
            if (tx2[curExprLoc[Left]].cnt > 0) {
                if (cfrag1 == CF_MOVMLR || cfrag1 == CF_STA) {
                    registerDataType[exprLoc[Right]] = BYTE_A;
                    registerContents[exprLoc[Right]] = curExprLoc[Left];
                } else if (cfrag1 == CF_SHLD || cfrag1 == CF_MOVMRP) {
                    registerDataType[exprLoc[Right]] = ADDRESS_A;
                    registerContents[exprLoc[Right]] = curExprLoc[Left];
                }
            }
    } else if (T2_51 <= curNodeType && curNodeType <= T2_56)
        InvalidateTargetRegisters();
    for (int InvalidateTargetRegisters = 5; InvalidateTargetRegisters < 9; InvalidateTargetRegisters++) {
        uint8_t precedenceRule = precedenceBits >> 13;
        uint8_t ruleValue      = typeRuleBits >> 12;
        precedenceBits <<= 3;
        typeRuleBits <<= 4;
        if (ruleValue <= 3) {
            CopyRegisterState(ruleValue, InvalidateTargetRegisters);
            if (precedenceRule == 1)
                registerOffset[InvalidateTargetRegisters]++;
            else if (precedenceRule == 2) {
                if (registerDataType[InvalidateTargetRegisters] == BYTE_A) {
                    registerDataType[InvalidateTargetRegisters] = 6;
                } else {
                    registerDataType[InvalidateTargetRegisters]  = ADDRESS_A;
                    registerIsDirect[InvalidateTargetRegisters] = 0;
                }
            }
        } else if (ruleValue == 4) {
            registerIsDirect[resultRegIdx = InvalidateTargetRegisters] = 0;
            if (0 < tx2[tx2qp].cnt) {
                registerContents[InvalidateTargetRegisters] = tx2qp;
                registerDataType[InvalidateTargetRegisters] = tx2[tx2qp].exprAttr = fragmentCodeLength[cfrag1] >> 5;
                registerOffset[InvalidateTargetRegisters]                       = 0;
            } else
                registerContents[InvalidateTargetRegisters] = 0;
        } else if (ruleValue == 5) {
            registerContents[InvalidateTargetRegisters]  = 0;
            registerStorageClass[InvalidateTargetRegisters]  = 0;
            registerOffset[InvalidateTargetRegisters]  = 0;
            registerIsDirect[InvalidateTargetRegisters] = 0xFF;
            registerDataType[InvalidateTargetRegisters]  = BYTE_A;
            registerStackOffset[InvalidateTargetRegisters]  = precedenceRule;
        } else {
            registerContents[InvalidateTargetRegisters]  = 0;
            registerIsDirect[InvalidateTargetRegisters] = 0;
        }
    }
    if (resultRegIdx == 0 && tx2[tx2qp].cnt > 0) {
        for (int tmpRegIdx = 5; tmpRegIdx < 9; tmpRegIdx++) {
            if (registerContents[tmpRegIdx] == 0)
                if (!registerIsDirect[resultRegIdx = tmpRegIdx])
                    break;
        }
        if (resultRegIdx != 0) {
            registerContents[resultRegIdx]            = tx2qp;
            registerIsDirect[resultRegIdx]           = 0;
            registerDataType[resultRegIdx]            = BYTE_A;
            tx2[tx2qp].exprAttr = BYTE_A;
            registerOffset[resultRegIdx]            = 0;
        }
    }
    for (int n = 0; n <= 3; n++)
        CopyRegisterState(n + 5, n);
} /* UpdateRegisterState() */

void GenerateExpressionCode() {
    curExprLoc[Left] = (uint8_t)tx2[tx2qp].left;
    curExprLoc[Right] = (uint8_t)tx2[tx2qp].right;
    first    = nodeTypeToAttribute[curNodeType];
    last     = first + operatorResultType[curNodeType] - 1;
    AnalyzeOperandLocations();

    while (1) {
        SelectOptimalFragment(); /*  OK */
        if (operandComplexity[Left] == 0 && operandComplexity[Right] == 0)
            break;
        if (boC1D8)
            AllocateRegister();
        else
            GenerateOperandCode();
    }
    AdjustOperandOffsets();
    DecrementExprRefs();
    InvalidateRegistersByMask(operatorPrecedence[selectedOperatorIdx] >> 12);
    HandleSpecialStatements();
    GenerateFragmentCode();
    UpdateRegisterState();
} /* GenerateExpressionCode() */

void c_procedure() {
    if (EnterBlk()) {
        blk[blkId].codeSize  = codeSize;
        blk[blkId].wB4B0     = currentStackDepth;
        blk[blkId].stackSize = stackUsage;
        blk[blkId].extProcId = curExtProcId;
        blk[activeGrpCnt].next  = blkId;
        blkId                = activeGrpCnt;
        info = blk[activeGrpCnt].info = FromIdx(tx2[tx2qp].left);
        curExtProcId               = info->procId;
        codeSize                   = 0;
        EmitTopItem();
        GenerateProcedureEntry();
    }
}
