/****************************************************************************
 *  plm2h.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

static bool Sub_9C33() {
    byte i, j, k;

    i = (byte)tx2[tx2qp].op1;
    if ((b5124[tx2[i].opc] & 0xc0) == 0) {
        if (tx2[i].auxw > 1)
            return false;
        if (tx2[i].op3 != 0)
            bC140[tx2[i].op3] = tx2qp;
    }
    k = (byte)tx2[tx2qp].auxw;
    Sub_56A0(i, tx2qp);
    tx2[tx2qp].auxw = k;
    tx2[i].auxw--;
    for (j = 0; j <= 3; j++) {
        if (bC04E[j] == i)
            bC04E[j] = tx2qp;
    }
    return true;
}

void Sub_9BB0() {
    bC0B7[0] = (byte)tx2[tx2qp].op1;
    bC0B7[1] = (byte)tx2[tx2qp].op2;
    if (T2_DOUBLE <= curOp && curOp <= T2_ADDRESSOF)
        Sub_717B();
    if (curOp <= T2_MEMBER) {
        Sub_7550();
        if (curOp == T2_65)
            if (Sub_9C33())
                return;
    }
    if ((bC1D2 & 0xc0) == 0) {
        Sub_87CB();
        if (curOp == T2_MOVE)
            procCallDepth = 0;
    } else if ((bC1D2 & 0xc0) == 0x80)
        Sub_994D();
}


static byte b9BA8[2] = { 12, 13 };
static byte b9BAA[2] = { 1, 2 };

void Sub_9D06() {
    byte j, k;
    pointer pbyt;
    byte m;

    if (procCallDepth <= 10) {
        SetInfo(tx2[tx2qp].op3);
        wAF54[T2_CALL] = info->returnType == ADDRESS_T ? ADDRESS_A : BYTE_A;
        j = m = info->paramCnt;
        pbyt  = &b44F7[wAF54[T2_CALL]];
        k     = 0;

        while (j > 0) {
            AdvNxtInfo();
            if (--j < 2) {
                *pbyt = ((*pbyt << 4) | (info->type == ADDRESS_T ? b9BA8[k] : b9BAA[k]));
                k = 1;
            }
        }

        if (m == 1)
            *pbyt <<= 4;
        Sub_9BB0();
        wC1C3 = wB528[procCallDepth];
    }
    procCallDepth--;
}

static pointer pb_C2EB;

static byte b9BAC[2] = { 12, 13 };
static byte b9BAE[2] = { 1, 2 };

static void Sub_9EAA(byte arg1b, byte arg2b) {
    *pb_C2EB <<= 4;
    if (arg1b != 0)
        *pb_C2EB |= tx2[arg1b].aux1 == BYTE_A ? b9BAE[arg2b] : b9BAC[arg2b];
}

void Sub_9DD7() {
    byte i;

    if (procCallDepth <= 10) {
        i = (byte)tx2[tx2qp].op3;
        if (tx2[i].opc == T2_IDENTIFIER) {
            SetInfo(tx2[i].op1);
            if ((info->flag & F_AUTOMATIC))
                wAF54[T2_CALLVAR] = 3;
            else
                wAF54[T2_CALLVAR] = 4;
        } else if (tx2[i].op3 == wB53C[procCallDepth]) {
            wAF54[T2_CALLVAR] = 5;
            wB528[procCallDepth]--;
        } else
            wAF54[T2_CALLVAR] = 2;

        pb_C2EB = &b44F7[wAF54[T2_CALLVAR]];
        Sub_9EAA((byte)tx2[tx2qp].op1, 0);
        Sub_9EAA((byte)tx2[tx2qp].op2, 1);
        Sub_9BB0();
        wC1C3 = wB528[procCallDepth];
    }
    procCallDepth--;
}

void Sub_9EF8() {
    procCallDepth = 1;
    Sub_9BB0();
    wB53C[procCallDepth] = wC1C3;
}

void Sub_9F14() {
    if (EnterBlk())
        blk[blkSP].info = caseCnt;
}

void Sub_9F2F() {
    word p, q;
    p = q = (word)blk[blkSP].info;
    if (ExitBlk()) {
        while (p < caseCnt) {
            wC1DC[0] = 14;
            wC1DC[1] = casetab[p];
            EncodeFragData(CF_DW);
            codeSize += 2;
            p++;
        }
        if (caseCnt == q) {
            Tx2SyntaxError(ERR201); /*  Invalid() do CASE block, */
                                    /*  at least on case required */
            EmitTopItem();
        }
        caseCnt = q;
    }
}

void Sub_9F9F() {
    if (ExitBlk()) {
        SetInfo(blk[blkId].info);
        if (!boC1CC) {
            Sub_5EE8();
            EncodeFragData(CF_RET);
            codeSize++;
        }
        if ((info->flag & F_INTERRUPT))
            stackUsage += 8;

        info->dim     = codeSize;
        info->baseVal = stackUsage + wC1C7;
        codeSize            = blk[blkId = blk[blkId].next].codeSize;
        fragLen       = 0;
        PutTx1Byte(0xa4);
        PutTx1Word(blk[blkId].info);
        PutTx1Word(codeSize);
        WrFragData();
        wC1C3        = blk[blkId].wB4B0;
        stackUsage        = blk[blkId].stackSize;
        wC1C7        = 0;
        curExtProcId = blk[blkId].extProcId;
    }
}

void Sub_A072(byte arg1b) {
    word p;
    SetInfo(tx2[tx2qp].op1);
    p = info->dim - arg1b;
    Sub_5F4B(p, 0, p < 0x100 ? BYTE_A : ADDRESS_A, 8);
}

void Sub_A0C4() {
    word p;
    p = Sub_575E(tx2[tx2qp].op1);
    Sub_5F4B(p, 0, p < 0x100 ? BYTE_A : ADDRESS_A, 8);
}

void Sub_A10A() {
    procCallDepth++;
    if (procCallDepth <= 10) {
        Sub_5E66(0xf);
        wB528[procCallDepth] = wC1C3;
        wB53C[procCallDepth] = wC1C3;
    } else if (procCallDepth == 11) {
        Tx2SyntaxError(ERR203); /*  LIMIT EXCEEDED: NESTING OF TYPED */
                                /*  procedure CALLS */
        EmitTopItem();
    }
}

static void Sub_A266() {
    byte i;

    boC1CD = false;
    for (i = 0; i <= 3; i++) {
        bC045[i]  = 0xc;
        bC04E[i]  = 0;
        boC057[i] = false;
    }
}

void Sub_A153() {
    Sub_A266();
    for (tx2qp = 4; tx2qp <= tx2qNxt - 1; tx2qp++) {
        curOp = tx2[tx2qp].opc;
        bC1D2 = b5124[curOp];
        switch (bC1D2 >> 6) {
        case 0:
            if (curOp == T2_CALL)
                Sub_9D06();
            else if (curOp == T2_CALLVAR)
                Sub_9DD7();
            else if (curOp == T2_BEGMOVE)
                Sub_9EF8();
            else
                Sub_9BB0();
            break;
        case 1:
            if (curOp == T2_LENGTH)
                Sub_A072(0);
            else if (curOp == T2_LAST)
                Sub_A072(1);
            else if (curOp == T2_SIZE)
                Sub_A0C4();
            break;
        case 2:
            if (curOp == T2_PROCEDURE)
                Sub_9457();
            else
                Sub_994D();
            break;
        case 3:
            if (curOp == T2_CASE)
                Sub_9F14();
            else if (curOp == T2_ENDCASE)
                Sub_9F2F();
            else if (curOp == T2_ENDPROC)
                Sub_9F9F();
            else if (curOp == T2_BEGCALL)
                Sub_A10A();
            break;
        }

        tx2[tx2qp].op3 = 0;
    }
    Sub_5795(0);
    boC1CC = boC1CD;
}
