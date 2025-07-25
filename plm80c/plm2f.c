/****************************************************************************
 *  plm2f.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

static byte bC2A7;
static word wC2A9, first, last;

static bool Sub_8861(byte  lrIdx) {
    for (wC1D6 = first; wC1D6 <= last; wC1D6++) {
        byte i = Sub_5679(lrIdx);
        if (i <= 3 || (12 <= i && i <= 14))
            return true;
    }
    return false;
} /* Sub_8861() */

static void Sub_88C1(byte exprIdx, byte lrIdx) {
    if (Sub_8861(lrIdx)) {
        for (int i = 0; i < 4; i++) {
            if (bC04E[i] == exprIdx) {
                if (bC045[i] == 0 || bC045[i] == 1 || bC045[i] == 6) {
                    exprAttr[lrIdx] = bC045[i];
                    exprLoc[lrIdx] = i;
                    if (exprLoc[1 - lrIdx] != i)
                        return;
                }
            }
        }
    }
} /* Sub_88C1() */

static void Sub_894A(byte exprIdx, byte lrIdx) {
    if (exprLoc[lrIdx] > 3) {
        for (int i = 1; i < 4; i++) {
            if (bC04E[i] == exprIdx && (bC045[i] == 2 || bC045[i] == 3)) {
                exprAttr[lrIdx] = bC045[i];
                exprLoc[lrIdx] = i;
                if (exprLoc[1 - lrIdx] != i)
                    return;
            }
        }
    }
} /* Sub_894A() */

static void Sub_89D1(byte exprIdx, byte lrIdx) {
    byte i;
    word p;

    if (exprLoc[lrIdx] == 0xA)
        wC2A9 = tx2[exprIdx].right;
    else if (exprLoc[lrIdx] == 9) {
        wC2A9 = tx2[exprIdx].extra;
        if ((!boC069[Left] && boC072[Left]) || bC0B1 > 0 || wC2A9 != wC1C3) {
            i = bC0B1 + bC0B2;
            for (p = wC2A9; p <= wC1C3; p++) {
                if (bC140[p] != 0)
                    i++;
            }
            if (i < 4)
                boC1D8 = true;
            else
                exprLoc[lrIdx] = 0xA;
        }
        wC2A9 = -(wC2A9 * 2);
    }
} /* Sub_89D1() */

static void Sub_8A9C(byte lrIdx) {
    word p, q;
    byte ii, j;
    word r;

    if (exprLoc[lrIdx] == 0xA) {
        p  = wC2A9;
        q  = 0x100;
        ii = 4;
        j  = IndirectAddr(exprAttr[lrIdx]);
    } else if (exprLoc[lrIdx] == 8 && exprAttr[lrIdx] == 1) {
        GetVal(curExprLoc[lrIdx], &p, &q);
        ii = 2;
        j  = ADDRESS_A;
    } else if (exprLoc[lrIdx] == 4 && (exprAttr[lrIdx] == 0 || exprAttr[lrIdx] == 8 || !Sub_8861(lrIdx))) {
        GetVal(curExprLoc[lrIdx], &p, &q);
        ii = 2;
        j  = IndirectAddr(exprAttr[lrIdx]);
    } else
        return;

    for (int i = 1; i < 4; i++) {
        if (boC069[i]) {
            if (curExprLoc[Left] == curExprLoc[Right] && curNodeType != T2_COLONEQUALS)
                if (exprLoc[lrIdx] > 3)
                    exprLoc[lrIdx] = i;
        } else if (!boC072[i] && wC096[i] == q && boC057[i] && 1 <= bC045[i] && bC045[i] <= 6) {
            r = wC084[i] + bC0A8[i] - p;
            if (r > 0xff)
                r = -r;
            if (r < ii) {
                exprLoc[lrIdx] = i;
                ii           = (byte)r;
            }
        }
    }
    if (exprLoc[lrIdx] <= 3) {
        int i    = exprLoc[lrIdx];
        bC045[i] = exprAttr[lrIdx] = j;
        bC04E[i]                = curExprLoc[lrIdx];
        bC0A8[i]                = wC084[i] + bC0A8[i] - p;
        wC084[i]                = p;
    }
} /* Sub_8A9C() */

static void Sub_8CF5() {
    byte exprIdx, lrIdx;
    exprLoc[Left] = 8;
    exprLoc[Right] = 8;
    for (lrIdx = 0; lrIdx <= 1; lrIdx++) {
        if ((exprIdx = curExprLoc[lrIdx]) == 0)
            exprAttr[lrIdx] = 0xC;
        else if ((bC2A7 = tx2[exprIdx].nodeType) == T2_STACKPTR)
            exprAttr[lrIdx] = 0xA;
        else if (bC2A7 == T2_LOCALLABEL)
            exprAttr[lrIdx] = 9;
        else {
            exprAttr[lrIdx] = tx2[exprIdx].exprAttr;
            exprLoc[lrIdx] = tx2[exprIdx].exprLoc;
            Sub_88C1(exprIdx, lrIdx);
            Sub_894A(exprIdx, lrIdx); /*  checked */
        }
    }
    for (lrIdx = 0; lrIdx <= 1; lrIdx++) {
        exprIdx = curExprLoc[lrIdx];
        Sub_597E();
        Sub_89D1(exprIdx, lrIdx);
        Sub_8A9C(lrIdx);
        Sub_61A9(lrIdx);
    }
} /* Sub_8CF5() */

static byte Sub_8E7E(byte lrIdx) {

    if (curExprLoc[lrIdx] == 0 || curExprLoc[lrIdx] == 1)
        return 1;
    return b4D23[bC0C1[lrIdx]][Sub_5679(lrIdx)];
} /* Sub_8E7E() */

static void Sub_8ECD(byte lrIdx, byte srcIdx) {
    bC0B9[lrIdx] = b4C45[srcIdx];
    bC0BB[lrIdx] = b4CB4[srcIdx];
    bC0BD[lrIdx] = b4FA3[srcIdx];
} /* Sub_8ECD() */

static void Sub_8DCD() {
    byte h, i, j, k, m, n;
    h = i = 0; // set to avoid compiler warning

    j     = 198;
    for (wC1D6 = first; wC1D6 <= last; wC1D6++) {
        k = Sub_8E7E(Left);
        m = Sub_8E7E(Right);
        n = b4C45[k] + b4C45[m] + (codeAttrLen[b4A21[wC1D6]] & 0x1f);
        if (n < j) {
            j        = n;
            h        = k;
            i        = m;
            cfrag1   = b4A21[wC1D6];
            bC1D9    = b46EB[wC1D6];
            bC0BF[Left] = Sub_5679(Left);
            bC0BF[Right] = Sub_5679(Right);
        }
    }
    Sub_8ECD(Left, h);
    Sub_8ECD(Right, i);
} /* Sub_8DCD() */

static void Sub_8F16() {
    if (curExprLoc[Left] != 0)
        Sub_63AC(exprLoc[Left]);

    if (curExprLoc[Right] != 0)
        Sub_63AC(exprLoc[Right]);
} /* Sub_8F16() */

static void Sub_8F35() {
    word p;

    if (curNodeType == T2_STKARG || curNodeType == T2_STKBARG || curNodeType == T2_STKWARG) {
        Sub_5795(-(wB53C[procCallDepth] * 2));
        wB53C[procCallDepth]++;
        wC1C3++;
    } else if (curNodeType == T2_CALL) {
        Sub_5795(-(wB53C[procCallDepth] * 2));
        info = FromIdx(tx2[tx2qp].extra);
        if ((info->flag & F_EXTERNAL))
            p = (wB53C[procCallDepth] + 1) * 2;
        else
            p = (wB528[procCallDepth] + 1) * 2 + info->stackUsage;
        if (p > stackUsage)
            stackUsage = p;
    } else if (curNodeType == T2_CALLVAR) {
        Sub_5795(-(wB53C[procCallDepth] * 2));
        if (stackUsage < wC1C3 * 2)
            stackUsage = wC1C3 * 2;
    } else if (curNodeType == T2_RETURN || curNodeType == T2_RETURNBYTE ||
               curNodeType == T2_RETURNWORD) {
        boC1CD = true;
        Sub_5EE8();
    } else if (curNodeType == T2_JMPFALSE) {
        Sub_5795(0);
        if (boC20F) {
            cfrag1 = CF_JMPTRUE;
            boC20F = false;
        }
    } else if (curNodeType == T2_CASEBLOCK)
        Sub_5795(0);
    else if (curNodeType == T2_MOVE) {
        if (wB53C[procCallDepth] != wC1C3) {
            Sub_5795(-((wB53C[procCallDepth] + 1) * 2));
            Sub_6416(3);
        }
        if (bC045[3] == 1)
            cfrag1 = CF_MOVE_HL;
    }
} /* Sub_8F35() */

static void Sub_940D() {
    for (int i = 0; i < 4; i++) {
        if (bC04E[i] == curExprLoc[Left])
            if (bC045[i] < 2 || 5 < bC045[i])
                bC04E[i] = 0;
    }
}

static void Sub_90EB() {
    word p, q;
    byte j, k;

    p = w48DF[bC1D9] * 16;
    q = w493D[bC1D9];
    k = 0;
    if (curNodeType == T2_COLONEQUALS) {
        Sub_940D();
        if (tx2[curExprLoc[Right]].cnt == 0)
            if (tx2[curExprLoc[Left]].cnt > 0) {
                if (cfrag1 == CF_MOVMLR || cfrag1 == CF_STA) {
                    bC045[exprLoc[Right]] = BYTE_A;
                    bC04E[exprLoc[Right]] = curExprLoc[Left];
                } else if (cfrag1 == CF_SHLD || cfrag1 == CF_MOVMRP) {
                    bC045[exprLoc[Right]] = ADDRESS_A;
                    bC04E[exprLoc[Right]] = curExprLoc[Left];
                }
            }
    } else if (T2_51 <= curNodeType && curNodeType <= T2_56)
        Sub_940D();
    for (int n = 5; n < 9; n++) {
        byte i = p >> 13;
        j      = q >> 12;
        p <<= 3;
        q <<= 4;
        if (j <= 3) {
            Sub_5B96(j, n);
            if (i == 1)
                bC0A8[n]++;
            else if (i == 2) {
                if (bC045[n] == BYTE_A) {
                    bC045[n] = 6;
                } else {
                    bC045[n]  = ADDRESS_A;
                    boC057[n] = 0;
                }
            }
        } else if (j == 4) {
            boC057[k = n] = 0;
            if (0 < tx2[tx2qp].cnt) {
                bC04E[n] = tx2qp;
                bC045[n] = tx2[tx2qp].exprAttr = codeAttrLen[cfrag1] >> 5;
                bC0A8[n]                       = 0;
            } else
                bC04E[n] = 0;
        } else if (j == 5) {
            bC04E[n]  = 0;
            wC096[n]  = 0;
            bC0A8[n]  = 0;
            boC057[n] = 0xFF;
            bC045[n]  = BYTE_A;
            wC084[n]  = i;
        } else {
            bC04E[n]  = 0;
            boC057[n] = 0;
        }
    }
    if (k == 0 && tx2[tx2qp].cnt > 0) {
        for (int n = 5; n < 9; n++) {
            if (bC04E[n] == 0)
                if (!boC057[k = n])
                    break;
        }
        if (k != 0) {
            bC04E[k]            = tx2qp;
            boC057[k]           = 0;
            bC045[k]            = BYTE_A;
            tx2[tx2qp].exprAttr = BYTE_A;
            bC0A8[k]            = 0;
        }
    }
    for (int n = 0; n <= 3; n++)
        Sub_5B96(n + 5, n);
} /* Sub_90EB() */

void Sub_87CB() {
    curExprLoc[Left] = (byte)tx2[tx2qp].left;
    curExprLoc[Right] = (byte)tx2[tx2qp].right;
    first    = wAF54[curNodeType];
    last     = first + b499B[curNodeType] - 1;
    Sub_8CF5();

    while (1) {
        Sub_8DCD(); /*  OK */
        if (bC0B9[Left] == 0 && bC0B9[Right] == 0)
            break;
        if (boC1D8)
            Sub_7A85();
        else
            Sub_7DA9();
    }
    Sub_8F16();
    Sub_611A();
    Sub_5E66(w48DF[bC1D9] >> 12);
    Sub_8F35();
    Sub_84ED();
    Sub_90EB();
} /* Sub_87CB() */

void c_procedure() {
    if (EnterBlk()) {
        blk[blkId].codeSize  = codeSize;
        blk[blkId].wB4B0     = wC1C3;
        blk[blkId].stackSize = stackUsage;
        blk[blkId].extProcId = curExtProcId;
        blk[activeGrpCnt].next  = blkId;
        blkId                = activeGrpCnt;
        info = blk[activeGrpCnt].info = FromIdx(tx2[tx2qp].left);
        curExtProcId               = info->procId;
        codeSize                   = 0;
        EmitTopItem();
        Sub_981C();
    }
}
