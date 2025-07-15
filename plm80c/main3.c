/****************************************************************************
 *  main3.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";
word putWord(pointer buf, word val) {
    buf[0] = val & 0xff;
    buf[1] = val >> 8;
    return val;
}

word getWord(pointer buf) {
    return buf[0] + buf[1] * 256;
}

static void Sub_3F3C() {
    printOrObj = PRINT | OBJECT;

    vfReset(&utf2);
    //   vfReset(&utf1);
    vfRewind(&atf);
    if (OBJECT)
        fseek(objFile.fp, 0, SEEK_END);
    curOffset = csegSize;
}

static void AllocateCodeAddr() {
    for (int i = 1; i <= procCnt; i++) {
        info_t *pInfo = procInfo[i];
        if (!(pInfo->flag & F_EXTERNAL)) {
            pInfo->addr = curOffset;
            curOffset += pInfo->codeSize;
        }
    }
}

static void AdjustLabels() {
    info = infotab;
    AdvNxtInfo();
    while (info) {
        if (info->type == LABEL_T) {
            if (!(info->flag & F_LABEL))
                p3Error(ERR172, info, 0); // invalid label: undefined
            else if (!(info->flag & F_EXTERNAL))
                info->addr += procInfo[High(info->scope)]->addr;
        }
        AdvNxtInfo();
    }
}

static void AdjustLocalLabels() {
    for (int i = 1; i <= localLabelCnt; i++)
        localLabels[i] += procInfo[procIds[i]]->addr;
}

static void AppendHelpers() {
    if (!standAlone)
        return;
    Wr1Byte(T2_SETADDR);    // back to top level
    Wr1Info(procInfo[1]);

    Wr1Word(curOffset - procInfo[1]->addr);
    for (byte i = 0; i <= 45; i++) {        // emit any needed helper modules
        byte helperId = modHelperId[i];
        byte endHelperId = helperId + modHelperIdCnt[i];
        bool required = false;
        while (helperId < endHelperId) {
            if (helperAddr[helperId])   // check if used i.e. needed
                required = true;
            if (required) {     // have started module so need to emit rest
                helperAddr[helperId] = curOffset;
                curOffset += helperCodeLen[helperId];
            }
            helperId++;
        }
    }
}

static void emitModHdr() {
    curSym = procInfo[1]->sym;
    if (curSym == 0)
        RecAddByte(recModHdr, 0, 0);
    else
        RecAddName(recModHdr, 0, curSym->name);
    RecAddByte(recModHdr, 0, 1);                                      // translator is PL/M
    RecAddByte(recModHdr, 0, (version[1] << 4) | (version[3] & 0xf)); // the encoded version
    RecAddByte(recModHdr, 0, S_CODE);
    RecAddWord(recModHdr, 0, csegSize);
    RecAddByte(recModHdr, 0, 3); // all segments are byte relocatable
    RecAddByte(recModHdr, 0, S_DATA);
    RecAddWord(recModHdr, 0, dsegSize);
    RecAddByte(recModHdr, 0, 3);
    RecAddByte(recModHdr, 0, S_STACK);
    RecAddWord(recModHdr, 0, CalcMaxStack());
    RecAddByte(recModHdr, 0, 3);
    RecAddByte(recModHdr, 0, S_MEMORY);
    RecAddWord(recModHdr, 0, 0);
    RecAddByte(recModHdr, 0, 3);
    WriteRec(recModHdr, 0);
}

static void Sub_436C() {
    pointer rec;
    word curRecLen, s;
    byte seg;

    s       = 0;
    info = infotab;
    AdvNxtInfo();
    while (info) {
        curSym = info->sym;
        if (LABEL_T <= info->type && info->type <= PROC_T && curSym) {
            if ((info->flag & (F_EXTERNAL | F_AT)) == F_EXTERNAL) {
                if (getWord(&recExternals[REC_LEN]) + curSym->name->len + 2 >= 299)
                    WriteRec(recExternals, 0);
                s++;
                RecAddName(recExternals, 0, curSym->name);
                RecAddByte(recExternals, 0, 0);
            } else if (!(info->flag & (F_AUTOMATIC | F_BASED | F_MEMBER))) {
                if ((info->flag & F_DATA) || info->type == LABEL_T || info->type == PROC_T) {
                    rec       = recPublicCode;
                    curRecLen = getWord(&recPublicCode[REC_LEN]);
                    seg       = 1;
                } else if ((info->flag & F_MEMORY)) {
                    rec       = recPublicMemory;
                    curRecLen = getWord(&recPublicMemory[REC_LEN]);
                    seg       = 4;
                } else if ((info->flag & F_ABSOLUTE)) {
                    rec       = recPublicAbs;
                    curRecLen = getWord(&recPublicAbs[REC_LEN]);
                    seg       = 0;
                } else {
                    rec       = recPublicData;
                    curRecLen = getWord(&recPublicData[REC_LEN]);
                    seg       = 2;
                }

                if ((info->flag & F_PUBLIC)) {
                    if (curRecLen + curSym->name->len + 4 >= 299)
                        WriteRec(rec, 1);
                    RecAddWord(rec, 1, info->addr);
                    RecAddName(rec, 1, curSym->name);
                    RecAddByte(rec, 1, 0);
                }
                bool isExtern;
                if (DEBUG) {
                    if (info == procInfo[1])
                        isExtern = true;
                    else if (!(info->flag & F_PARAMETER))
                        isExtern = (info->flag & F_EXTERNAL);
                    else
                        isExtern = procInfo[High(info->scope)]->flag & F_EXTERNAL;
                    if (!isExtern) {
                        if (seg != recLocals[REC_DATA] ||
                            getWord(&recLocals[REC_LEN]) + curSym->name->len + 4 >= 1019)
                            WriteRec(recLocals, 1);
                        recLocals[REC_DATA] = seg;
                        RecAddWord(recLocals, 1, info->addr);
                        RecAddName(recLocals, 1, curSym->name);
                        RecAddByte(recLocals, 1, 0);
                    }
                }
            }
        }
        AdvNxtInfo();
    } /* of while */

    if (!standAlone) {
        for (byte helperId = 0; helperId < 117; helperId++) {
            if (helperAddr[helperId]) {
                helperAddr[helperId] = s++;
                if (getWord(&recExternals[REC_LEN]) + 8 >= 299)
                    WriteRec(recExternals, 0);
                char t[8];
                t[0] = sprintf(t + 1, "@P%04d", helperId);
                RecAddName(recExternals, 0,(pstr_t *)t);
                RecAddByte(recExternals, 0, 0);
            }
        }
    }
    WriteRec(recExternals, 0);
    WriteRec(recPublicAbs, 1);
    WriteRec(recPublicCode, 1);
    WriteRec(recPublicData, 1);
    WriteRec(recPublicMemory, 1);
    WriteRec(recLocals, 1);
}

static void Sub_46B7() {
    if (intVecNum == 0)
        return;
    for (int i = 1; i <= procCnt; i++) {
        info_t *pInfo = procInfo[i];
        if ((pInfo->flag & F_INTERRUPT)) {
            putWord(&recInitContent[CONTENT_OFF], intVecLoc + intVecNum * pInfo->intno);
            RecAddByte(recInitContent, 3, 0xC3);    // jmp instruction
            RecAddWord(recInitContent, 3, pInfo->addr);
            WriteRec(recInitContent, 3);
            RecAddWord(recCodeFixup, 2, getWord(&recInitContent[CONTENT_OFF]) + 1);
            WriteRec(recCodeFixup, 2);
        }
    }
}

static void Sub_4746() {
    if (printOrObj || IXREF) {
        Wr1Byte(T2_EOF);
        vfRewind(&utf1);
    }
}

word Start3() {
 //   dump(&atf, "atf_main3"); // diagnostic dump
    vfRewind(&atf);

    Sub_3F3C();
    if (printOrObj || IXREF)
        AllocateCodeAddr();
    AdjustLabels();
    if (printOrObj || IXREF) {
        AdjustLocalLabels();
        AppendHelpers();
        csegSize = curOffset;
    }
    if (OBJECT) {
        emitModHdr();
        Sub_436C();
        Sub_46B7();
    }
    Sub_49F9();
    Sub_4746();
    if (printOrObj)
        return 4; // Chain(overlay[4]);
    else {
        if (IXREF)
            return 5; // Chain(overlay[5]);
        else {
            EndCompile();
            Exit(programErrCnt != 0);
        }
    }
}
