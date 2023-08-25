/****************************************************************************
 *  main3.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
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

static void Sub_3FE2() {
    word p;
    for (p = 1; p <= procCnt; p++) {
        SetInfo(procInfo[p]);
        if (!(info->flag & F_EXTERNAL)) {
            info->linkVal = curOffset;
            curOffset += info->dim;
        }
    }
}

static void Sub_402F() {
    infoIdx = 0;
    AdvNxtInfo();
    while (infoIdx) {
        if (info->type == LABEL_T) {
            if (!(info->flag & F_LABEL))
                p3Error(ERR172, info, 0); // invalid label: undefined
            else if (!(info->flag & F_EXTERNAL))
                info->linkVal += infotab[procInfo[High(info->scope)]].linkVal;
        }
        AdvNxtInfo();
    }
}

static void Sub_40B6() {
    word p;

    for (p = 1; p <= localLabelCnt; p++) {
        SetInfo(procInfo[procIds[p]]);
        localLabels[p] += info->linkVal;
    }
}

static void Sub_4105() {
    byte i, j, k;
    bool m;
    word p;

    if (!standAlone)
        return;
    Wr1Byte(T2_SETADDR);    // back to top level
    Wr1Word(procInfo[1]);

    SetInfo(procInfo[1]);
    p = curOffset - info->linkVal;
    Wr1Word(p);
    for (i = 0; i <= 45; i++) {
        k = b42D6[i];
        j = k + b42A8[i];
        m = false;
        while (k < j) {
            if (m) {
                helpers[k] = curOffset;
                curOffset += b4813[k];
            } else if (helpers[k] != 0) {
                m          = true;
                helpers[k] = curOffset;
                curOffset += b4813[k];
            }
            k++;
        }
    }
}

static void emitModHdr() {
    SetInfo(procInfo[1]);
    curSym = info->sym;
    if (curSym == 0)
        RecAddByte(recModHdr, 0, 0);
    else
        RecAddName(recModHdr, 0, symtab[curSym].name->len, symtab[curSym].name->str);
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
    byte seg, k;

    s       = 0;
    infoIdx = 0;
    AdvNxtInfo();
    while (infoIdx) {
        curSym = info->sym;
        if (LABEL_T <= info->type && info->type <= PROC_T && curSym != 0) {
            if ((info->flag & (F_EXTERNAL | F_AT)) == F_EXTERNAL) {
                if (getWord(&recExternals[REC_LEN]) + symtab[curSym].name->len + 2 >= 299)
                    WriteRec(recExternals, 0);
                s++;
                RecAddName(recExternals, 0, symtab[curSym].name->len, symtab[curSym].name->str);
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
                    if (curRecLen + symtab[curSym].name->len + 4 >= 299)
                        WriteRec(rec, 1);
                    RecAddWord(rec, 1, info->linkVal);
                    RecAddName(rec, 1, symtab[curSym].name->len, symtab[curSym].name->str);
                    RecAddByte(rec, 1, 0);
                }
                bool isExtern;
                if (DEBUG) {
                    if (infoIdx == procInfo[1])
                        isExtern = true;
                    else if (!(info->flag & F_PARAMETER))
                        isExtern = (info->flag & F_EXTERNAL);
                    else
                        isExtern = infotab[procInfo[High(info->scope)]].flag & F_EXTERNAL;
                    if (!isExtern) {
                        if (seg != recLocals[REC_DATA] ||
                            getWord(&recLocals[REC_LEN]) + symtab[curSym].name->len + 4 >= 1019)
                            WriteRec(recLocals, 1);
                        recLocals[REC_DATA] = seg;
                        RecAddWord(recLocals, 1, info->linkVal);
                        RecAddName(recLocals, 1, symtab[curSym].name->len,
                                   symtab[curSym].name->str);
                        RecAddByte(recLocals, 1, 0);
                    }
                }
            }
        }
        AdvNxtInfo();
    } /* of while */

    if (!standAlone) {
        for (k = 0; k < 117; k++) {
            if (helpers[k]) {
                helpers[k] = s++;
                if (getWord(&recExternals[REC_LEN]) + 8 >= 299)
                    WriteRec(recExternals, 0);
                char t[7];
                sprintf(t, "@P%04d", k);
                RecAddName(recExternals, 0, 6, t);
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
    word p;

    if (intVecNum == 0)
        return;
    for (p = 1; p <= procCnt; p++) {
        SetInfo(procInfo[p]);
        if ((info->flag & F_INTERRUPT)) {
            putWord(&recInitContent[CONTENT_OFF], intVecLoc + intVecNum * info->intno);
            RecAddByte(recInitContent, 3, 0xC3);
            RecAddWord(recInitContent, 3, info->linkVal);
            WriteRec(recInitContent, 3);
            RecAddWord(recCodeFixup, 2, getWord(&recInitContent[CONTENT_OFF]) + 1);
            WriteRec(recCodeFixup, 2);
        }
    }
}

static void Sub_4746() {
    if (printOrObj || IXREF) {
        Wr1Byte(0x9c);
        vfRewind(&utf1);
    }
    vfReset(&atf);
}

word Start3() {
    dump(&atf, "atf_main3");
    vfRewind(&atf);

    Sub_3F3C();
    if (printOrObj || IXREF)
        Sub_3FE2();
    Sub_402F();
    if (printOrObj || IXREF) {
        Sub_40B6();
        Sub_4105();
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
        vfReset(&utf1);
        if (IXREF)
            return 5; // Chain(overlay[5]);
        else {
            EndCompile();
            Exit(programErrCnt != 0);
        }
    }
}
