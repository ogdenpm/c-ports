/****************************************************************************
 *  plm4c.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "../shared/os.h"
#include "plm.h"
#include <stdio.h>

static byte ccBits[]  = { 0x10, 0x18, 8, 0, 0x18, 0x10 };
static char *ccCodes[] = { "NC", "C", "Z", "NZ", "C", "NC" };

// lifted to file scope for nested procedures
static word codePtr;
static byte codeByte;

pstr_t *pstrcpy(pstr_t *dst, pstr_t const *src) {
    memcpy(dst, src, src->len + 1);
    return dst;
}

pstr_t *pcstrcpy(pstr_t *dst, char const *src) {
    dst->len = (byte)strlen(src);
    memcpy(dst->str, src, dst->len);
    return dst;
}

pstr_t *pstrcat(pstr_t *dst, pstr_t const *src) {
    if (src && src->len) {
        memcpy(&dst->str[dst->len], src->str, src->len);
        dst->len += src->len;
    }
    return dst;
}

pstr_t *pcstrcat(pstr_t *dst, char const *src) {
    if (src && *src) {
        byte len = (byte)strlen(src);
        memcpy(&dst->str[dst->len], src, len);
        dst->len += len;
    }
    return dst;
}

static void ModifyOpCode() {
    byte codeVal;

    pstr_t *codeStr;

    byte codeMode  = (codeByte >> 4) & 3;
    byte codeParam = codeByte & 0xf;
    if (codeParam < 4) {
        codeVal = (byte)wValAry[codeParam];
        codeStr = sValAry[codeParam];
    } else if (codeMode == 0) {
        codeVal = regPairNo[codeParam - 4];
        codeStr = (pstr_t *)regPairStr[codeParam - 4];
    } else {
        codeVal = regNo[codeParam - 4];
        codeStr = (pstr_t *)regStr[codeParam - 4];
    }

    if (codeMode == 0)
        codeVal = (codeVal << 4) | (codeVal >> 4);
    else if (codeMode == 1)
        codeVal = (codeVal << 3) | (codeVal >> 5);
    opBytes[0] |= codeVal;
    pstrcat((pstr_t *)&line, codeStr);
}

static void AddWord(byte codeParam) {

    fixupSelector = fixupType;
    putWord(&opBytes[opByteCnt], wValAry[codeParam]);
    opByteCnt += 2;
    pstrcat((pstr_t *)&line, sValAry[codeParam]);
}

static void AddHelper(byte codeParam) {
    word helperId;

    if (codeParam == 1)
        helperId = 105; // delay
    else {
        byte grp = helperGroup[b969D];
        helperId = helperMap[grp][b9692];
    }
    line.len += (byte)sprintf(&line.str[line.len], "@P%04d", helperId);
    if (standAlone) {
        putWord(&opBytes[opByteCnt], helperAddr[helperId]);
        opByteCnt += 2;
        fixupSelector = 1;
    } else {
        putWord(&opBytes[opByteCnt], 0);
        opByteCnt += 2;
        fixupSelector = 5;
        curExtId      = (byte)helperAddr[helperId];
    }
}

static void AddSmallNum(byte codeParam) {
    byte num             = codeTable[++codePtr];
    opBytes[opByteCnt++] = num;
    /* extend to word on opBytes if not 0x84 */
    if (codeParam)
        opBytes[opByteCnt++] = 0;
    line.len += (byte)sprintf(&line.str[line.len], "%d", num);
}

static void AddStackOrigin() {
    fixupSelector = 3;
    putWord(&opBytes[opByteCnt], 0);
    opByteCnt += 2;
    pstrcat((pstr_t *)&line, (pstr_t *)stackOrigin);
}

static void AddByte(byte codeParam) {
    opBytes[opByteCnt++] = (byte)wValAry[codeParam];
    if (wValAry[codeParam] > 255) /* reformat number to byte */
        pcstrcpy(sValAry[codeParam], hexfmt(0, Low(wValAry[codeParam])));
    pstrcat((pstr_t *)&line, sValAry[codeParam]);
}

static void AddPCRel() {
    fixupSelector = 1;
    word offset   = codeTable[++codePtr];
    if (offset > 127) /* Sign() extend */
        offset = offset | 0xff00;
    putWord(&opBytes[opByteCnt], baseAddr + offset);
    opByteCnt += 2;
    line.str[line.len++] = '$';
    AddWrdDisp((pstr_t *)&line, offset);
}

static void AddCcCode() {
    opBytes[0] |= ccBits[b969C];
    pcstrcat((pstr_t *)&line, ccCodes[b969C]);
}

static void EmitHelperLabel() {
    line.len += (byte)sprintf(&line.str[line.len], "@P%04d:", helperId++);
}

static void Sub_64CF(byte codeParam) {
    byte i;
    switch (codeParam) {
    case 0:
        i = helperGroup[b969D];
        break;
    case 1:
        i = b475E[b969D];
        break;
    case 2:
        i = b4774[b969D];
        break;
    case 3:
        i = b478A[b969D];
        break;
    }
    opBytes[0] = b473D[i];
    opByteCnt  = 1;
    pcstrcat((pstr_t *)&line, opcodes[i]);
}

static void BuildInstruction(word codeIdx) {
    codePtr = codeSeq[codeIdx];
    if (codeTable[codePtr] == 0)
        opByteCnt = 0;
    else {
        opBytes[0] = codeTable[codePtr];
        opByteCnt  = 1;
    }

    fixupSelector = 0;
    line.len      = 0;

    while (1) {
        codeByte = codeTable[++codePtr];
        if (codeByte < 0x80) { // normal char
            line.str[line.len++] = codeByte;
        } else if (codeByte >= 0xc0)
            ModifyOpCode();
        else {
            byte codeParam = (codeByte >> 4) & 3;
            switch (codeByte & 0xf) {
            case 0:
                return;
            case 1:
                pstrcat((pstr_t *)&line, sValAry[codeParam]);
                break;
            case 2:
                AddWord(codeParam);
                break;
            case 3:
                AddHelper(codeParam);
                break;
            case 4:
                AddSmallNum(codeParam);
                break;
            case 5:
                AddStackOrigin();
                break;
            case 6:
                AddByte(codeParam);
                break;
            case 7:
                AddPCRel();
                break;
            case 8:
                AddCcCode();
                break;
            case 9:
                EmitHelperLabel();
                break;
            case 10:
                pstrcat((pstr_t *)&line, (pstr_t *)commentPStr);
                break;
            case 11:
                Sub_64CF(codeParam);
                break;
            }
        }
    }
}

static void WrInstructionObj() {
    word p;
    byte i;

    if (opByteCnt == 0 || !OBJECT)
        return;
    if (getWord(&recExec[REC_LEN]) + opByteCnt >= 1018)
        FlushRecs();
    p = baseAddr + opByteCnt - 2;
    switch (fixupSelector) {
    case 0:
        break;
    case 1:
        if (getWord(&recSelfFixup[REC_LEN]) + 2 >= 1018)
            FlushRecs();
        RecAddWord(recSelfFixup, 1, p);
        break;
    case 2:
        if (getWord(&recCodeFixup[REC_LEN]) + 2 >= 1017)
            FlushRecs();
        RecAddWord(recCodeFixup, 2, p);
        break;
    case 3:
        if (getWord(&recDataFixup[REC_LEN]) + 2 >= 99)
            FlushRecs();
        RecAddWord(recDataFixup, 2, p);
        break;
    case 4:
        if (getWord(&recMemoryFixup[REC_LEN]) + 2 >= 99)
            FlushRecs();
        RecAddWord(recMemoryFixup, 2, p);
        break;
    case 5:
        if (getWord(&recExtFixup[REC_LEN]) + 4 >= 1018)
            FlushRecs();
        RecAddWord(recExtFixup, 1, curExtId);
        RecAddWord(recExtFixup, 1, p);
        break;
    }
    for (i = 0; i < opByteCnt; i++) {
        RecAddByte(recExec, 3, opBytes[i]);
    }
} /* Sub_654F() */

void EmitCodeSeq(word codeIdx, byte len) {
    while (len-- > 0) {
        BuildInstruction(codeIdx++);
        WrInstructionObj();
        ListInstruction();
        if (baseAddr + opByteCnt > 0xffff) {
            errData.stmt = errData.info = 0;
            errData.num                 = 0xCE;
            EmitError();
        }
        baseAddr += opByteCnt;
    }
}

static byte operandClass;

static void Sub_66F1(void) {
    if (cfCode >= CF_174) {
        byte i = cfCode - CF_174;
        cfCode = b4602[i];
        i      = b413B[i];
        b9692  = b4128[i];
    }
}

static void RdBVal(byte slot) {
    wValAry[slot] = Rd1Byte();
    sValAry[slot] = pcstrcpy((pstr_t *)valPStr, hexfmt(0, wValAry[slot]));
}

static void RdWVal(byte slot) {
    wValAry[slot] = Rd1Word();
    sValAry[slot] = pcstrcpy((pstr_t *)valPStr, hexfmt(0, wValAry[slot]));
}

static void RdLocLab(byte slot) {
    locLabelNum   = Rd1Word();
    wValAry[slot] = localLabels[locLabelNum];
    locPStr[0]    = sprintf(locPStr + 1, "@%d", locLabelNum);
    sValAry[slot] = (pstr_t *)locPStr;
    fixupType     = 1;
}

static void Sub_6982(byte slot) {
    byte i         = Rd1Byte();
    word p         = Rd1Word();
    commentPStr[0] = sprintf(commentPStr + 1, "; %d", i);
    wValAry[slot]  = p;
    valPStr[0]     = sprintf(valPStr + 1, "%d", p);
    sValAry[slot]  = (pstr_t *)valPStr;
}

static void RdSymbol(word disp, byte slot) {
    info          = FromIdx(Rd1Word());
    wValAry[slot] = info->addr + disp;
    curSym        = info->sym;
    if (curSym)
        pstrcpy((pstr_t *)valPStr, curSym->name);
    else {
        valPStr[0] = 1;
        valPStr[1] = '$';
        disp       = wValAry[slot] - baseAddr;
    }
    sValAry[slot] = (pstr_t *)valPStr;
    AddWrdDisp(sValAry[slot], disp);
    if ((info->flag & F_EXTERNAL)) {
        fixupType = 5;
        curExtId  = info->extId;
    } else if (info->type == PROC_T || info->type == LABEL_T)
        fixupType = 1;
    else if ((info->flag & F_MEMBER) || (info->flag & F_BASED))
        ;
    else if ((info->flag & F_DATA))
        fixupType = 1;
    else if ((info->flag & F_MEMORY))
        fixupType = 4;
    else if (!(info->flag & F_ABSOLUTE))
        fixupType = 2;
}

static void Sub_6B0E(byte slot) {
    word disp      = Rd1Word();

    info           = FromIdx(Rd1Word());
    wValAry[slot]  = Rd1Word();
    sValAry[slot]  = pcstrcpy((pstr_t *)valPStr, hexfmt(0, wValAry[slot]));
    curSym         = info->sym;
    commentPStr[0] = sprintf(commentPStr + 1, "; %s", curSym->name->str);
    AddWrdDisp((pstr_t *)commentPStr, disp);
}

static void Sub_6B9B(byte reg, byte slot) {
    switch (reg - 8) {
    case 0:
        RdBVal(slot);
        break;
    case 1:
        RdWVal(slot);
        break;
    case 2:
        Sub_6982(slot);
        break;
    case 3:
        RdSymbol(Rd1Word(), slot);
        break;
    case 4:
        Sub_6B0E(slot);
        break;
    }
}

static void Sub_67AD(byte reg, byte slot) {
    switch (operandClass) {
    case 0:
        return;
    case 1: // byte reg pair
        wValAry[slot]     = regNo[reg];
        sValAry[slot]     = (pstr_t *)regStr[reg];
        wValAry[slot + 2] = regNo[reg + 4];
        sValAry[slot + 2] = (pstr_t *)regStr[reg + 4];
        break;
    case 2: // word reg pair
        wValAry[slot] = regPairNo[reg];
        sValAry[slot] = (pstr_t *)regPairStr[reg];
        break;
    case 3:
        Sub_6B9B(reg, slot);
        break;
    case 4:
        RdBVal(slot);
        break;
    case 5:
        RdWVal(slot);
        break;
    case 6:
        RdLocLab(slot);
        break;
    case 7:
        RdSymbol(0, slot);
        break;
    }
} /* Sub_67AD() */

static void Sub_6720(void) {
    fixupType = 0;
    if (b4029[cfCode] & 0x80) {
        b969C = Rd1Byte();
        b969D = b4273[b969C];
    }
    commentPStr[0] = 0; // clear any existing comment string
    operandClass   = (b4029[cfCode] >> 4) & 7;
    if (operandClass) {
        byte regs = operandClass <= 3 ? Rd1Byte() : 0;
        Sub_67AD((regs >> 4) & 0xf, 0);
        operandClass = (b4029[cfCode] >> 1) & 7;
        Sub_67AD(regs & 0xf, 1);
    }
} /* Sub_6720() */

void Sub_668B(void) {
    Sub_66F1();
    Sub_6720();
    if (cfCode == T2_PROCEDURE) {
        baseAddr = info->addr;
        if (DEBUG) {
            // replace the statement location with the proc location
            putWord(&recLineNum[REC_LEN], getWord(&recLineNum[REC_LEN]) - 4);
            RecAddWord(recLineNum, 1, baseAddr);
            putWord(&recLineNum[REC_LEN], getWord(&recLineNum[REC_LEN]) + 2);
        }
        FlushRecs();
    }
    EmitLinePrefix();
    EmitCodeSeq(instuctionSeq[cfCode] & 0xfff, instuctionSeq[cfCode] >> 12);
}
