/****************************************************************************
 *  plm3a.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

byte helperCodeLen[] = { 3, 7, 3, 7,   2, 3,   8, 1, 3,   1, 8,    2, 3, 8,   1, 3,    1, 8, 3,   7,
                         3, 7, 2, 3,   8, 1,   3, 1, 8,   2, 0x1D, 3, 1, 7,   2, 0x12, 2, 1, 0xA, 2,
                         1, 8, 2, 1,   8, 2,   1, 7, 3,   7, 3,    7, 2, 3,   8, 1,    3, 1, 8,   1,
                         6, 3, 1, 0xB, 1, 6,   3, 1, 0xB, 1, 6,    1, 6, 3,   7, 3,    7, 2, 3,   8,
                         3, 8, 3, 9,   1, 6,   3, 1, 6,   1, 7,    3, 1, 0xC, 3, 7,    3, 7, 2,   3,
                         8, 3, 8, 3,   9, 0xC, 3, 7, 3,   7, 2,    3, 8, 1,   3, 1,    8 };

word curOffset;
byte printOrObj;
byte recModHdr[54]        = { 2, 0, 0 };
byte recExternals[303]    = { 0x18, 0, 0 };
byte recPublicAbs[304]    = { 0x16, 0, 0, 0 };
byte recPublicCode[304]   = { 0x16, 0, 0, 1 };
byte recPublicData[304]   = { 0x16, 0, 0, 2 };
byte recPublicMemory[304] = { 0x16, 0, 0, 4 };
byte recLocals[1024]      = { 0x12, 0, 0, 1 };
byte recInitContent[306]  = { 6, 0, 0, 0 };
byte recDataFixup[154]    = { 0x24, 0, 0, 2, 3 };
byte recMemoryFixup[154]  = { 0x24, 0, 0, 4, 3 };

void FlushRecGrp() {
    WriteRec(recInitContent, 3);
    WriteRec(recSelfFixup, 1);
    WriteRec(recCodeFixup, 2);
    WriteRec(recDataFixup, 2);
    WriteRec(recMemoryFixup, 2);
    WriteRec(recExtFixup, 1);
}

void RecAddName(pointer recP, byte offset, pstr_t const *pstr) {
    word rlen    = getWord(&recP[REC_LEN]);
    pointer outP = recP + REC_DATA + rlen + offset;
    memcpy(outP, pstr, pstr->len + 1);
    putWord(&recP[REC_LEN], rlen + pstr->len + 1);
}

void ExtendChk(pointer recP, word limit, byte toAdd) {
    if (getWord(&recP[REC_LEN]) + toAdd >= limit) {
        FlushRecGrp();
        putWord(&recInitContent[CONTENT_OFF], curOffset);
    }
}

word CalcMaxStack() {
    word maxStack = 0;
    for (int i = 1; i <= procCnt; i++) {
        if (maxStack < procInfo[i]->stackUsage)
            maxStack = procInfo[i]->stackUsage;
    }
    return maxStack;
}

void p3Error(word errNum, info_t *tokInfo, word stmt) {
    if (printOrObj) {
        Wr1Byte(T2_ERROR);
        Wr1Word(errNum);
        Wr1Info(tokInfo); // scale to index
        Wr1Word(stmt);
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

static word stringLen, stringIdx, structDim, itemDim;
static bool moreToInit;
static info_t *atInfo;

static void Sub_4B6C() {
    if (!info || !(info->flag & F_MEMBER)) {
        if (structDim > 1) {
            structDim--;
            info = FromIdx(atFData.infoP); // restart for next iteration
        } else if (!info) {
            moreToInit = false;
            return;
        } else {
            if (!(info->flag & F_PACKED))
                moreToInit = false;
            if (info->type == STRUCT_T) {
                if ((info->flag & F_ARRAY))
                    structDim = info->dim;
                AdvNxtInfo();
                atFData.infoP = ToIdx(info);
            }
        }
    }
    if ((info->flag & F_ARRAY))
        itemDim = info->dim;
}

static void Sub_4BF4() {
    if (!moreToInit || stringIdx >= stringLen) {
        atFData.type = RdAtByte();
        switch (atFData.type) {
        case ATI_2:
            atFData.var.val = RdAtWord();
            break;
        case ATI_STRING:
            stringLen = RdAtWord();
            vfRbuf(&atf, dat, stringLen);
            stringIdx = 0;
            break;
        case ATI_DATA:
            RdAtData();
            break;
        case ATI_END:
            break;
        }
    }
}

static void EmitInitItem() {
    if (info->type == BYTE_T) {
        ExtendChk(recInitContent, 300, 1);
        RecAddByte(recInitContent, 3, (byte)atFData.var.val);
        curOffset++;
    } else {
        ExtendChk(recInitContent, 300, 2);
        RecAddWord(recInitContent, 3, atFData.var.val);
        curOffset += 2;
    }
}

static void BadInit() {
    p3Error(ERR210, info, atFData.stmt); /* ILLEGAL INITIALIZATION OF A BYTE TO A VALUE > 255 */
    EmitInitItem();
}

static void Sub_4D13() {
    atFData.var.val = dat[stringIdx++];
    if (info->type != BYTE_T && stringIdx < stringLen)
        atFData.var.val = (atFData.var.val << 8) + dat[stringIdx++];
    EmitInitItem();
}

static void Sub_4D85() {
    /* Chk for ILLEGAL INITIALIZATION OF A BYTE TO A VALUE > 255 */
    if (atFData.var.val > 255 && info->type == BYTE_T)
        p3Error(ERR210, info, atFData.stmt);
    EmitInitItem();
}

static void Sub_4DA8() {

    if (atFData.var.infoIdx == 0)
        Sub_4D85();
    else if (info->type == BYTE_T)
        BadInit();
    else {
        info_t *atInfo = FromIdx(atFData.var.infoIdx);
        if ((atInfo->flag & F_MEMBER)) {
            atFData.var.val = atFData.var.val +
                              GetElementSize(atInfo) * atFData.var.memberSubscript +
                              atInfo->linkVal;
            atInfo = atInfo->parent;
        }

        atFData.var.val += GetElementSize(atInfo) * atFData.var.subscript + atInfo->linkVal;
        if ((atInfo->flag & F_EXTERNAL)) {
            ExtendChk(recExtFixup, 149, 4);
            EmitInitItem();
            RecAddWord(recExtFixup, 1, atInfo->extId);
            RecAddWord(recExtFixup, 1, curOffset - 2);
        } else if ((atInfo->flag & F_ABSOLUTE)) {
            EmitInitItem();
        } else {
            pointer fixupRec;
            if (atInfo->type == PROC_T || atInfo->type == LABEL_T || (atInfo->flag & F_DATA))
                fixupRec = recCodeFixup;
            else if ((atInfo->flag & F_MEMORY))
                fixupRec = recMemoryFixup;
            else
                fixupRec = recDataFixup;
            if (recInitContent[CONTENT_SEG] == fixupRec[FIXUP_SEG]) // see if self ref fixup
                fixupRec = recSelfFixup;

            ExtendChk(fixupRec, 149, 2);
            EmitInitItem();
            RecAddWord(fixupRec, fixupRec == recSelfFixup ? 1 : 2, curOffset - 2); // add fixup
        }
    }
} /* Sub_4DA8() */

static void Sub_4C7A() {
    if (moreToInit) {
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

static void EmitInitData() {
    info         = FromIdx(atFData.infoP = RdAtWord());
    atFData.stmt = RdAtWord();
    atInfo       = info;
    structDim = itemDim = stringLen = stringIdx = 0;
    if ((info->flag & F_DATA))
        recInitContent[CONTENT_SEG] = S_CODE;
    else if ((info->flag & F_MEMORY))
        recInitContent[CONTENT_SEG] = S_MEMORY;
    else if ((info->flag & F_ABSOLUTE))
        recInitContent[CONTENT_SEG] = S_ABS;
    else
        recInitContent[CONTENT_SEG] = S_DATA;

    curOffset = putWord(&recInitContent[CONTENT_OFF], info->addr);
    if (!info)
        moreToInit = false;
    else if ((info->flag & F_EXTERNAL)) {
        p3Error(ERR217, atInfo, atFData.stmt); /* ILLEGAL INITIALIZATION OF AN EXTERNAL VARIABLE */
        moreToInit = false;
    } else {
        Sub_4B6C();
        moreToInit = true;
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
        } else if (moreToInit) {
            if (itemDim > 1)
                itemDim--;
            else {
                do {
                    AdvNxtInfo();
                } while (info && !(BYTE_T <= info->type && info->type <= STRUCT_T));
                Sub_4B6C();
                if (!moreToInit)
                    p3Error(ERR209, atInfo,
                            atFData.stmt); /* ILLEGAL INITIALIZATION OF MORE SPACE THAN DECLARED */
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
            EmitInitData();
        else
            return;
    }
}
