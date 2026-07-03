/****************************************************************************
 *  plm3a.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

uint8_t helperCodeLen[] = { 3, 7, 3, 7,   2, 3,   8, 1, 3,   1, 8,    2, 3, 8,   1, 3,    1, 8, 3,   7,
                         3, 7, 2, 3,   8, 1,   3, 1, 8,   2, 0x1D, 3, 1, 7,   2, 0x12, 2, 1, 0xA, 2,
                         1, 8, 2, 1,   8, 2,   1, 7, 3,   7, 3,    7, 2, 3,   8, 1,    3, 1, 8,   1,
                         6, 3, 1, 0xB, 1, 6,   3, 1, 0xB, 1, 6,    1, 6, 3,   7, 3,    7, 2, 3,   8,
                         3, 8, 3, 9,   1, 6,   3, 1, 6,   1, 7,    3, 1, 0xC, 3, 7,    3, 7, 2,   3,
                         8, 3, 8, 3,   9, 0xC, 3, 7, 3,   7, 2,    3, 8, 1,   3, 1,    8 };

uint16_t curOffset;
uint8_t printOrObj;
uint8_t recModHdr[54]        = { 2, 0, 0 };
uint8_t recExternals[303]    = { 0x18, 0, 0 };
uint8_t recPublicAbs[304]    = { 0x16, 0, 0, 0 };
uint8_t recPublicCode[304]   = { 0x16, 0, 0, 1 };
uint8_t recPublicData[304]   = { 0x16, 0, 0, 2 };
uint8_t recPublicMemory[304] = { 0x16, 0, 0, 4 };
uint8_t recLocals[1024]      = { 0x12, 0, 0, 1 };
uint8_t recInitContent[306]  = { 6, 0, 0, 0 };
uint8_t recDataFixup[154]    = { 0x24, 0, 0, 2, 3 };
uint8_t recMemoryFixup[154]  = { 0x24, 0, 0, 4, 3 };

void FlushRecGrp() {
    WriteRec(recInitContent, 3);
    WriteRec(recSelfFixup, 1);
    WriteRec(recCodeFixup, 2);
    WriteRec(recDataFixup, 2);
    WriteRec(recMemoryFixup, 2);
    WriteRec(recExtFixup, 1);
}

void RecAddName(pointer recP, uint8_t offset, pstr_t const *pstr) {
    uint16_t rlen    = getWord(&recP[REC_LEN]);
    pointer outP = recP + REC_DATA + rlen + offset;
    memcpy(outP, pstr, pstr->len + 1);
    putWord(&recP[REC_LEN], rlen + pstr->len + 1);
}

void ExtendChk(pointer recP, uint16_t limit, uint8_t toAdd) {
    if (getWord(&recP[REC_LEN]) + toAdd >= limit) {
        FlushRecGrp();
        putWord(&recInitContent[CONTENT_OFF], curOffset);
    }
}

uint16_t CalcMaxStack() {
    uint16_t maxStack = 0;
    for (int i = 1; i <= procCnt; i++) {
        if (maxStack < procInfo[i]->stackUsage)
            maxStack = procInfo[i]->stackUsage;
    }
    return maxStack;
}

void p3Error(uint16_t errNum, info_t *tokInfo, uint16_t stmt) {
    if (printOrObj) {
        Wr1Byte(T2_ERROR);
        Wr1Word(errNum);
        Wr1Info(tokInfo); // scale to index
        Wr1Word(stmt);
    } else
        programErrCnt++;
}

extern union {
    uint8_t str[256];
    struct {
        uint8_t type;
        offset_t infoP;
        uint16_t stmt;
        var_t var;
    };
} atFData;

static uint8_t dat[255];

static uint16_t stringLen, stringIdx, structDim, itemDim;
static bool moreToInit;
static info_t *atInfo;

/*
    Advances the info pointer to the next variable in the initialisation sequence. Handles
    structure-array repetition (structDim), end-of-packed detection (moreToInit = false), descent into
    STRUCT_T members, and sets itemDim for array variables. It positions the "current target" for the
    next value to be written.
*/
static void AdvanceInitTarget() {   // Sub_4B6C
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

/*
    Reads the next initialiser record from the attribute file (atf) into atFData. Handles ATI_2 (numeric
    constant), ATI_STRING (byte buffer), ATI_DATA (variable reference / var_t), and ATI_END. Only
    fetches when the current string buffer is exhausted or moreToInit is false.
*/
static void FetchNextInitValue() {  // Sub_4BF4
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
        RecAddByte(recInitContent, 3, (uint8_t)atFData.var.val);
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

/*
    Consumes one or two bytes from the string buffer (dat[stringIdx++]) packing them into val, then calls EmitInitItem().
    For BYTE_T targets one byte is used; for ADDRESS_T two bytes are merged into a word.
*/
static void EmitStringInitItem() {  // Sub_4D13
    atFData.var.val = dat[stringIdx++];
    if (info->type != BYTE_T && stringIdx < stringLen)
        atFData.var.val = (atFData.var.val << 8) + dat[stringIdx++];
    EmitInitItem();
}

/*
    Validates that a numeric constant fits the target type (ERR210 if a value > 255 is written to a BYTE_T)
    then calls EmitInitItem(). Handles the plain ATI_2 numeric-constant case.
*/
static void EmitConstInitItem() {  // Sub_4D85
    /* Chk for ILLEGAL INITIALIZATION OF A BYTE TO A VALUE > 255 */
    if (atFData.var.val > 255 && info->type == BYTE_T)
        p3Error(ERR210, info, atFData.stmt);
    EmitInitItem();
}


/*
    The most complex emit path. If the initialiser has no variable reference, delegates to
    EmitConstInitItem. If it references a variable, computes the byte address (with member and subscript
    offsets) and emits the value plus the appropriate fixup record (recExtFixup, recCodeFixup,
    recDataFixup, recMemoryFixup, or recSelfFixup). Handles the ATI_DATA address-reference case.
*/
static void EmitRelocInitItem() { //Sub_4DA8

    if (atFData.var.infoIdx == 0)
        EmitConstInitItem();
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
} /* EmitRelocInitItem() */

/*
    Dispatcher: if moreToInit is set, calls EmitStringInitItem, EmitConstInitItem, or EmitRelocInitItem
    depending on the current type. Acts as the single call-site that routes to the three concrete
    emitters.
*/
static void EmitCurrentInitItem() {  // Sub_4C7A
    if (moreToInit) {
        switch (atFData.type) {
        case ATI_2:
            EmitConstInitItem();
            break;
        case ATI_STRING:
            EmitStringInitItem();
            break;
        case ATI_DATA:
            EmitRelocInitItem();
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
        AdvanceInitTarget();
        moreToInit = true;
    }

    FetchNextInitValue();
    if (atFData.type == ATI_END) {
        FlushRecGrp();
        return;
    }

    while (1) {
        EmitCurrentInitItem();
        FetchNextInitValue();
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
                AdvanceInitTarget();
                if (!moreToInit)
                    p3Error(ERR209, atInfo,
                            atFData.stmt); /* ILLEGAL INITIALIZATION OF MORE SPACE THAN DECLARED */
            }
        }
    }
}

/*
    Top-level loop over the attribute file. Skips ATI_AHDR records (AT-clause headers, already processed
    in phase 2) and dispatches ATI_DHDR records to EmitInitData() to emit data/INITIAL content and fixup
    records into the object file. Returns when any other record type (including ATI_EOF) is seen.
*/
void ProcessInitRecords() { // Sub_49F9
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
