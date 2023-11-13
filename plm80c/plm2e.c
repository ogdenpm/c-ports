/****************************************************************************
 *  plm2e.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "../shared/os.h"
#include "plm.h"
#include <stdio.h>

static byte b7A81[4] = { 0x3C, 0x46, 0x50, 0x5A };

static byte bC28B[4], bC28F;

static void Sub_7D32() {
    EncodeFragData(CF_XCHG);
    codeSize++;
    Sub_5B96(3, 4);
    Sub_5B96(2, 3);
    Sub_5B96(4, 2);
}

static void Sub_7D54() {
    wC1DC[0] = 0xa;
    wC1DC[1] = wC1C3;
    EncodeFragData(CF_XTHL);
    codeSize++;
    Sub_5C97(4);
    Sub_5C1D(3);
    Sub_5B96(4, 3);
}

static void Sub_7D7E(byte arg1b) {
    if (arg1b <= 3)
        if (bC28B[arg1b] < 0xc8)
            bC28B[arg1b] += bC28F;
}

void Sub_7A85() {
    byte j, k;

    Sub_597E();
    for (byte i = 0; i <= 3; i++) {
        if (boC072[i] || boC069[i])
            bC28B[i] = 0xc8;
        else if (boC060[i])
            bC28B[i] = b7A81[i] + 0xf;
        else
            bC28B[i] = b7A81[i];
    }

    if ((bC0C3[wC1C3] >> 4) != 0xb && bC140[wC1C3] != 0)
        bC28B[0] = 0xC8;
    for (byte m = 0; m <= 1; m++) {
        if (bC0B7[m] != 0) {
            if (bC0B5[m] == 9 && bC140[wC1C3] == bC0B7[m]) {
                k      = m;
                bC28F  = 0xce;
                boC1D8 = false;
            } else
                bC28F = 0x32;
            Sub_7D7E(b52B5[bC0BF[m]]);
            Sub_7D7E(b4C2D[bC0BD[m]] >> 5);
        }
    }

    byte n = 0xc8;
    for (byte i = 0; i <= 3; i++) {
        if (bC28B[i] <= n)
            n = bC28B[j = i];
    }

    if (n == 0xC8) {
        if (boC069[3]) {
            Sub_7D32();
            if (bC0B5[0] == 3) {
                bC0B5[0] = 2;
                Sub_61A9(0);
            } else {
                bC0B5[1] = 2;
                Sub_61A9(1);
            }
        }
        Sub_7D54();
        j = 3;
    } else
        Sub_6416(j);

    if (bC045[j] == 0xb) {
        bC045[j] = 0;
        if (j != 0 && bC04E[j] != 0) {
            wC1DC[0] = j;
            EncodeFragData(CF_MOVLRHR);
            codeSize++;
        }
    }
    if (!boC1D8) {
        if (bC0B5[1 - k] == 9) {
            if (bC0B7[1 - k] == bC0B7[k]) {
                bC0B5[1 - k] = j;
                bC0B3[1 - k] = bC045[j];
                Sub_61A9(1 - k);
            } else
                boC1D8 = true;
        }
        bC0B5[k] = j;
        bC0B3[k] = bC045[j];
        Sub_61A9(k);
    }
}

static byte bC294, bC295, bC296, bC297, bC298;

static bool Sub_7FD9(byte arg1b) {
    if (arg1b <= 3)
        if (bC0B5[bC296] == arg1b)
            return true;
    return false;
}

static void Sub_7F19() {
    for (bC295 = 0; bC295 <= 1; bC295++) {
        if (bC0B7[bC295] != 0 && bC0BB[bC295] != 0) {
            if (bC0BB[bC296 = 1 - bC295] != 0) {
                if (Sub_7FD9(b4C2D[bC0BD[bC295]] >> 5) || Sub_7FD9(b52B5[bC0BF[bC295]]))
                    bC0BB[bC296] = 10 + bC0BB[bC296];
            }
        }
    }

    if (bC0BB[0] > bC0BB[1])
        bC295 = 0;
    else
        bC295 = 1;

    bC296 = 1 - bC295;
    bC298 = bC0BD[bC295];
}

static void Sub_7FFC() {
    if (bC298 == 0x13) {
        if (boC072[3] || boC069[3])
            bC298 = 0x15;
    } else if (bC298 == 0x14) {
        bC294 = 5 - bC297;
        if (bC0B5[0] == bC0B5[1])
            bC298 = bC0B3[bC295] == 0 ? 0xE : 0x11;
    } else if (bC298 == 8) {
        if (tx2[bC0B7[bC295]].left != 0) {
            bC298 = 6;
            if (bC294 == 0)
                bC294 = 1;
        }
    }
}

static void Sub_8086() {
    if (9 <= bC298 && bC298 <= 13)
        Sub_63AC(bC297);

    if (b4C2D[bC298] & 1) {
        if (bC294 != bC297)
            Sub_5D6B(bC294);
        else if (9 <= bC298 && bC298 <= 13) {
            bC0B5[bC295] = 9;
            Sub_597E();
            Sub_5D6B(bC294);
            bC0B5[bC295] = bC297;
            Sub_597E();
        }
    } else if (bC298 == 0x15) {
        if (boC069[3])
            bC0B5[bC296] = 9;
    } else if (bC298 == 0x14) {
        if (bC0B5[bC296] == bC294)
            bC0B5[bC296] = bC297;
    }
}

static void Sub_8148(byte arg1b, byte arg2b) {
    //	byte i;
    if (arg2b == 0)
        return;
    if (arg2b == 1 || arg2b == 2) {
        if (arg1b == 0)
            wC1DC[bC1DB] = bC297;
        else
            wC1DC[bC1DB] = bC294;
        bC1DB++;
    } else if (arg1b == 2) {
        wC1DC[bC1DB] = 8;
        bC1DB        = bC1DB + 2;
    } else if (arg1b == 3) {
        wC1DC[bC1DB]     = 0xA;
        wC1DC[bC1DB + 1] = wC1C3;
        bC1DB            = bC1DB + 3;
    } else {
        //		i = bC1DB;
        Sub_636A(bC295);
    }
}

static void Sub_8207() {
    switch (b4C15[bC298] >> 4) {
    case 0:
        Sub_5C97(bC294);
        wC1C3--;
        break;
    case 1:
        Sub_5C97(4);
        Sub_5C1D(3);
        Sub_5B96(4, 3);
        break;
    case 2:
        boC057[bC294] = true;
        bC0A8[bC294]  = 0;
        bC04E[bC294]  = bC0B7[bC295];
        wC096[bC294]  = 0x100;
        if (wC1DC[0] == 0xA) {
            wC084[bC294] = -(wC1DC[1] * 2);
            if (bC0C3[tx2[bC0B7[bC295]].extra] == 0xb0)
                if (bC298 == 5) {
                    wC084[bC294]--;
                    wC1DC[2] = wC1DC[2] + 1;
                }
        } else
            wC084[bC294] = wC1DC[3] - wC1C3 * 2;
        break;
    case 3:
        boC057[bC294] = true;
        bC0A8[bC294]  = 0;
        bC04E[bC294]  = bC0B7[bC295];
        GetVal(bC04E[bC294], &wC084[bC294], &wC096[bC294]);
        break;
    case 4:
        boC057[bC294] = 0;
        bC04E[bC294]  = bC0B7[bC295];
        if (bC0B3[bC295] == 4 || bC0B3[bC295] == 5) {
            bC0A8[bC294] = bC0C3[tx2[bC0B7[bC295]].extra] & 0xf;
            if (bC0A8[bC294] > 7)
                bC0A8[bC294] = bC0A8[bC294] | 0xf0;
        } else
            bC0A8[bC294] = 0;
        break;
    case 5:
        Sub_5B96(bC297, bC294);
        break;
    case 6:
        Sub_5B96(3, 4);
        Sub_5B96(2, 3);
        Sub_5B96(4, 2);
        break;
    case 7:
        break;
    }
}

static void Sub_841E() {
    switch (b4C15[bC298] & 0xf) {
    case 0:
        break;
    case 1:
        bC045[bC294] = 1;
        break;
    case 2:
        bC045[bC294] = 0;
        break;
    case 3:
        bC045[bC294] = 6;
        break;
    case 4:
        if (bC0B5[bC295] != 8)
            bC045[bC294] = IndirectAddr(bC0B3[bC295]);
        else if (bC0B3[bC295] == 0)
            bC045[bC294] = 6;
        else
            bC045[bC294] = bC0B3[bC295];
        break;
    case 5:
        bC045[bC294] = bC0B3[bC295] - 2;
        bC0A8[3]     = bC0A8[3] + 1;
        break;
    }
}

void Sub_7DA9() {
    byte fragCode;

    Sub_7F19();
    if (bC298 == 0x17)
        Sub_58F5(ERR214);
    else if (bC298 == 0x16) {
        bC0C1[bC295] = bC0BF[bC295];
        bC0B3[bC295] = b528D[bC0C1[bC295]];
        bC0B5[bC295] = b52B5[bC0C1[bC295]];
    } else if (bC298 == 0x12)
        boC1D8 = true;
    else {
        bC294 = b4C2D[bC298] >> 5;
        if (bC294 > 3)
            bC294 = b52B5[bC0BF[bC295]];
        bC297 = bC0B5[bC295];
        Sub_597E();
        Sub_7FFC();
        fragCode = b5012[bC298];
        Sub_8086();
        bC1DB = 0;
        Sub_8148((b4C2D[bC298] >> 3) & 3, (b4029[fragCode] >> 4) & 7);
        Sub_8148((b4C2D[bC298] >> 1) & 3, (b4029[fragCode] >> 1) & 7);
        Sub_8207();
        Sub_841E();
        bC0B3[bC295] = bC045[bC294];
        bC0B5[bC295] = bC294;
        Sub_61A9(0);
        Sub_61A9(1);
        EncodeFragData(fragCode);
        codeSize += (b43F8[fragCode] & 0x1f);
    }
}

static byte Sub_8683(byte arg1b) {
    if (bC0B5[0] == arg1b)
        return 1;
    else
        return 0;
}

static void Sub_8698(byte arg1b, byte arg2b) {
    byte i;
    word p;
    switch (arg1b) {
    case 0:
        return;
    case 1:
        i = 0;
        break;
    case 2:
        i = 1;
        break;
    case 3:
        wC1DC[bC1DB]     = 0xA;
        wC1DC[bC1DB + 1] = wC1C3;
        bC1DB            = bC1DB + 3;
        return;
    case 4:
        i = Sub_8683(3);
        break;
    case 5:
        i = Sub_8683(0);
        break;
    case 6:
        if (arg2b == 7) {
            wC1DC[bC1DB]     = 0x10;
            wC1DC[bC1DB + 1] = tx2[tx2qp].extra;
            bC1DB += 2;
        } else
            Sub_61E0((byte)tx2[tx2qp].extra);
        return;
    default:
        fprintf(stderr, "out of bounds in Sub_8698 arg1b = %d\n", arg1b);
        Exit(1);
    }
    if (arg2b <= 3)
        Sub_636A(i);
    else {
        wC1DC[bC1DB] = arg2b + 9;
        if (arg2b == 6)
            wC1DC[bC1DB + 1] = tx2[1].right;
        else
            GetVal(bC0B7[i], &wC1DC[bC1DB + 1], &p);
        bC1DB += 2;
    }
}

void Sub_84ED() {
    if (cfrag1 > CF_3) {
        bC1DB = 0;
        Sub_8698(b42F9[cfrag1] >> 4, (b4029[cfrag1] >> 4) & 7);
        if (cfrag1 == CF_67 || cfrag1 == CF_68)
            wC1DC[bC1DB - 1] += 2;
        Sub_8698(b42F9[cfrag1] & 0xf, (b4029[cfrag1] >> 1) & 7);
        EncodeFragData(cfrag1);
        codeSize += (b43F8[cfrag1] & 0x1f);
        if (cfrag1 == CF_DELAY) {
            helperAddr[105] = 1;
            if (stackUsage < (wC1C3 + 1) * 2)
                stackUsage = (wC1C3 + 1) * 2;
        } else if (cfrag1 > CF_171) {
            byte i     = b4128[b413B[cfrag1 - CF_174]];
            byte grp     = helperGroup[b4273[curNodeType]];
            helperAddr[helperMap[grp][i]] = 1;      // mark as used
            if (curNodeType == T2_SLASH || curNodeType == T2_MOD || curNodeType == T2_44) {
                if (stackUsage < (wC1C3 + 2) * 2)
                    stackUsage = (wC1C3 + 2) * 2;
            } else if (stackUsage < (wC1C3 + 1) * 2)
                stackUsage = (wC1C3 + 1) * 2;
        }
    }
}
