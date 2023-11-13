/****************************************************************************
 *  plm2h.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

static bool Sub_9C33() {
    byte i, j, k;

    i = (byte)tx2[tx2qp].left;
    if ((nodeControlMap[tx2[i].nodeType] & 0xc0) == 0) {
        if (tx2[i].cnt > 1)
            return false;
        if (tx2[i].extra != 0)
            bC140[tx2[i].extra] = tx2qp;
    }
    k = (byte)tx2[tx2qp].cnt;
    Sub_56A0(i, tx2qp);
    tx2[tx2qp].cnt = k;
    tx2[i].cnt--;
    for (j = 0; j <= 3; j++) {
        if (bC04E[j] == i)
            bC04E[j] = tx2qp;
    }
    return true;
}

void Sub_9BB0() {
    bC0B7[0] = (byte)tx2[tx2qp].left;
    bC0B7[1] = (byte)tx2[tx2qp].right;
    if (T2_DOUBLE <= curNodeType && curNodeType <= T2_ADDRESSOF)
        Sub_717B();
    if (curNodeType <= T2_MEMBER) {
        Sub_7550();
        if (curNodeType == T2_65)
            if (Sub_9C33())
                return;
    }
    if ((nodeControlFlags & 0xc0) == 0) {
        Sub_87CB();
        if (curNodeType == T2_MOVE)
            procCallDepth = 0;
    } else if ((nodeControlFlags & 0xc0) == 0x80)
        Sub_994D();
}


static byte b9BA8[2] = { 12, 13 };
static byte b9BAA[2] = { 1, 2 };

void Sub_9D06() {
    byte j, k;
    pointer pbyt;
    byte m;

    if (procCallDepth <= 10) {
        info = FromIdx(tx2[tx2qp].extra);
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
        *pb_C2EB |= tx2[arg1b].exprAttr == BYTE_A ? b9BAE[arg2b] : b9BAC[arg2b];
}

void Sub_9DD7() {
    byte i;

    if (procCallDepth <= 10) {
        i = (byte)tx2[tx2qp].extra;
        if (tx2[i].nodeType == T2_IDENTIFIER) {
            info = FromIdx(tx2[i].left);
            if ((info->flag & F_AUTOMATIC))
                wAF54[T2_CALLVAR] = 3;
            else
                wAF54[T2_CALLVAR] = 4;
        } else if (tx2[i].extra == wB53C[procCallDepth]) {
            wAF54[T2_CALLVAR] = 5;
            wB528[procCallDepth]--;
        } else
            wAF54[T2_CALLVAR] = 2;

        pb_C2EB = &b44F7[wAF54[T2_CALLVAR]];
        Sub_9EAA((byte)tx2[tx2qp].left, 0);
        Sub_9EAA((byte)tx2[tx2qp].right, 1);
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
        blk[firstCase].firstCase = topCase;
}

void Sub_9F2F() {
    index_t curCase, q;
    curCase = q = blk[firstCase].firstCase;
    if (ExitBlk()) {
        while (curCase < topCase) {
            wC1DC[0] = 14;
            wC1DC[1] = casetab[curCase];
            EncodeFragData(CF_DW);
            codeSize += 2;
            curCase++;
        }
        if (topCase == q) {
            Tx2SyntaxError(ERR201); /*  Invalid() do CASE block, */
                                    /*  at least on case required */
            EmitTopItem();
        }
        topCase = q;
    }
}

void Sub_9F9F() {
    if (ExitBlk()) {
        info = blk[blkId].info;
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
        PutTx1Word(ToIdx(blk[blkId].info));
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
    info = FromIdx(tx2[tx2qp].left);
    p = info->dim - arg1b;
    Sub_5F4B(p, NULL, p < 0x100 ? BYTE_A : ADDRESS_A, LOC_REG);
}

void Sub_A0C4() {
    word p;
    p = GetElementSize(FromIdx(tx2[tx2qp].left));
    Sub_5F4B(p, NULL, p < 0x100 ? BYTE_A : ADDRESS_A, LOC_REG);
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
        curNodeType = tx2[tx2qp].nodeType;
        nodeControlFlags = nodeControlMap[curNodeType];
        switch (nodeControlFlags >> 6) {
        case 0:
            if (curNodeType == T2_CALL)
                Sub_9D06();
            else if (curNodeType == T2_CALLVAR)
                Sub_9DD7();
            else if (curNodeType == T2_BEGMOVE)
                Sub_9EF8();
            else
                Sub_9BB0();
            break;
        case 1:
            if (curNodeType == T2_LENGTH)
                Sub_A072(0);
            else if (curNodeType == T2_LAST)
                Sub_A072(1);
            else if (curNodeType == T2_SIZE)
                Sub_A0C4();
            break;
        case 2:
            if (curNodeType == T2_PROCEDURE)
                Sub_9457();
            else
                Sub_994D();
            break;
        case 3:
            if (curNodeType == T2_CASE)
                Sub_9F14();
            else if (curNodeType == T2_ENDCASE)
                Sub_9F2F();
            else if (curNodeType == T2_ENDPROC)
                Sub_9F9F();
            else if (curNodeType == T2_BEGCALL)
                Sub_A10A();
            break;
        }

        tx2[tx2qp].extra = 0;
    }
    Sub_5795(0);
    boC1CC = boC1CD;
}
