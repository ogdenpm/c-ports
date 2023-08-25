/****************************************************************************
 *  plm2d.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

// lifted to file scope
static word lhsAcc, lhsAccFlag, rhsAcc, rhsAccFlag;
static byte operand;
static byte arithOp;
static byte bC272, bC273;

static bool Sub_7254(byte arg1b, word arg2w) {
    if (arg1b == 0) {
        if (tx2[operand].aux2 != 4)
            return false;
    } else if (arg1b != 3) {
        if (tx2[operand].aux2 != 8)
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
    case 0:
        return lhsAcc;
    case 1:
        return lhsAcc + rhsAcc;
    case 2:
        return lhsAcc - rhsAcc;
    case 3:
        return lhsAcc * rhsAcc;
    case 4:
        return lhsAcc / rhsAcc;
    case 5:
        return lhsAcc % rhsAcc;
    case 6:
        return lhsAcc & rhsAcc;
    case 7:
        return lhsAcc | rhsAcc;
    case 8:
        return lhsAcc ^ rhsAcc;
    case 9:
        return lhsAcc + rhsAcc;
    case 10:
        return lhsAcc + rhsAcc;
    case 11:
        return lhsAcc + rhsAcc + rhsAcc;
    case 12:
        return lhsAcc + rhsAcc;
    case 13:
        return -lhsAcc;
    case 14:
        return ~lhsAcc;
    case 15:
        return Low(lhsAcc);
    case 16:
        return High(lhsAcc);
    case 17:
        return lhsAcc;
    }
    return lhsAcc; // added to prevent compiler warning C4715
}

static void Sub_73C5() {
    if (curOp == T2_MEMBER && tx2[tx2[tx2qp].op1].aux2 == 0xa) {
        bC272 = tx2[tx2[tx2qp].op2].aux1;
        bC273 = 0xA;
    } else
        switch (b5112[arithOp] & 7) {
        case 0:
            bC272 = 0;
            bC273 = 8;
            break;
        case 1:
            if (tx2[tx2[tx2qp].op1].aux1 == 0 && tx2[tx2[tx2qp].op2].aux1 == 0)
                bC272 = 0;
            else
                bC272 = 1;
            bC273 = 8;
            break;
        case 2:
            bC272 = 1;
            bC273 = 8;
            break;
        case 3:
            bC272 = tx2[tx2[tx2qp].op1].aux1;
            bC273 = tx2[tx2[tx2qp].op1].aux2;
            break;
        case 4:
            bC272 = tx2[tx2[tx2qp].op2].aux1;
            bC273 = tx2[tx2[tx2qp].op2].aux2;
            break;
        case 5:
            bC272 = Sub_5748(tx2[tx2[tx2qp].op2].aux1);
            bC273 = 8;
            break;
        }
    if (lhsAccFlag == rhsAccFlag)
        infoIdx = 0;
    else if (rhsAccFlag == 0)
        infoIdx = tx2[tx2[tx2qp].op1].op1;
    else
        infoIdx = tx2[tx2[tx2qp].op2].op1;
    info = infoIdx ? &infotab[infoIdx] : NULL;
}

void Sub_717B() {
    word constVal;

    arithOp = curOp - 0x12;
    operand = (byte)tx2[tx2qp].op1;
    GetVal(operand, &lhsAcc, &lhsAccFlag);
    if (Sub_7254(b5112[arithOp] >> 6, lhsAccFlag)) {
        operand = (byte)tx2[tx2qp].op2;
        GetVal(operand, &rhsAcc, &rhsAccFlag);
        if (Sub_7254((b5112[arithOp] >> 3) & 7, rhsAccFlag)) {
            Sub_611A();
            constVal = evalConst();
            Sub_73C5();
            if (bC272 == 0 && bC273 == 8)
                constVal &= 0xFF;
            Sub_5F4B(constVal, infoIdx, bC272, bC273);
            bC1D2 = b5124[curOp = tx2[tx2qp].opc];
        }
    }
}

static byte bC27A;
static word acc, accFlag;
static byte bC27F;

static bool Sub_76E2(byte opx) {
    if ((bC27F & 0x40))
        return tx2[opx].aux1 == 1 || tx2[opx].aux1 == 3;
    return tx2[opx].aux1 == 0 || tx2[opx].aux1 == 2 || tx2[opx].aux1 == 8;
}

static void Sub_7754(byte opx) {
    byte i;

    for (i = tx2qp + 1; tx2[i].opc == T2_OPTBACKREF && i < tx2qNxt; i++)
        ;

    if (tx2[i].opc == T2_COLONEQUALS && opx == tx2[i].op1 && tx2[i].op2 == tx2qp &&
        tx2[tx2qp].auxw == 1) {
        bC27A          = 6;
        tx2[i].opc      = T2_SEMICOLON;
        tx2[tx2qp].auxw = 0;
        tx2[opx].auxw--;
    }
}

static bool Sub_7801(byte arg1b, byte op1, byte op2) {
    word p;
    if (accFlag != 0 || (curOp < T2_BASED && tx2[op1].aux2 != 8))
        return false;
    if ((bC27F & 0x20) != 0)
        if (!Sub_76E2(op2))
            return false;
    if ((bC27F & 1) && Sub_76E2(op1) != Sub_76E2(op2))
        return false;
    if ((bC27F & 4)) {
        if ((bC27F & 8))
            return acc != 0;
        p = w502A[b5048[arg1b] >> 3];
        if (p != acc)
            return false;
        if ((bC27F & 0x80))
            Sub_7754(op2);
        return true;
    }
    if (!(bC27F & 0x40)) {
        p     = w502A[b5048[arg1b] >> 3];
        bC27A = acc % p;
        return true;
    } else if (acc <= 4) {
        bC27A = (byte)acc;
        return true;
    } else
        return false;
}

static void Sub_7925(byte arg1b, byte op1, byte op2) {
    byte j;

    arg1b--; // non nested ok here
    byte opc = b5221[arg1b] + bC27A;
    j        = b5048[arg1b] & 3;
    if (opc == 0xAD) {
        if (j == 0)
            acc = 0;
        Sub_611A();
        bC27F = 0;
        bool isWord     = true;
        if (curOp != T2_STAR && curOp != T2_SLASH && curOp != T2_MOD && acc < 0x100 &&
            Sub_76E2(op1) && (op2 == 0 || Sub_76E2(op1)))
            isWord = false;
        Sub_5F4B(acc, 0, isWord, 8);
    } else {
        if (j == 1) {
            tx2[tx2qp].op1 = op2;
            tx2[op1].auxw--;
        } else {
            if (tx2[tx2qp].op2 != 0)
                tx2[tx2[tx2qp].op2].auxw--;
            if ((bC1D2 & 4)) {
                tx2[tx2qp].op1 = tx2[1].op2;
                if (boC20F) {
                    opc    = 0x43 - opc;
                    boC20F = false;
                }
            }
        }
        tx2[tx2qp].opc = opc;
        tx2[tx2qp].op2 = 0;
    }
    bC1D2 = b5124[curOp = opc];
}

static bool Sub_765B(byte arg1b, byte op1, byte op2) {
    GetVal(op1, &acc, &accFlag);
    bC27A     = 0;
    bool more = true;

    while (more) {
        if ((bC27F = b50AD[arg1b]) == 0xff)
            return false;
        if ((bC27F & 0x10)) {
            more = !Sub_76E2(op1);
            if ((bC27F & 4) && !more)
                more = !Sub_7801(arg1b, op1, op2);
        } else
            more = !Sub_7801(arg1b, op1, op2);
        arg1b++;
    }
    Sub_7925(arg1b, op1, op2);
    return true;
} /* Sub() */

void Sub_7550() {
    if (curOp <= T2_GT) {
        if (tx2[tx2qp + 1].opc == T2_JMPFALSE && tx2[tx2qp].auxw == 1) {
            tx2[tx2qp + 1].opc = T2_JNZ;
            tx2[tx2qp + 1].op1 = tx2[1].op2;
            if (boC20F) {
                tx2[tx2qp + 1].op2 = bC209[curOp];
                boC20F            = false;
            } else
                tx2[tx2qp + 1].op2 = curOp;
            tx2[tx2qp].auxw = 0;
        } else
            tx2[tx2qp].opc = curOp += 0x26;
    } else if (!Sub_765B(b5202[curOp], (byte)tx2[tx2qp].op2, (byte)tx2[tx2qp].op1))
        Sub_765B(b51E3[curOp], (byte)tx2[tx2qp].op1, (byte)tx2[tx2qp].op2);
}
