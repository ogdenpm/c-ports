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
        if (tx2Aux2b[operand] != 4)
            return false;
    } else if (arg1b != 3) {
        if (tx2Aux2b[operand] != 8)
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
    if (curOp == T2_MEMBER && tx2Aux2b[tx2op1[tx2qp]] == 0xa) {
        bC272 = tx2Aux1b[tx2op2[tx2qp]];
        bC273 = 0xA;
    } else
        switch (b5112[arithOp] & 7) {
        case 0:
            bC272 = 0;
            bC273 = 8;
            break;
        case 1:
            if (tx2Aux1b[tx2op1[tx2qp]] == 0 && tx2Aux1b[tx2op2[tx2qp]] == 0)
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
            bC272 = tx2Aux1b[tx2op1[tx2qp]];
            bC273 = tx2Aux2b[tx2op1[tx2qp]];
            break;
        case 4:
            bC272 = tx2Aux1b[tx2op2[tx2qp]];
            bC273 = tx2Aux2b[tx2op2[tx2qp]];
            break;
        case 5:
            bC272 = Sub_5748(tx2Aux1b[tx2op2[tx2qp]]);
            bC273 = 8;
            break;
        }
    if (lhsAccFlag == rhsAccFlag)
        infoIdx = 0;
    else if (rhsAccFlag == 0)
        infoIdx = tx2op1[tx2op1[tx2qp]];
    else
        infoIdx = tx2op1[tx2op2[tx2qp]];
    info = infoIdx ? &infotab[infoIdx] : NULL;
}

void Sub_717B() {
    word constVal;

    arithOp = curOp - 0x12;
    operand = (byte)tx2op1[tx2qp];
    GetVal(operand, &lhsAcc, &lhsAccFlag);
    if (Sub_7254(b5112[arithOp] >> 6, lhsAccFlag)) {
        operand = (byte)tx2op2[tx2qp];
        GetVal(operand, &rhsAcc, &rhsAccFlag);
        if (Sub_7254((b5112[arithOp] >> 3) & 7, rhsAccFlag)) {
            Sub_611A();
            constVal = evalConst();
            Sub_73C5();
            if (bC272 == 0 && bC273 == 8)
                constVal &= 0xFF;
            Sub_5F4B(constVal, infoIdx, bC272, bC273);
            bC1D2 = b5124[curOp = tx2opc[tx2qp]];
        }
    }
}

static byte bC27A;
static word acc, accFlag;
static byte bC27F;

static bool Sub_76E2(byte arg1b) {
    if ((bC27F & 0x40))
        return tx2Aux1b[arg1b] == 1 || tx2Aux1b[arg1b] == 3;
    return tx2Aux1b[arg1b] == 0 || tx2Aux1b[arg1b] == 2 || tx2Aux1b[arg1b] == 8;
}

static void Sub_7754(byte arg3b) {
    byte i;

    for (i = tx2qp + 1; tx2opc[i] == T2_OPTBACKREF && i < bC1BF; i++)
        ;

    if (tx2opc[i] == T2_COLONEQUALS && arg3b == tx2op1[i] && tx2op2[i] == tx2qp &&
        tx2Auxw[tx2qp] == 1) {
        bC27A          = 6;
        tx2opc[i]      = T2_SEMICOLON;
        tx2Auxw[tx2qp] = 0;
        tx2Auxw[arg3b]--;
    }
}

static bool Sub_7801(byte arg1b, byte arg2b, byte arg3b) {
    word p;
    if (accFlag != 0 || (curOp < T2_BASED && tx2Aux2b[arg2b] != 8))
        return false;
    if ((bC27F & 0x20) != 0)
        if (!Sub_76E2(arg3b))
            return false;
    if ((bC27F & 1) && Sub_76E2(arg2b) != Sub_76E2(arg3b))
        return false;
    if ((bC27F & 4)) {
        if ((bC27F & 8))
            return acc != 0;
        p = w502A[b5048[arg1b] >> 3];
        if (p != acc)
            return false;
        if ((bC27F & 0x80))
            Sub_7754(arg3b);
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

static void Sub_7925(byte arg1b, byte arg2b, byte arg3b) {
    byte j, k;

    arg1b--; // non nested ok here
    byte opc = b5221[arg1b] + bC27A;
    j        = b5048[arg1b] & 3;
    if (opc == 0xAD) {
        if (j == 0)
            acc = 0;
        Sub_611A();
        bC27F = 0;
        k     = 1;
        if (curOp != T2_STAR && curOp != T2_SLASH && curOp != T2_MOD && acc < 0x100 &&
            Sub_76E2(arg2b) && (arg3b == 0 || Sub_76E2(arg2b)))
            k = 0;
        Sub_5F4B(acc, 0, k, 8);
    } else {
        if (j == 1) {
            tx2op1[tx2qp] = arg3b;
            tx2Auxw[arg2b]--;
        } else {
            if (tx2op2[tx2qp] != 0)
                tx2Auxw[tx2op2[tx2qp]]--;
            if ((bC1D2 & 4)) {
                tx2op1[tx2qp] = tx2op2[1];
                if (boC20F) {
                    opc    = 0x43 - opc;
                    boC20F = false;
                }
            }
        }
        tx2opc[tx2qp] = opc;
        tx2op2[tx2qp] = 0;
    }
    bC1D2 = b5124[curOp = opc];
}

static bool Sub_765B(byte arg1b, byte operand, byte arg3b) {
    GetVal(operand, &acc, &accFlag);
    bC27A     = 0;
    bool more = true;

    while (more) {
        if ((bC27F = b50AD[arg1b]) == 0xff)
            return false;
        if ((bC27F & 0x10)) {
            more = !Sub_76E2(operand);
            if ((bC27F & 4) && !more)
                more = !Sub_7801(arg1b, operand, arg3b);
        } else
            more = !Sub_7801(arg1b, operand, arg3b);
        arg1b++;
    }
    Sub_7925(arg1b, operand, arg3b);
    return true;
} /* Sub() */

void Sub_7550() {
    if (curOp <= T2_GT) {
        if (tx2opc[tx2qp + 1] == T2_JMPFALSE && tx2Auxw[tx2qp] == 1) {
            tx2opc[tx2qp + 1] = T2_JNZ;
            tx2op1[tx2qp + 1] = tx2op2[1];
            if (boC20F) {
                tx2op2[tx2qp + 1] = bC209[curOp];
                boC20F            = false;
            } else
                tx2op2[tx2qp + 1] = curOp;
            tx2Auxw[tx2qp] = 0;
        } else
            tx2opc[tx2qp] = curOp += 0x26;
    } else if (!Sub_765B(b5202[curOp], (byte)tx2op2[tx2qp], (byte)tx2op1[tx2qp]))
        Sub_765B(b51E3[curOp], (byte)tx2op1[tx2qp], (byte)tx2op2[tx2qp]);
}