/****************************************************************************
 *  plm2b.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

static byte bC252; // lifted to file scope

static void Sub_69EB(byte arg1b, word *pw) {
    word p;

    if (*pw) {
        if (arg1b > bC252)
            *pw = 0;
        else {
            p   = *pw;
            *pw = tx2qp;

            while (p--) {
                --*pw;
                if (tx2opc[*pw] == T2_LINEINFO && tx2op2[*pw] == 0 && tx2op3[*pw] != 0)
                    p -= (tx2op3[*pw] - tx2op1[*pw]);
            }
        }
    }
} /* sub69EB */

static void Sub_68E8() {
    byte i;

    bC252 = bC1D2 & 3;
    if ((bC1D2 & 4) != 0) {
        tx2op2[1]     = tx2op1[tx2qp];
        tx2op1[tx2qp] = 1;
    } else
        Sub_69EB(1, &tx2op1[tx2qp]);

    Sub_69EB(2, &tx2op2[tx2qp]);
    if (bC252 == 3) {
        if (curOp == T2_CALL)
            tx2op3[tx2qp] = tx2op3[tx2qp];
        else if (curOp == T2_BYTEINDEX || curOp == T2_WORDINDEX) {
            i         = (byte)tx2op1[tx2qp];
            tx2op2[i] += tx2op3[tx2qp] * Sub_575E(tx2op1[i]);
        } else
            Sub_69EB(3, &tx2op3[tx2qp]);
    }
    tx2Aux1b[tx2qp] = 0xc;
    tx2Aux2b[tx2qp] = 9;
}

static void Sub_6AA4() {
    if (curOp == T2_IDENTIFIER) {
        SetInfo(tx2op1[tx2qp]);
        if ((info->flag & F_MEMBER)) {
            SetInfo(info->parent);
            tx2Aux2b[tx2qp] = 4;
        } else if ((info->flag & F_AUTOMATIC))
            tx2Aux2b[tx2qp] = 0xa;
        else
            tx2Aux2b[tx2qp] = 4;
        SetInfo(tx2op1[tx2qp]);
        tx2op2[tx2qp]   = info->linkVal;
        tx2Aux1b[tx2qp] = b5286[info->type];
    } else if (curOp <= T2_BIGNUMBER) {
        tx2op2[tx2qp]   = tx2op1[tx2qp];
        tx2Aux2b[tx2qp] = 8;
        tx2op1[tx2qp]   = 0;
        if (curOp == T2_BIGNUMBER) {
            tx2Aux1b[tx2qp] = 1;
            tx2opc[tx2qp]   = T2_NUMBER;
        } else
            tx2Aux1b[tx2qp] = 0;
    } else {
        tx2Aux1b[tx2qp] = 0;
        tx2op2[tx2qp]   = 0;
    }
}

void Sub_689E() {
    for (tx2qp = 4; tx2qp <= bC1BF - 1; tx2qp++) {
        curOp = tx2opc[tx2qp];
        bC1D2 = b5124[curOp];
        if ((bC1D2 & 0xc0) == 0)
            Sub_68E8();
        else if ((bC1D2 & 0xc0) == 0x40)
            Sub_6AA4();
    }
}