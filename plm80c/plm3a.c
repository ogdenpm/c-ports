/****************************************************************************
 *  plm3a.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

byte b4813[] = { 3, 7, 3, 7,   2, 3,   8, 1, 3,   1, 8,    2, 3, 8,   1, 3,    1, 8, 3,   7,
                 3, 7, 2, 3,   8, 1,   3, 1, 8,   2, 0x1D, 3, 1, 7,   2, 0x12, 2, 1, 0xA, 2,
                 1, 8, 2, 1,   8, 2,   1, 7, 3,   7, 3,    7, 2, 3,   8, 1,    3, 1, 8,   1,
                 6, 3, 1, 0xB, 1, 6,   3, 1, 0xB, 1, 6,    1, 6, 3,   7, 3,    7, 2, 3,   8,
                 3, 8, 3, 9,   1, 6,   3, 1, 6,   1, 7,    3, 1, 0xC, 3, 7,    3, 7, 2,   3,
                 8, 3, 8, 3,   9, 0xC, 3, 7, 3,   7, 2,    3, 8, 1,   3, 1,    8 };

byte              /* tx1Buf[1280], use plm0a.c nmsBuf[1280], use main6.c */
    objBuf[1280]; // use buffer in plm0a.c
word w7197;
byte b7199;
byte rec2[54]     = { 2, 0, 0 };
byte rec18[303]   = { 0x18, 0, 0 };
byte rec16_1[304] = { 0x16, 0, 0, 0 };
byte rec16_2[304] = { 0x16, 0, 0, 1 };
byte rec16_3[304] = { 0x16, 0, 0, 2 };
byte rec16_4[304] = { 0x16, 0, 0, 4 };
byte rec12[1024]  = { 0x12, 0, 0, 1 };
byte rec6[306]    = { 6, 0, 0, 0 };
byte rec24_2[154] = { 0x24, 0, 0, 2, 3 };
byte rec24_3[154] = { 0x24, 0, 0, 4, 3 };

void FlushRecGrp() {
    WriteRec(rec6, 3);
    WriteRec(rec22, 1);
    WriteRec(rec24_1, 2);
    WriteRec(rec24_2, 2);
    WriteRec(rec24_3, 2);
    WriteRec(rec20, 1);
}

void RecAddName(pointer recP, byte offset, byte len, char const *str) {
    RecAddByte(recP, offset, len);
    for (int i = 0; i != len; i++)
        RecAddByte(recP, offset, (byte)str[i]);
}

void ExtendChk(pointer arg1wP, word arg2w, byte arg3b) {
    if (getWord(arg1wP + 1) + arg3b >= arg2w) {
        FlushRecGrp();
        putWord(&rec6[CONTENT_OFF], w7197);
    }
}

word Sub_4938() {
    word p, q;
    q = 0;
    for (p = 1; p <= procCnt; p++) {
        SetInfo(procInfo[p]);
        if (q < info->baseVal)
            q = info->baseVal;
    }
    return q;
}

word Sub_4984() {
    switch (info->type) {
    case BYTE_T:
        return 1;
    case ADDRESS_T:
        return 2;
    case STRUCT_T:
        return info->totalSize;
    }
    return 0;
}

void Sub_49BC(word arg1w, word arg2w, word arg3w) {
    if (b7199) {
        Wr1Byte(0xa3);
        Wr1Word(arg1w);
        Wr1Word(arg2w);
        Wr1Word(arg3w);
    } else
        programErrCnt++;
}

extern union {
    byte str[256];
    struct {
        byte type;
        offset_t infoP;
        word stmt;
        var_t var;
    };
} atFData;

static byte dat[255];

static word w8115, w8117, w8119, w811B;
static bool b811D;
static word w811E;

static void Sub_4B6C() {
    if (infoIdx == 0 || !(info->flag & F_MEMBER)) {
        if (w8119 > 1) {
            w8119--;
            SetInfo(atFData.infoP);
        } else if (infoIdx == 0) {
            b811D = false;
            return;
        } else {
            if (!(info->flag & F_PACKED))
                b811D = false;
            if (info->type == STRUCT_T) {
                if ((info->flag & F_ARRAY))
                    w8119 = info->dim;
                AdvNxtInfo();
                atFData.infoP = infoIdx;
            }
        }
    }
    if ((info->flag & F_ARRAY))
        w811B = info->dim;
}

static void Sub_4BF4() {
    if (!b811D || w8117 >= w8115) {
        atFData.type = RdAtByte();
        switch (atFData.type) {
        case ATI_2:
            atFData.var.val = RdAtWord();
            break;
        case ATI_STRING:
            w8115 = RdAtWord();
            vfRbuf(&atf, dat, w8115);
            w8117 = 0;
            break;
        case ATI_DATA:
            RdAtData();
            break;
        case ATI_END:
            break;
        }
    }
}

static void Sub_4CAC() {
    if (info->type == BYTE_T) {
        ExtendChk(rec6, 0x12c, 1);
        RecAddByte(rec6, 3, (byte)atFData.var.val);
        w7197++;
    } else {
        ExtendChk(rec6, 0x12C, 2);
        RecAddWord(rec6, 3, atFData.var.val);
        w7197 += 2;
    }
}

static void Sub_4CF9() {
    Sub_49BC(0xd2, infoIdx, atFData.stmt);
    Sub_4CAC();
}

static void Sub_4D13() {
    atFData.var.val = dat[w8117++];
    if (info->type != BYTE_T && w8117 < w8115)
        atFData.var.val = (atFData.var.val << 8) + dat[w8117++];
    Sub_4CAC();
}

static void Sub_4D85() {
    if (atFData.var.val > 255 && info->type == BYTE_T)
        Sub_4CF9();
    else
        Sub_4CAC();
}

static void Sub_4DA8() {
    byte i, j;
    index_t savIdx;
    pointer recPtr;

    if (atFData.var.infoOffset == 0)
        Sub_4D85();
    else if (info->type == BYTE_T)
        Sub_4CF9();
    else {
        savIdx       = infoIdx;
        SetInfo(atFData.var.infoOffset);
        if ((info->flag & F_MEMBER)) {
            atFData.var.val =
                atFData.var.val + Sub_4984() * atFData.var.nestedArrayIndex + info->linkVal;
            SetInfo(info->parent);
        }

        atFData.var.val += Sub_4984() * atFData.var.arrayIndex + info->linkVal;
        if ((info->flag & F_EXTERNAL)) {
            i       = info->extId;
            SetInfo(savIdx);
            ExtendChk(rec20, 0x95, 4);
            Sub_4CAC();
            RecAddWord(rec20, 1, i);
            RecAddWord(rec20, 1, w7197 - 2);
        } else if ((info->flag & F_ABSOLUTE)) {
            SetInfo(savIdx);
            Sub_4CAC();
        } else {
            if (info->type == PROC_T || info->type == LABEL_T || (info->flag & F_DATA)) {
                recPtr = rec24_1;
                i = 1;
            } else if ((info->flag & F_MEMORY)) {
                recPtr = rec24_3;
                i = 4;
            } else {
                recPtr = rec24_2;
                i = 2;
            }

            if (i == rec6[CONTENT_SEG]) {
                recPtr = rec22;
                j = 1;
            } else
                j = 2;

            SetInfo(savIdx);
            ExtendChk(recPtr, 0x95, 2);
            Sub_4CAC();
            RecAddWord(recPtr, j, w7197 - 2);
        }
    }
} /* Sub_4DA8() */

static void Sub_4C7A() {
    if (b811D) {
        switch (atFData.type) {
        case ATI_2:
            Sub_4D85();
            break;
        case ATI_STRING:
            Sub_4D13();
            break;
        case ATI_DATA:
            Sub_4DA8();
            break;
        }
    }
}

static void Sub_4A31() {
    atFData.infoP = RdAtWord();
    atFData.stmt  = RdAtWord();
    SetInfo(w811E = atFData.infoP);
    w8119 = w811B = w8115 = w8117 = 0;
    if ((info->flag & F_DATA))
        rec6[CONTENT_SEG] = 1;
    else if ((info->flag & F_MEMORY))
        rec6[CONTENT_SEG] = 4;
    else if ((info->flag & F_ABSOLUTE))
        rec6[CONTENT_SEG] = 0;
    else
        rec6[CONTENT_SEG] = 2;

    w7197 = putWord(&rec6[CONTENT_OFF], info->linkVal);
    if (infoIdx == 0)
        b811D = false;
    else if ((info->flag & F_EXTERNAL)) {
        Sub_49BC(0xd9, w811E, atFData.stmt);
        b811D = false;
    } else {
        Sub_4B6C();
        b811D = true;
    }

    Sub_4BF4();
    if (atFData.type == ATI_END) {
        FlushRecGrp();
        return;
    }

    while (1) {
        Sub_4C7A();
        Sub_4BF4();
        if (atFData.type == ATI_END) {
            FlushRecGrp();
            return;
        } else if (b811D) {
            if (w811B > 1)
                w811B--;
            else {
                do {
                    AdvNxtInfo();
                } while (infoIdx && !(BYTE_T <= info->type && info->type <= STRUCT_T));
                Sub_4B6C();
                if (!b811D)
                    Sub_49BC(0xd1, w811E, atFData.stmt);
            }
        }
    }
}

void Sub_49F9() {
    while (1) {
        atFData.type = RdAtByte();
        if (atFData.type == ATI_AHDR) {
            RdAtHdr();
            RdAtData();
        } else if (atFData.type == ATI_DHDR)
            Sub_4A31();
        else
            return;
    }
}