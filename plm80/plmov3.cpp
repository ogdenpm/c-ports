/****************************************************************************
 *  plmov3.cpp: part of the C port of Intel's ISIS-II plm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C++ by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// $Id: plmov3.cpp,v 1.1 2003/10/04 21:08:48 Mark Ogden Exp $
#include "common.hpp"
#include "plm.hpp"
#include "trace.hpp"
#include <stdio.h>

Byte b3EBF = 0xA4;
Byte b3EC0 = 0x9C;

#pragma pack(push, 1)

typedef struct {
    Byte type;
    word len;
    Byte data[300];
} rec300;

typedef struct {
    Byte type;
    word len;
    Byte data[1021]; // possibly 1020
} rec1020;

struct {
    Byte type;
    word len;
    Byte data[51];
} recModHdr = { 2 };

struct {
    Byte type;
    word len;
    Byte seg;
    word addr;
    Byte data[300];
} recInitContent = { 6 };

typedef struct {
    Byte type;
    word len;
    Byte data[150];
} rec150;

#pragma pack(pop)

// static Byte tx1Buf[1280];
Byte nmsBuf[1280];
// static Byte atBuf[1280];
Byte objBuf[1280];
word curOffset;
Byte printOrObj;
rec150 recSelfFixup   = { 0x22, 0, { 3 } };    // might be rec151
rec150 recCodeFixup = { 0x24, 0, { 1, 3 } }; // might be rec151
rec150 recDataFixup = { 0x24, 0, { 2, 3 } }; // might be rec151
rec150 rec24_4 = { 0x24, 0, { 4, 3 } }; // might be rec151
rec150 recExtFixup   = { 0x20, 0, { 3 } };
static atFData_t atFData;
Byte ba8016[255];
word stringLen;
word stringIdx;
word structDim;
word itemDim;
Byte moreToInit;

Byte b4789[] = { 2, 2, 3, 4, 3, 4, 2, 2, 3, 4, 2, 3, 2, 3, 3, 3, 3, 2, 2, 3, 4, 2, 3,
                 2, 3, 2, 2, 2, 2, 3, 2, 2, 2, 3, 2, 3, 2, 2, 3, 2, 2, 1, 2, 2, 3, 4 };

Byte b47B7[] = { 0,    2,    4,    7,    0xB,  0xE,  0x12, 0x14, 0x16, 0x19, 0x1D, 0x1F, 0x22, 0x24,
                 0x27, 0x2A, 0x2D, 0x30, 0x32, 0x34, 0x37, 0x3B, 0x3D, 0x40, 0x42, 0x45, 0x47, 0x49,
                 0x4B, 0x4D, 0x50, 0x52, 0x54, 0x56, 0x59, 0x5B, 0x5E, 0x60, 0x62, 0x65, 0x67, 0x69,
                 0x6A, 0x6C, 0x6E, 0x71, 0x24, 0x24, 0x24, 0x24, 0x13, 0x13, 0x18, 0x18, 0x18, 0x18,
                 0x16, 0x2C, 0x15, 0x1F, 0x1F, 0x20, 0x20, 0x19, 0x19, 0x19, 0x19, 8,    8,    9,
                 9,    6,    7,    0x25, 0x25, 0x25, 0x25, 0x25, 0xA,  0xA,  0xB,  0xB,  0x14, 0x14,
                 0x14, 0x14, 0x14, 0x39, 0x1A, 0x1A, 0x1A, 0x1A };

Byte b4813[] = { 3, 7, 3, 7,   2, 3,   8, 1, 3,   1, 8,    2, 3, 8,   1, 3,    1, 8, 3,   7,
                 3, 7, 2, 3,   8, 1,   3, 1, 8,   2, 0x1D, 3, 1, 7,   2, 0x12, 2, 1, 0xA, 2,
                 1, 8, 2, 1,   8, 2,   1, 7, 3,   7, 3,    7, 2, 3,   8, 1,    3, 1, 8,   1,
                 6, 3, 1, 0xB, 1, 6,   3, 1, 0xB, 1, 6,    1, 6, 3,   7, 3,    7, 2, 3,   8,
                 3, 8, 3, 9,   1, 6,   3, 1, 6,   1, 7,    3, 1, 0xC, 3, 7,    3, 7, 2,   3,
                 8, 3, 8, 3,   9, 0xC, 3, 7, 3,   7, 2,    3, 8, 1,   3, 1,    8 };

void recAddWord(rec *arg1w, Byte arg2b, word arg3w);
void recAddByte(rec *arg1w, Byte arg2b, Byte arg3b);
void writeRec(rec *arg1w, Byte arg2b);
void advNextInfo();
void sub_4DA8();
void sub_4D85();
void sub_4D13();
void sub_4CF9();
void sub_4CAC();
void sub_4C7A();
void sub_4BF4();
void sub_4B6C();
void sub_4A31();
void sub_49F9();
void writeError(word arg1w, word arg2w, word arg3w);
word sub_4984();
word sub_4938();
void sub_4908(rec *arg1w, word arg2w, Byte arg3b);
void AddRecPStr(rec *recp, Byte off, Byte len, Byte *str);
void sub_4889();
void sub_4746();
void sub_46B7();
void sub_436C();
void sub_426E();
void sub_4201();
void sub_4105();
void sub_40B6();
void sub_402F();
void sub_3FE2();
void sub_3F3C();

int overlay3() {
    sub_3F3C();
    if (IXREF || printOrObj)
        sub_3FE2();
    sub_402F();
    if (IXREF || printOrObj) {
        sub_40B6();
        sub_4105();
        initDataSize = curOffset;
        sub_4201();
    }
    if (OBJECT) {
        sub_426E();
        sub_436C();
        sub_46B7();
    }
    sub_49F9();
    sub_4746();
    if (printOrObj)
        return 4;
    closeFile(&tx1File);
#ifdef _DEBUG
    copyFile(tx1File.fullName, "tx1File.tmp_ov3");
#endif
    deleteFile(&tx1File);
    if (IXREF)
        return 5;
    printExitMsg();
    return -1;
}

void sub_3F3C() {
    printOrObj = OBJECT | PRINT;
    if (OBJECTSet) {
        deleteFile(&objFile);
        OBJECTSet = 0;
    }
    if (!lstFileOpen && PRINTSet) {
        deleteFile(&lstFile);
        PRINTSet = 0;
    }
    closeFile(&tx2File);
#ifdef _DEBUG
    copyFile(tx2File.fullName, "tx2File.tmp_ov3");
#endif
    deleteFile(&tx2File);
    //	assignFileBuffer(&tx1File, tx1Buf, 1280, 2);
    //	if (IXREF || b7199)
    //		assignFileBuffer(&nmsFile, nmsBuf, 1280, 1);
    //	assignFileBuffer(&atFile, atBuf, 1280, 1);
    if (OBJECT) {
        openFile(&objFile, 3);
        //		assignFileBuffer(&objFile, objBuf, 1280, 2);
        SeekEnd(&objFile);
    }
    curOffset = initDataSize;
}

void sub_3FE2() {
    word p;
    for (p = 1; p <= procCnt; p++) {
        curInfo_p = off2Info(procInfo[p]);
        if (!testInfoFlag(F_EXTERNAL)) {
            setAddr(curOffset);
            curOffset += getDimension();
        }
    }
}

void sub_402F() {
    word p, q;
    curInfo_p = off2Info(2);
    while (curInfo_p != 0) {
        if (getInfoType() == LABEL_T) {
            if (!testInfoFlag(F_LABEL))
                writeError(172, info2Off(curInfo_p), 0);
            else if (!testInfoFlag(F_EXTERNAL)) {
                p         = getAddr();
                q         = curInfo_p;
                curInfo_p = off2Info(procInfo[getInfoScope() >> 8]);
                p += getAddr();
                curInfo_p = q;
                setAddr(p);
            }
        }
        advNextInfo();
    }
}

void sub_40B6() {
    word p;

    for (p = 1; p <= localLabelCnt; p++) {
        curInfo_p = off2Info(procInfo[((Byte *)word_381E)[p]]);
        ((word *)localLabels_p)[p] += getAddr();
    }
}

void sub_4105() {
    Byte i, j, k, m;
    word p;

    if (!byte_3C3B)
        return;

    ifwrite(&tx1File, &b3EBF, 1);
    ifwrite(&tx1File, &procInfo[1], 2);
    curInfo_p = off2Info(procInfo[1]);
    p         = curOffset - getAddr();
    ifwrite(&tx1File, &p, 2);
    for (i = 0; i <= 0x2d; i++) {
        j = (k = b47B7[i]) + b4789[i];
        m = 0;
        while (j > k) { // loc_41F9
            if (m) {
                ((word *)helpers_p)[k] = curOffset;
                curOffset += b4813[k];
            } else if (((word *)helpers_p)[k] != 0) {
                m                      = 0xFF;
                ((word *)helpers_p)[k] = curOffset;
                curOffset += b4813[k];
            }
            k++;
        }
    }
}

void sub_4201() {
    Byte i;

    curSymbol_p = (topSymbol = localLabels_p - 3) - 1;
    ifread(&nmsFile, &i, 1);
    while (i != 0) {
        curSymbol_p -= i + 1;
        curSymbol_p->name[0] = i;
        ifread(&nmsFile, &curSymbol_p->name[1], i);
        ifread(&nmsFile, &i, 1);
    }
    botSymbol = curSymbol_p + 4;
    botMem    = (word)botSymbol;
}

void sub_426E() {
    curInfo_p = off2Info(procInfo[1]);
    if ((curSymbol_p = getInfoSymbol()) == 0)
        recAddByte((rec *)&recModHdr, 0, 0);
    else
        AddRecPStr((rec *)&recModHdr, 0, curSymbol_p->name[0], &curSymbol_p->name[1]);
    recAddByte((rec *)&recModHdr, 0, 1);
    recAddByte((rec *)&recModHdr, 0, (version[1] << 4) | (version[3] & 0xf));
    recAddByte((rec *)&recModHdr, 0, 1);
    recAddWord((rec *)&recModHdr, 0, initDataSize);
    recAddByte((rec *)&recModHdr, 0, 3);
    recAddByte((rec *)&recModHdr, 0, 2);
    recAddWord((rec *)&recModHdr, 0, uninitDataSize);
    recAddByte((rec *)&recModHdr, 0, 3);
    recAddByte((rec *)&recModHdr, 0, 3);
    recAddWord((rec *)&recModHdr, 0, sub_4938());
    recAddByte((rec *)&recModHdr, 0, 3);
    recAddByte((rec *)&recModHdr, 0, 4);
    recAddWord((rec *)&recModHdr, 0, 0);
    recAddByte((rec *)&recModHdr, 0, 3);
    writeRec((rec *)&recModHdr, 0);
}

rec300 recExternals   = { 0x18 };
rec300 recPublicAbs = { 0x16 };
rec300 recPublicCode = { 0x16, 0, { 1 } };
rec300 recPublicData = { 0x16, 0, { 2 } };
rec300 recPublicMemory = { 0x16, 0, { 4 } };
rec1020 recLocals  = { 0x12, 0, { 1 } };

void sub_436C() {
    rec *p;
    word q;
    Byte i, j;
    word r, s;
    Byte k;
    Byte t[6];

    s         = 0;
    j         = 0;
    curInfo_p = off2Info(2);
    while (curInfo_p != 0) {
        curSymbol_p = getInfoSymbol();
        if (getInfoType() >= LABEL_T && getInfoType() <= PROC_T && curSymbol_p != 0) { // 45E6
            if (testInfoFlag(F_EXTERNAL) && !testInfoFlag(F_AT)) {                     // 4418
                if (recExternals.len + curSymbol_p->name[0] + 2 >= 299)
                    writeRec((rec *)&recExternals.type, 0);
                ++s;
                AddRecPStr((rec *)&recExternals.type, 0, curSymbol_p->name[0], &curSymbol_p->name[1]);
                recAddByte((rec *)&recExternals.type, 0, 0);
            } else if (!(testInfoFlag(F_AUTOMATIC) || testInfoFlag(F_BASED) ||
                         testInfoFlag(F_MEMBER))) {
                if (testInfoFlag(F_DATA) || getInfoType() == LABEL_T || getInfoType() == PROC_T) {
                    p = (rec *)&recPublicCode;
                    q = recPublicCode.len;
                    i = 1;
                } else if (testInfoFlag(F_MEMORY)) { // loc_4485
                    p = (rec *)&recPublicMemory;
                    q = recPublicMemory.len;
                    i = 4;
                } else if (testInfoFlag(F_ABSOLUTE)) { // loc_44A2
                    p = (rec *)&recPublicAbs;
                    q = recPublicAbs.len;
                    i = 0;
                } else {
                    p = (rec *)&recPublicData;
                    q = recPublicData.len;
                    i = 2;
                }

                if (testInfoFlag(F_PUBLIC)) { // loc_4514
                    if (q + curSymbol_p->name[0] + 4 >= 299)
                        writeRec(p, 1);
                    recAddWord(p, 1, getAddr());
                    AddRecPStr(p, 1, curSymbol_p->name[0], &curSymbol_p->name[1]);
                    recAddByte(p, 1, 0);
                }
                if (DEBUG) { // loc_45E6
                    if (curInfo_p == off2Info(procInfo[1]))
                        j = 0xFF;
                    else if (!testInfoFlag(F_PARAMETER))
                        j = testInfoFlag(F_EXTERNAL);
                    else {
                        r         = curInfo_p;
                        curInfo_p = off2Info(procInfo[getInfoScope() >> 8]);
                        j         = testInfoFlag(F_EXTERNAL);
                        curInfo_p = r;
                    }
                    if (!j) { // loc_45E6
                        if (recLocals.data[0] != i || curSymbol_p->name[0] + recLocals.len + 4 >= 1019)
                            writeRec((rec *)&recLocals, 1);
                        recLocals.data[0] = i;
                        recAddWord((rec *)&recLocals, 1, getAddr());
                        AddRecPStr((rec *)&recLocals, 1, curSymbol_p->name[0], &curSymbol_p->name[1]);
                        recAddByte((rec *)&recLocals, 1, 0);
                    }
                }
            }
        }
        advNextInfo();
    } // of while

    t[0] = '@';
    t[1] = 'P';
    if (!byte_3C3B) {
        for (k = 0; k <= 0x74; k++) {
            if (((word *)helpers_p)[k] != 0) {
                ((word *)helpers_p)[k] = s;
                ++s;
                if (recExternals.len + 8 >= 299)
                    writeRec((rec *)&recExternals, 0);
                num2Asc(k, 0xfc, 10, (char *)&t[2]);
                AddRecPStr((rec *)&recExternals, 0, 6, t);
                recAddByte((rec *)&recExternals, 0, 0);
            }
        }
    }
    writeRec((rec *)&recExternals, 0);
    writeRec((rec *)&recPublicAbs, 1);
    writeRec((rec *)&recPublicCode, 1);
    writeRec((rec *)&recPublicData, 1);
    writeRec((rec *)&recPublicMemory, 1);
    writeRec((rec *)&recLocals, 1);
}

void sub_46B7() {
    word p;

    if (intVecNum == 0)
        return;
    for (p = 1; p <= procCnt; p++) {
        curInfo_p = off2Info(procInfo[p]);
        if (testInfoFlag(F_INTERRUPT)) {
            recInitContent.addr = intVecLoc + intVecNum * getIntrNo();
            recAddByte((rec *)&recInitContent, 3, 0xC3);
            recAddWord((rec *)&recInitContent, 3, getAddr());
            writeRec((rec *)&recInitContent, 3);
            recAddWord((rec *)&recCodeFixup, 2, recInitContent.addr + 1);
            writeRec((rec *)&recCodeFixup, 2);
        }
    }
}

void sub_4746() {
    if (IXREF || printOrObj) {
        ifwrite(&tx1File, &b3EC0, 1);
        //		flushFile(&tx1File);
        rewindFile(&tx1File);
        closeFile(&nmsFile);
#ifdef _DEBUG
        copyFile(nmsFile.fullName, "nmsFile.tmp_ov3");
#endif
        deleteFile(&nmsFile);
        //		flushFile(&objFile);
    }
    closeFile(&atFile);
#ifdef _DEBUG
    copyFile(atFile.fullName, "atFile.tmp_ov3");
#endif
    deleteFile(&atFile);
}

Byte b4888 = 0xA3;

void sub_4889() {
    writeRec((rec *)&recInitContent, 3);
    writeRec((rec *)&recSelfFixup, 1);
    writeRec((rec *)&recCodeFixup, 2);
    writeRec((rec *)&recDataFixup, 2);
    writeRec((rec *)&rec24_4, 2);
    writeRec((rec *)&recExtFixup, 1);
}

void AddRecPStr(rec *recp, Byte off, Byte len, Byte *str) {
    Byte i;
    recAddByte(recp, off, len);
    for (i = 0; len != i; i++) {
        recAddByte(recp, off, str[i]);
    }
}

void sub_4908(rec *arg1w, word arg2w, Byte arg3b) {
    if (arg1w->len + arg3b >= arg2w) {
        sub_4889();
        recInitContent.addr = curOffset;
    }
}

word sub_4938() {
    word p, q;
    q = 0;
    for (p = 1; p <= procCnt; p++) {
        curInfo_p = off2Info(procInfo[p]);
        if (q < getBasedOffset())
            q = getBasedOffset();
    }
    return q;
}

word sub_4984() {
    switch (getInfoType()) {
    case BYTE_T:
        return 1;
    case ADDRESS_T:
        return 2;
    case STRUCT_T:
        return getOwningStructure();
    }
    return 0;
}

void writeError(word arg1w, word arg2w, word arg3w) {
    word tmp[3];
    tmp[0] = arg1w;
    tmp[1] = arg2w;
    tmp[2] = arg3w;

    if (printOrObj) { // loc_49F1
        ifwrite(&tx1File, &b4888, 1);
        ifwrite(&tx1File, tmp, 6);
    } else
        ++programErrCnt;
}

void sub_49F9() {
    while (1) {
        ifread(&atFile, &atFData.type, 1);
        if (atFData.type == AT_AHDR)
            ifread(&atFile, &atFData.info_p, 12);
        else if (atFData.type == AT_DHDR)
            sub_4A31();
        else
            return;
    }
}

void sub_4A31() {
    word w811E;
    ifread(&atFile, &atFData.info_p, 4);
    curInfo_p = off2Info(w811E = atFData.info_p);
    stringIdx = stringLen = itemDim = structDim = 0;
    if (testInfoFlag(F_DATA))
        recInitContent.seg = 1;
    else if (testInfoFlag(F_MEMORY))
        recInitContent.seg = 4;
    else if (testInfoFlag(F_ABSOLUTE))
        recInitContent.seg = 0;
    else
        recInitContent.seg = 2;

    recInitContent.addr = getAddr();
    curOffset     = recInitContent.addr;
    if (off2Info(0) == curInfo_p)
        moreToInit = 0;
    else if (testInfoFlag(F_EXTERNAL)) {
        writeError(217, w811E, atFData.stmtNum);
        moreToInit = 0;
    } else {
        sub_4B6C();
        moreToInit = 0xff;
    }

    sub_4BF4();
    if (atFData.type == AT_END) {
        sub_4889();
        return;
    }

    while (1) {
        sub_4C7A();
        sub_4BF4();
        if (atFData.type == AT_END) {
            sub_4889();
            return;
        } else if (moreToInit) {
            if (itemDim > 1)
                --itemDim;
            else {
                advNextInfo();
                while (curInfo_p != 0) {
                    if (getInfoType() >= BYTE_T && getInfoType() <= STRUCT_T)
                        break;
                    advNextInfo();
                }
                sub_4B6C();
                if (!moreToInit)
                    writeError(0xd1, w811E, atFData.stmtNum);
            }
        }
    }
}

void sub_4B6C() {
    if (curInfo_p == 0 || !testInfoFlag(F_MEMBER)) {
        if (structDim > 1) {
            --structDim;
            curInfo_p = atFData.info_p;
        } else if (curInfo_p == 0) {
            moreToInit = 0;
            return;
        } else {
            if (!testInfoFlag(F_PACKED))
                moreToInit = 0;
            if (getInfoType() == STRUCT_T) {
                if (testInfoFlag(F_ARRAY))
                    structDim = getDimension();
                advNextInfo();
                atFData.info_p = curInfo_p;
            }
        }
    }
    if (testInfoFlag(F_ARRAY))
        itemDim = getDimension();
}

void sub_4BF4() {
    if (!moreToInit || stringIdx >= stringLen) {
        ifread(&atFile, &atFData.type, 1);
        switch (atFData.type) {
        case AT_2:
            ifread(&atFile, &atFData.val, 2);
            break;
        case AT_STRING:
            ifread(&atFile, &stringLen, 2);
            ifread(&atFile, ba8016, stringLen);
            stringIdx = 0;
            break;
        case AT_DATA:
            ifread(&atFile, &atFData.varInfoOffset, 8);
            break;
        case AT_END:
            break;
        }
    }
}

void sub_4C7A() {
    if (moreToInit) {
        switch (atFData.type) {
        case AT_2:
            sub_4D85();
            break;
        case AT_STRING:
            sub_4D13();
            break;
        case AT_DATA:
            sub_4DA8();
            break;
        }
    }
}

void sub_4CAC() {
    if (getInfoType() == BYTE_T) {
        sub_4908((rec *)&recInitContent, 0x12c, 1);
        recAddByte((rec *)&recInitContent, 3, (Byte)atFData.val);
        ++curOffset;
    } else {
        sub_4908((rec *)&recInitContent, 0x12C, 2);
        recAddWord((rec *)&recInitContent, 3, atFData.val);
        curOffset += 2;
    }
}

void sub_4CF9() {
    writeError(210, info2Off(curInfo_p), atFData.stmtNum);
    sub_4CAC();
}

void sub_4D13() {
    Byte *w8120;
    if (getInfoType() == BYTE_T) {
        atFData.val = ba8016[stringIdx];
        ++stringIdx;
    } else {
        w8120    = (Byte *)&atFData.val;
        w8120[1] = ba8016[stringIdx];
        ++stringIdx;
        if (stringIdx < stringLen) {
            w8120[0] = ba8016[stringIdx];
            ++stringIdx;
        } else {
            w8120[0] = w8120[1];
            w8120[1] = 0;
        }
    }
    sub_4CAC();
}

void sub_4D85() {
    if (atFData.val > 255 && getInfoType() == BYTE_T)
        sub_4CF9();
    else
        sub_4CAC();
}

void sub_4DA8() {
    Byte i, j;
    word p;
    rec *q;

    if (atFData.varInfoOffset == 0)
        sub_4D85();
    else if (getInfoType() == BYTE_T)
        sub_4CF9();
    else {
        p         = curInfo_p;
        curInfo_p = off2Info(atFData.varInfoOffset);
        if (testInfoFlag(F_MEMBER)) { // loc_4E01
            atFData.val += getAddr() + sub_4984() * atFData.varNestedArrayIndex;
            curInfo_p = getOwningStructure();
        }

        atFData.val += getAddr() + sub_4984() * atFData.varArrayIndex;
        if (testInfoFlag(F_EXTERNAL)) { // loc_4E5F
            i         = getExternId();
            curInfo_p = p;
            sub_4908((rec *)&recExtFixup, 0x95, 4);
            sub_4CAC();
            recAddWord((rec *)&recExtFixup, 1, i);
            recAddWord((rec *)&recExtFixup, 1, curOffset - 2);
        } else if (testInfoFlag(F_ABSOLUTE)) { // loc_4E74
            curInfo_p = p;
            sub_4CAC();
        } else {
            if (getInfoType() == PROC_T || getInfoType() == LABEL_T || testInfoFlag(F_DATA)) {
                q = (rec *)&recCodeFixup;
                i = 1;
            } else if (testInfoFlag(F_MEMORY)) {
                q = (rec *)&rec24_4;
                i = 4;
            } else {
                q = (rec *)&recDataFixup;
                i = 2;
            }

            if (recInitContent.seg == i) {
                q = (rec *)&recSelfFixup;
                j = 1;
            } else
                j = 2;

            curInfo_p = p;
            sub_4908(q, 0x95, 2);
            sub_4CAC();
            recAddWord(q, j, curOffset - 2);
        }
    }
}

void writeRec(rec *recp, Byte adjust) {
    Byte *buf;
    Byte crc;
    word p;
    word len;

    buf = (Byte *)recp;
    if (recp->len > 0 && OBJECT) {
        recp->len += adjust + 1;
        len = recp->len + 2;
        crc = 0;
        for (p = 0; p < len; p++)
            crc -= buf[p];
        buf[len] = crc; // put in checksum
        ifwrite(&objFile, buf, len + 1);
    }
    recp->len = 0;
}

void recAddByte(rec *recp, Byte off, Byte val) {
    recp->data[recp->len++ + off] = val;
}

void recAddWord(rec *recp, Byte off, word val) {
    recAddByte(recp, off, (Byte)val);
    recAddByte(recp, off, (Byte)(val >> 8));
}