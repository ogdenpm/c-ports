/****************************************************************************
 *  plm2d.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

// lifted to file scope
static word lhsAcc, lhsAccFlag, rhsAcc, rhsAccFlag;
static byte operand;
static byte arithOp;
static byte resAttrib, resOpFlag;

static bool Sub_7254(byte arg1b, word arg2w) {
    if (arg1b == 0) {
        if (tx2[operand].exprLoc != LOC_MEM)
            return false;
    } else if (arg1b != 3) {
        if (tx2[operand].exprLoc != LOC_REG)
            return false;
        if (arg2w != 0) {
            if (arg1b == 2)
                return false;
            if (arg1b == 4) {
                if (lhsAccFlag != 0)
                    return false;
            } else if (arg1b == 5) {
                if (lhsAccFlag != rhsAccFlag)
                    return false;
            }
        }
    } else if (arg2w == 0x4000)
        return false;
    return true;
}

static word evalConst() {
    switch (arithOp) {
    case 0: // DOUBLE
        return lhsAcc;
    case 1: // PLUSSIGN
        return lhsAcc + rhsAcc;
    case 2: // MINUSSIGN
        return lhsAcc - rhsAcc;
    case 3: // STAR
        return lhsAcc * rhsAcc;
    case 4: // SLASH
        return lhsAcc / rhsAcc;
    case 5: // MOD
        return lhsAcc % rhsAcc;
    case 6: // AND
        return lhsAcc & rhsAcc;
    case 7: // OR
        return lhsAcc | rhsAcc;
    case 8: // XOR
        return lhsAcc ^ rhsAcc;
    case 9: // BASED
        return lhsAcc + rhsAcc;
    case 10: // BYTEINDEX
        return lhsAcc + rhsAcc;
    case 11: // WORDINDEX
        return lhsAcc + rhsAcc + rhsAcc;
    case 12: // MEMBER
        return lhsAcc + rhsAcc;
    case 13: // UNARY MINUS
        return -lhsAcc;
    case 14: // NOT
        return ~lhsAcc;
    case 15: // LOW
        return Low(lhsAcc);
    case 16: // HIGH
        return High(lhsAcc);
    case 17: // ADDRESSOF
        return lhsAcc;
    }
    return lhsAcc; // added to prevent compiler warning C4715
}

static void Sub_73C5() {
    if (curNodeType == T2_MEMBER && tx2[tx2[tx2qp].left].exprLoc == LOC_STACK) {
        resAttrib = tx2[tx2[tx2qp].right].exprAttr;
        resOpFlag = LOC_STACK;
    } else
        switch (b5112[arithOp] & 7) {
        case 0: // LOW HIGH
            resAttrib = BYTE_A;
            resOpFlag = LOC_REG;
            break;
        case 1: // PLUSSIGN MINUSSIGN AND OR XOR
            if (tx2[tx2[tx2qp].left].exprAttr == BYTE_A && tx2[tx2[tx2qp].right].exprAttr == BYTE_A)
                resAttrib = BYTE_A;
            else
                resAttrib = ADDRESS_A;
            resOpFlag = LOC_REG;
            break;
        case 2: // DOUBLE STAR SLASH MOD ADDRESSOF
            resAttrib = ADDRESS_A;
            resOpFlag = LOC_REG;
            break;
        case 3:                                        // BYTEINDEX WORDINDEX UNARYMINUS NOT
            resAttrib = tx2[tx2[tx2qp].left].exprAttr; // propagate attribute of item op applied to
            resOpFlag = tx2[tx2[tx2qp].left].exprLoc;  // and the opFlag
            break;
        case 4:                                         // MEMBER
            resAttrib = tx2[tx2[tx2qp].right].exprAttr; // get the member's attribute
            resOpFlag = tx2[tx2[tx2qp].right].exprLoc;  // and opFlag
            break;
        case 5: // BASED
            resAttrib = IndirectAddr(tx2[tx2[tx2qp].right].exprAttr);
            resOpFlag = LOC_REG;
            break;
        }
    if (lhsAccFlag == rhsAccFlag)
        info = NULL;
    else if (rhsAccFlag == 0)
        info = FromIdx(tx2[tx2[tx2qp].left].left);
    else
        info = FromIdx(tx2[tx2[tx2qp].right].left);
}

void Sub_717B() {
    word constVal;

    arithOp = curNodeType - T2_DOUBLE; // remove offset
    operand = (byte)tx2[tx2qp].left;
    GetVal(operand, &lhsAcc, &lhsAccFlag);
    if (Sub_7254(b5112[arithOp] >> 6, lhsAccFlag)) {
        operand = (byte)tx2[tx2qp].right;
        GetVal(operand, &rhsAcc, &rhsAccFlag);
        if (Sub_7254((b5112[arithOp] >> 3) & 7, rhsAccFlag)) {
            Sub_611A();
            constVal = evalConst();
            Sub_73C5();
            if (resAttrib == BYTE_A && resOpFlag == LOC_REG)
                constVal &= 0xFF;
            Sub_5F4B(constVal, info, resAttrib, resOpFlag);
            nodeControlFlags = nodeControlMap[curNodeType = tx2[tx2qp].nodeType];
        }
    }
}

static byte bC27A;
static word acc, accFlag;
static byte curStepActions;

static bool ExpectedAttr(byte nodeLoc) {
    if ((curStepActions & 0x40))
        return tx2[nodeLoc].exprAttr == ADDRESS_A || tx2[nodeLoc].exprAttr == ADDRESSIND_A;
    return tx2[nodeLoc].exprAttr == BYTE_A || tx2[nodeLoc].exprAttr == BYTEIND_A ||
           tx2[nodeLoc].exprAttr == STRUCT_A;
}

static void Sub_7754(byte nodeIdx) {
    byte i;

    for (i = tx2qp + 1; tx2[i].nodeType == T2_OPTBACKREF && i < tx2qNxt; i++)
        ;

    if (tx2[i].nodeType == T2_COLONEQUALS && nodeIdx == tx2[i].left && tx2[i].right == tx2qp &&
        tx2[tx2qp].cnt == 1) {
        bC27A           = 6;
        tx2[i].nodeType = T2_SEMICOLON;
        tx2[tx2qp].cnt = 0;
        tx2[nodeIdx].cnt--;
    }
}

static bool Sub_7801(byte stepIdx, byte left, byte right) {
    if (accFlag != 0 || (curNodeType < T2_BASED && tx2[left].exprLoc != LOC_REG))
        return false;
    if ((curStepActions & 0x20) && !ExpectedAttr(right))
        return false;
    if ((curStepActions & 1) && ExpectedAttr(left) != ExpectedAttr(right))
        return false;
    if ((curStepActions & 4)) {
        if ((curStepActions & 8))
            return acc != 0;
        if (w502A[b5048[stepIdx] >> 3] != acc)
            return false;
        if ((curStepActions & 0x80))
            Sub_7754(right);
        return true;
    }
    if (!(curStepActions & 0x40)) {
        bC27A = acc % w502A[b5048[stepIdx] >> 3];
        return true;
    } else if (acc <= 4) {
        bC27A = (byte)acc;
        return true;
    } else
        return false;
}

static void Sub_7925(byte stepIdx, byte left, byte right) {
    byte j;

    stepIdx--; // non nested ok here
    byte nodeType = b5221[stepIdx] + bC27A;
    j             = b5048[stepIdx] & 3;
    if (nodeType == T2_NUMBER) {
        if (j == 0)
            acc = 0;
        Sub_611A();
        curStepActions = 0;     // forces ExpectedAttr to test for BYTE types
        byte exprAttr  = ADDRESS_A;
        // PMO - only makes sense if last test is for right not left below
        if (curNodeType != T2_STAR && curNodeType != T2_SLASH && curNodeType != T2_MOD &&
            acc < 0x100 && ExpectedAttr(left) && (!right || ExpectedAttr(/*left*/ right)))
            exprAttr = BYTE_A;
        Sub_5F4B(acc, NULL, exprAttr, LOC_REG);
    } else {
        if (j == 1) {
            tx2[tx2qp].left = right;
            tx2[left].cnt--;
        } else {
            if (tx2[tx2qp].right != 0)
                tx2[tx2[tx2qp].right].cnt--;
            if ((nodeControlFlags & 4)) {
                tx2[tx2qp].left = tx2[1].right;
                if (boC20F) {
                    nodeType = 0x43 - nodeType;
                    boC20F   = false;
                }
            }
        }
        tx2[tx2qp].nodeType = nodeType;
        tx2[tx2qp].right    = 0;
    }
    nodeControlFlags = nodeControlMap[curNodeType = nodeType];
}

static bool Sub_765B(byte stepIdx, byte left, byte right) {
    GetVal(left, &acc, &accFlag);
    bC27A = 0;

    for (bool more = true; more; stepIdx++) {
        if ((curStepActions = stepTable[stepIdx]) == 0xff)
            return false;
        if ((curStepActions & 0x10)) {
            more = !ExpectedAttr(left);
            if ((curStepActions & 4) && !more)
                more = !Sub_7801(stepIdx, left, right);
        } else
            more = !Sub_7801(stepIdx, left, right);
    }
    Sub_7925(stepIdx, left, right);
    return true;
} /* Sub() */

void Sub_7550() {
    if (curNodeType <= T2_GT) { // relational operation
        if (tx2[tx2qp + 1].nodeType == T2_JMPFALSE && tx2[tx2qp].cnt == 1) {
            tx2[tx2qp + 1].nodeType = T2_JNZ;   // single relational then direct jmp
            tx2[tx2qp + 1].left     = tx2[1].right;
            if (boC20F) {
                tx2[tx2qp + 1].right = bC209[curNodeType];
                boC20F               = false;
            } else
                tx2[tx2qp + 1].right = curNodeType;
            tx2[tx2qp].cnt = 0;
        } else
            tx2[tx2qp].nodeType = curNodeType += T2_LT_VAL; // convert to conditional value
    } else if (!Sub_765B(step1Map[curNodeType], (byte)tx2[tx2qp].right, (byte)tx2[tx2qp].left))
        Sub_765B(step2Map[curNodeType], (byte)tx2[tx2qp].left, (byte)tx2[tx2qp].right);
}
