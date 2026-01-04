/****************************************************************************
 *  plm2h.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"


// Can inline if:
// 1. Left operand is binary/unary operator (nodeControlMap & 0xC0 == 0)
// 2. Reference count <= 1 (not shared)
// 3. No stack reference (extra == 0) OR update stack tracking

// Actions:
// - Move expression from exprNodeIdx to tx2qp
// - Decrement original node's reference count
// - Update all register contents pointing to exprNodeIdx
static bool TryInlineExpression() {
    uint8_t exprNodeIdx, regIdx, savedRefCount;

    exprNodeIdx = (uint8_t)tx2[tx2qp].left;
    if ((nodeControlMap[tx2[exprNodeIdx].nodeType] & 0xc0) == 0) {
        if (tx2[exprNodeIdx].cnt > 1)
            return false;
        if (tx2[exprNodeIdx].extra != 0)
            stackNodeContents[tx2[exprNodeIdx].extra] = tx2qp;
    }
    savedRefCount = (uint8_t)tx2[tx2qp].cnt;
    MoveTx2(exprNodeIdx, tx2qp);
    tx2[tx2qp].cnt = savedRefCount;
    tx2[exprNodeIdx].cnt--;
    for (regIdx = 0; regIdx <= 3; regIdx++) {
        if (registerContents[regIdx] == exprNodeIdx)
            registerContents[regIdx] = tx2qp;
    }
    return true;
}

void GenerateOperatorCode() {
    curExprLoc[Left]  = (uint8_t)tx2[tx2qp].left;
    curExprLoc[Right] = (uint8_t)tx2[tx2qp].right;
    if (T2_DOUBLE <= curNodeType && curNodeType <= T2_ADDRESSOF) // type conversion
        FoldConstantExpr();
    if (curNodeType <= T2_MEMBER) {
        OptimiseExpression();
        if (curNodeType == T2_65)
            if (TryInlineExpression()) // attempt inline optimisation
                return;
    }
    if ((nodeControlFlags & 0xc0) == 0) { // binary/unary operator
        GenerateExpressionCode();
        if (curNodeType == T2_MOVE)
            procCallDepth = 0;
    } else if ((nodeControlFlags & 0xc0) == 0x80)   // statement level node
        ProcessSpecialNodes(); // handle statement-level codegeneration
}

static uint8_t addressTypeEncoding[2] = { 12, 13 };
static uint8_t byteTypeEncoding[2] = { 1, 2 };

void GenerateCallCode() {
    uint8_t remainingParams, paramIdx;
    pointer encodingPtr;
    uint8_t totalParams;

    if (procCallDepth <= 10) {
        info           = FromIdx(tx2[tx2qp].extra);
        nodeTypeToAttribute[T2_CALL] = info->returnType == ADDRESS_T ? ADDRESS_A : BYTE_A;
        remainingParams = totalParams = info->paramCnt;
        encodingPtr  = &fragmentOpcodeTable[nodeTypeToAttribute[T2_CALL]];
        paramIdx     = 0;

        while (remainingParams > 0) {
            AdvNxtInfo();
            if (--remainingParams < 2) {
                *encodingPtr = ((*encodingPtr << 4) | (info->type == ADDRESS_T ? addressTypeEncoding[paramIdx] : byteTypeEncoding[paramIdx]));
                paramIdx     = 1;
            }
        }

        if (totalParams == 1)
            *encodingPtr <<= 4;
        GenerateOperatorCode();
        currentStackDepth = callStackDepth[procCallDepth];
    }
    procCallDepth--;
}

static uint8_t GetParameterTypeEncoding(uint8_t exprIdx, uint8_t lrIdx) {
    return exprIdx == 0 ? 0 : tx2[exprIdx].exprAttr == BYTE_A ? byteTypeEncoding[lrIdx] : addressTypeEncoding[lrIdx];
}

void GenerateIndirectCallCode() {
    uint8_t callVarNodeIDx;

    if (procCallDepth <= 10) {
        uint8_t callVariableType;
        callVarNodeIDx = (uint8_t)tx2[tx2qp].extra;
        if (tx2[callVarNodeIDx].nodeType == T2_IDENTIFIER) {
            info  = FromIdx(tx2[callVarNodeIDx].left);
            callVariableType = info->flag & F_AUTOMATIC ? 3 : 4;    // automatic or static/memory variable
        } else if (tx2[callVarNodeIDx].extra == callStackBase[procCallDepth]) {
            callVariableType = 5;   // stack/memory variable
            callStackDepth[procCallDepth]--;
        } else
            callVariableType = 2;   // generic call variable

        nodeTypeToAttribute[T2_CALLVAR] = callVariableType;
        fragmentOpcodeTable[callVariableType] =
            (GetParameterTypeEncoding((uint8_t)tx2[tx2qp].left, Left) << 4) | GetParameterTypeEncoding((uint8_t)tx2[tx2qp].right, Right);
        GenerateOperatorCode();
        currentStackDepth = callStackDepth[procCallDepth];
    }
    procCallDepth--;
}

void BeginMoveOperation() {
    procCallDepth = 1;
    GenerateOperatorCode();
    callStackBase[procCallDepth] = currentStackDepth;
}

void EnterCaseBlock() {
    if (EnterBlk())
        blk[activeGrpCnt].activeGrpCnt = topCase;
}

void ExitCaseBlock() {
    index_t curCase, savedTopCase;
    curCase = savedTopCase = blk[activeGrpCnt].activeGrpCnt;
    if (ExitBlk()) {
        while (curCase < topCase) {
            iCodeArgs[0] = IR_CASELAB;
            iCodeArgs[1] = casetab[curCase];
            EncodeFragData(CF_DW);
            codeSize += 2;
            curCase++;
        }
        if (topCase == savedTopCase) {
            Tx2SyntaxError(ERR201); /*  Invalid() do CASE block, */
                                    /*  at least on case required */
            EmitTopItem();
        }
        topCase = savedTopCase;
    }
}

void ExitProcedure() {
    if (ExitBlk()) {
        info = blk[blkId].info;
        if (!returnGenerated) {
            GenerateReturnSequence();
            EncodeFragData(CF_RET);
            codeSize++;
        }
        if ((info->flag & F_INTERRUPT))
            stackUsage += 8;

        info->dim     = codeSize;
        info->baseVal = stackUsage + localVariableSize;
        codeSize      = blk[blkId = blk[blkId].next].codeSize;
        fragLen       = 0;
        PutTx1Byte(0xa4);
        PutTx1Word(ToIdx(blk[blkId].info));
        PutTx1Word(codeSize);
        WrFragData();
        currentStackDepth        = blk[blkId].wB4B0;
        stackUsage   = blk[blkId].stackSize;
        localVariableSize        = 0;
        curExtProcId = blk[blkId].extProcId;
    }
}

void GenerateLengthBuiltin(uint8_t adjust) {
    uint16_t resultValue;
    info = FromIdx(tx2[tx2qp].left);
    resultValue    = info->dim - adjust;
    CreateConstantOrIdNode(resultValue, NULL, resultValue < 0x100 ? BYTE_A : ADDRESS_A, LOC_REG);
}

void GenerateSIzeBuiltin() {
    uint16_t resultValue;
    resultValue = GetElementSize(FromIdx(tx2[tx2qp].left));
    CreateConstantOrIdNode(resultValue, NULL, resultValue < 0x100 ? BYTE_A : ADDRESS_A, LOC_REG);
}

void BeginProcedureCall() {
    procCallDepth++;
    if (procCallDepth <= 10) {
        InvalidateRegistersByMask(0xf);
        callStackDepth[procCallDepth] = currentStackDepth;
        callStackBase[procCallDepth] = currentStackDepth;
    } else if (procCallDepth == 11) {
        Tx2SyntaxError(ERR203); /*  LIMIT EXCEEDED: NESTING OF TYPED */
                                /*  procedure CALLS */
        EmitTopItem();
    }
}

static void ResetCodeGenState() {
    uint8_t regIdx;

    nextReturnState = false;
    for (regIdx = 0; regIdx <= 3; regIdx++) {
        registerDataType[regIdx]  = 0xc;    // likely expression attribute/types
        registerContents[regIdx]  = 0;      // likely expression locations
        registerIsDirect[regIdx] = false;  // likely expression flags/conditions
    }
}

void GenerateStatementCode() {
    ResetCodeGenState();
    for (tx2qp = 4; tx2qp <= tx2qNxt - 1; tx2qp++) {
        curNodeType      = tx2[tx2qp].nodeType;
        nodeControlFlags = nodeControlMap[curNodeType];
        switch (nodeControlFlags >> 6) {    // node category
        case 0: // Binary/Unary operators & special operations
            if (curNodeType == T2_CALL)
                GenerateCallCode();
            else if (curNodeType == T2_CALLVAR)
                GenerateIndirectCallCode();
            else if (curNodeType == T2_BEGMOVE)
                BeginMoveOperation();
            else
                GenerateOperatorCode();
            break;
        case 1: // builtin functions  (LENGTH, LAST, SIZE)
            if (curNodeType == T2_LENGTH)
                GenerateLengthBuiltin(0);
            else if (curNodeType == T2_LAST)
                GenerateLengthBuiltin(1);
            else if (curNodeType == T2_SIZE)
                GenerateSIzeBuiltin();
            break;
        case 2: // procedure/statement nodes
            if (curNodeType == T2_PROCEDURE)
                c_procedure();
            else
                ProcessSpecialNodes();
            break;
        case 3: // control flow (CASE, ENDCASE, ENDPROC, BEGCALL)
            if (curNodeType == T2_CASE)
                EnterCaseBlock();
            else if (curNodeType == T2_ENDCASE)
                ExitCaseBlock();
            else if (curNodeType == T2_ENDPROC)
                ExitProcedure();
            else if (curNodeType == T2_BEGCALL)
                BeginProcedureCall();
            break;
        }

        tx2[tx2qp].extra = 0;
    }
    AdjustStackOnReturn(0);    // final code generation cleanup
    returnGenerated = nextReturnState;    // update code generation state flag
}
