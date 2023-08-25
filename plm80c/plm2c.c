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
        tx2[arg1b].auxw++;
}

static void Sub_6C73(byte arg1b) {
    if (OPTIMIZE)
        while (tx2qp - 1 > arg1b) {
            arg1b++;
            if (tx2[arg1b].opc == curOp && tx2[arg1b].op1 == tx2[tx2qp].op1 &&
                tx2[arg1b].op2 == tx2[tx2qp].op2 && tx2[arg1b].aux1 == tx2[tx2qp].aux1) {
                if (tx2[arg1b].op3 != 0xff00) {
                    tx2[tx2qp].opc = T2_OPTBACKREF;
                    tx2[tx2qp].op1 = arg1b;
                    return;
                }
            }
        }
    if ((bC1D2 & 0xc0) == 0) {
        Sub_6C54((byte)tx2[tx2qp].op1);
        Sub_6C54((byte)tx2[tx2qp].op2);
    }
    tx2[tx2qp].op3 = tx2qp;
}

static bool boC25D;

static void Sub_6EAB(wpointer arg1wP) {
    if (*arg1wP && tx2[*arg1wP].opc == T2_OPTBACKREF)
        *arg1wP = tx2[*arg1wP].op1;
}

static void Sub_6F20(byte arg1b) {
    byte opc;
    if ((opc = tx2[arg1b].opc) == T2_BASED)
        boC25D = true;
    else if (opc == T2_BYTEINDEX || opc == T2_WORDINDEX) {
        if (tx2[tx2[arg1b].op2].opc != T2_NUMBER)
            boC25D = true;
        else {
            SetInfo(tx2[tx2[arg1b].op1].op1);
            if (tx2[tx2[arg1b].op2].op2 >= info->dim || (info->flag & F_AT))
                boC25D = true;
        }
    } else if (opc == T2_IDENTIFIER) {
        SetInfo(tx2[arg1b].op1);
        if ((info->flag & F_AT))
            boC25D = true;
    }
} /* Sub_6F20() */

static void Sub_6EE1(byte arg1b) {
    boC25D = false;
    if (tx2[arg1b].opc == T2_MEMBER) {
        Sub_6F20((byte)tx2[arg1b].op1);
        Sub_6F20((byte)tx2[arg1b].op2);
    } else
        Sub_6F20(arg1b);
}

static byte bC263;

static void Sub_7018(byte arg1b) {
    if (tx2[arg1b].opc != T2_CALL)
        arg1b = (byte)tx2[arg1b].op3;
    if (arg1b && arg1b > bC263)
        bC263 = arg1b;
}

static byte Sub_6FE2() {
    bC263 = boC25D ? bC259 : bC25A;

    Sub_7018((byte)tx2[tx2qp].op1);
    Sub_7018((byte)tx2[tx2qp].op2);
    return bC263;
}

static bool sub_70BC(byte arg1b) {
    Sub_6EE1((byte)tx2[arg1b].op1);
    if (boC25D)
        return true;
    if (tx2[arg1b].op1 == 0xac) {
        SetInfo(tx2[tx2[arg1b].op1].op1);
        if ((info->flag & F_AUTOMATIC))
            return true;
    }
    return false;
}

void Sub_7055() {
    if (sub_70BC(tx2qp))
        bC25A = tx2qp;
    if (tx2[tx2[bC259 = tx2qp].op1].op3 != 0xff00)
        tx2[tx2[tx2qp].op1].op3 = tx2qp;
    Sub_6C54((byte)tx2[tx2qp].op1);
    Sub_6C54((byte)tx2[tx2qp].op2);
}

static void Sub_6D52() {
    Sub_6EAB(&tx2[tx2qp].op1);
    Sub_6EAB(&tx2[tx2qp].op2);
    if (curOp == T2_COLONEQUALS)
        Sub_7055();
    else if (procCallDepth > 0) {
        Sub_6C54((byte)tx2[tx2qp].op1);
        Sub_6C54((byte)tx2[tx2qp].op2);
        if (curOp == T2_CALL)
            procCallDepth--;
        else if (curOp == T2_MOVE || curOp == T2_CALLVAR) {
            procCallDepth--;
            Sub_6EAB(&tx2[tx2qp].op3);
            Sub_6C54((byte)tx2[tx2qp].op3);
        } else
            tx2[tx2qp].op3 = 0xff00;
    } else if (curOp == T2_OUTPUT || curOp == T2_TIME) {
        Sub_6C54((byte)tx2[tx2qp].op1);
        Sub_6C54((byte)tx2[tx2qp].op2);
    } else {
        tx2[tx2qp].op3 = 0;
        Sub_6EE1(tx2qp);
        Sub_6C73(Sub_6FE2());
        if (curOp == T2_JMPFALSE && tx2[tx2qp - 1].opc == T2_NOT) {
            boC20F        = true;
            tx2[tx2qp].op2 = tx2[tx2qp - 1].op1;
            Sub_56A0(tx2qp, tx2qp - 1);
            tx2[tx2qp].opc = T2_SEMICOLON;
        }
    }
}

static void Sub_7111() {
    if (procCallDepth > 0)
        tx2[tx2qp].op3 = 0xff00;
    else {
        tx2[tx2qp].op3 = 0;
        if (curOp == T2_IDENTIFIER) {
            SetInfo(tx2[tx2qp].op1);
            Sub_6C73((info->flag & F_AT) ? bC259 : bC25A);
        } else
            Sub_6C73(bC25A);
    }
}

void Sub_6BD6() {
    bC259 = 4;
    bC25A = 4;
    for (tx2qp = 4; tx2qp <= tx2qNxt - 1; tx2qp++) {
        tx2[tx2qp].auxw = 0;
        curOp          = tx2[tx2qp].opc;
        bC1D2          = b5124[curOp];
        if ((bC1D2 & 0xc0) == 0)
            Sub_6D52();
        else if ((bC1D2 & 0xc0) == 0x40)
            Sub_7111();
        if (curOp == T2_BEGCALL || curOp == T2_BEGMOVE)
            procCallDepth++;
    }
}
