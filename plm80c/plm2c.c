/****************************************************************************
 *  plm2c.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

static byte bC259, bC25A; // lifted to file scope

static void Sub_6C54(byte arg1b) {
    if (arg1b != 0)
        tx2Auxw[arg1b]++;
}

static void Sub_6C73(byte arg1b) {
    if (OPTIMIZE)
        while (tx2qp - 1 > arg1b) {
            arg1b++;
            if (tx2opc[arg1b] == curOp && tx2op1[arg1b] == tx2op1[tx2qp] &&
                tx2op2[arg1b] == tx2op2[tx2qp] && tx2Aux1b[arg1b] == tx2Aux1b[tx2qp]) {
                if (tx2op3[arg1b] != 0xff00) {
                    tx2opc[tx2qp] = T2_OPTBACKREF;
                    tx2op1[tx2qp] = arg1b;
                    return;
                }
            }
        }
    if ((bC1D2 & 0xc0) == 0) {
        Sub_6C54((byte)tx2op1[tx2qp]);
        Sub_6C54((byte)tx2op2[tx2qp]);
    }
    tx2op3[tx2qp] = tx2qp;
}

static bool boC25D;

static void Sub_6EAB(wpointer arg1wP) {
    if (*arg1wP && tx2opc[*arg1wP] == T2_OPTBACKREF)
        *arg1wP = tx2op1[*arg1wP];
}

static void Sub_6F20(byte arg1b) {
    byte opc;
    if ((opc = tx2opc[arg1b]) == T2_BASED)
        boC25D = true;
    else if (opc == T2_BYTEINDEX || opc == T2_WORDINDEX) {
        if (tx2opc[tx2op2[arg1b]] != T2_NUMBER)
            boC25D = true;
        else {
            SetInfo(tx2op1[tx2op1[arg1b]]);
            if (tx2op2[tx2op2[arg1b]] >= info->dim || (info->flag & F_AT))
                boC25D = true;
        }
    } else if (opc == T2_IDENTIFIER) {
        SetInfo(tx2op1[arg1b]);
        if ((info->flag & F_AT))
            boC25D = true;
    }
} /* Sub_6F20() */

static void Sub_6EE1(byte arg1b) {
    boC25D = false;
    if (tx2opc[arg1b] == T2_MEMBER) {
        Sub_6F20((byte)tx2op1[arg1b]);
        Sub_6F20((byte)tx2op2[arg1b]);
    } else
        Sub_6F20(arg1b);
}

static byte bC263;

static void Sub_7018(byte arg1b) {
    if (tx2opc[arg1b] != T2_CALL)
        arg1b = (byte)tx2op3[arg1b];
    if (arg1b && arg1b > bC263)
        bC263 = arg1b;
}

static byte Sub_6FE2() {
    bC263 = boC25D ? bC259 : bC25A;

    Sub_7018((byte)tx2op1[tx2qp]);
    Sub_7018((byte)tx2op2[tx2qp]);
    return bC263;
}

static bool sub_70BC(byte arg1b) {
    Sub_6EE1((byte)tx2op1[arg1b]);
    if (boC25D)
        return true;
    if (tx2op1[arg1b] == 0xac) {
        SetInfo(tx2op1[tx2op1[arg1b]]);
        if ((info->flag & F_AUTOMATIC))
            return true;
    }
    return false;
}

void Sub_7055() {
    if (sub_70BC(tx2qp))
        bC25A = tx2qp;
    if (tx2op3[tx2op1[bC259 = tx2qp]] != 0xff00)
        tx2op3[tx2op1[tx2qp]] = tx2qp;
    Sub_6C54((byte)tx2op1[tx2qp]);
    Sub_6C54((byte)tx2op2[tx2qp]);
}

static void Sub_6D52() {
    Sub_6EAB(&tx2op1[tx2qp]);
    Sub_6EAB(&tx2op2[tx2qp]);
    if (curOp == T2_COLONEQUALS)
        Sub_7055();
    else if (procCallDepth > 0) {
        Sub_6C54((byte)tx2op1[tx2qp]);
        Sub_6C54((byte)tx2op2[tx2qp]);
        if (curOp == T2_CALL)
            procCallDepth--;
        else if (curOp == T2_MOVE || curOp == T2_CALLVAR) {
            procCallDepth--;
            Sub_6EAB(&tx2op3[tx2qp]);
            Sub_6C54((byte)tx2op3[tx2qp]);
        } else
            tx2op3[tx2qp] = 0xff00;
    } else if (curOp == T2_OUTPUT || curOp == T2_TIME) {
        Sub_6C54((byte)tx2op1[tx2qp]);
        Sub_6C54((byte)tx2op2[tx2qp]);
    } else {
        tx2op3[tx2qp] = 0;
        Sub_6EE1(tx2qp);
        Sub_6C73(Sub_6FE2());
        if (curOp == T2_JMPFALSE && tx2opc[tx2qp - 1] == T2_NOT) {
            boC20F        = true;
            tx2op2[tx2qp] = tx2op1[tx2qp - 1];
            Sub_56A0(tx2qp, tx2qp - 1);
            tx2opc[tx2qp] = T2_SEMICOLON;
        }
    }
}

static void Sub_7111() {
    if (procCallDepth > 0)
        tx2op3[tx2qp] = 0xff00;
    else {
        tx2op3[tx2qp] = 0;
        if (curOp == T2_IDENTIFIER) {
            SetInfo(tx2op1[tx2qp]);
            Sub_6C73((info->flag & F_AT) ? bC259 : bC25A);
        } else
            Sub_6C73(bC25A);
    }
}

void Sub_6BD6() {
    bC259 = 4;
    bC25A = 4;
    for (tx2qp = 4; tx2qp <= bC1BF - 1; tx2qp++) {
        tx2Auxw[tx2qp] = 0;
        curOp          = tx2opc[tx2qp];
        bC1D2          = b5124[curOp];
        if ((bC1D2 & 0xc0) == 0)
            Sub_6D52();
        else if ((bC1D2 & 0xc0) == 0x40)
            Sub_7111();
        if (curOp == T2_BEGCALL || curOp == T2_BEGMOVE)
            procCallDepth++;
    }
}