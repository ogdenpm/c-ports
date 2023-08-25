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
                if (tx2[--*pw].opc == T2_LINEINFO && tx2[*pw].op2 == 0 && tx2[*pw].op3 != 0)
                    p -= (tx2[*pw].op3 - tx2[*pw].op1);
            }
        }
    }
} /* sub69EB */

static void Sub_68E8() {
    byte i;

    bC252 = bC1D2 & 3;
    if ((bC1D2 & 4) != 0) {
        tx2[1].op2     = tx2[tx2qp].op1;
        tx2[tx2qp].op1 = 1;
    } else
        Sub_69EB(1, &tx2[tx2qp].op1);

    Sub_69EB(2, &tx2[tx2qp].op2);
    if (bC252 == 3) {
        if (curOp == T2_CALL)
            tx2[tx2qp].op3 = tx2[tx2qp].op3;
        else if (curOp == T2_BYTEINDEX || curOp == T2_WORDINDEX) {
            i = (byte)tx2[tx2qp].op1;
            tx2[i].op2 += tx2[tx2qp].op3 * Sub_575E(tx2[i].op1);
        } else
            Sub_69EB(3, &tx2[tx2qp].op3);
    }
    tx2[tx2qp].aux1 = 0xc;
    tx2[tx2qp].aux2 = 9;
}


static byte aux1bMap[] = {
    LIT_A, LABEL_A, BYTE_A, ADDRESS_A, STRUCT_A, BYTE_A, BYTE_A
}; // index by info_t

static void Sub_6AA4() {
    if (curOp == T2_IDENTIFIER) {
        SetInfo(tx2[tx2qp].op1);
        if ((info->flag & F_MEMBER)) {
            tx2[tx2qp].aux2 = 4;
        } else if ((info->flag & F_AUTOMATIC))
            tx2[tx2qp].aux2 = 0xa;
        else
            tx2[tx2qp].aux2 = 4;
        tx2[tx2qp].op2   = info->linkVal;
        tx2[tx2qp].aux1 = aux1bMap[info->type];
    } else if (curOp <= T2_BIGNUMBER) {
        tx2[tx2qp].op2   = tx2[tx2qp].op1;
        tx2[tx2qp].aux2 = 8;
        tx2[tx2qp].op1   = 0;
        if (curOp == T2_BIGNUMBER) {
            tx2[tx2qp].aux1 = true; // flag as word op
            tx2[tx2qp].opc   = T2_NUMBER;
        } else
            tx2[tx2qp].aux1 = false; // flag as byte op
    } else {
        tx2[tx2qp].aux1 = false;
        tx2[tx2qp].op2   = 0;
    }
}

void Sub_689E() {
    for (tx2qp = 4; tx2qp <= tx2qNxt - 1; tx2qp++) {
        curOp = tx2[tx2qp].opc;
        bC1D2 = b5124[curOp];
        if ((bC1D2 & 0xc0) == 0)
            Sub_68E8();
        else if ((bC1D2 & 0xc0) == 0x40)
            Sub_6AA4();
    }
}
