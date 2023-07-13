/****************************************************************************
 *  plm1f.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

static byte externalsCnt = 0;
static word atStmtNum    = 0;
static byte b9B40;

static void Sub_6EF6(word arg) {
    hasErrors = true;
    Wr2Byte(T2_ERROR);
    Wr2Word(arg);
    Wr2Word(curInfoP - botInfo);
    Wr2Word(atStmtNum);
}




int32_t RdAtByte() {
    return vfRbyte(&atf);
}

int32_t RdAtWord() {
    return vfRword(&atf);
}


static word GetElementSize() {
    byte i;
    i = GetType() - 2;

    switch (i) {
    case 0:
        return 1; /* byte */
    case 1:
        return 2; /* address */
    case 2:
        return GetParentOffset(); /* structure. Returns() val not offeset for struct */
    }
    return 0;
}

static word GetVarSize() {
    if (TestInfoFlag(F_ARRAY))
        return GetDimension() * GetElementSize();
    else
        return GetElementSize();
}

static void RevMemMov(pointer srcp, pointer dstp, word cnt) {

    srcp = srcp + cnt - 2;
    dstp = dstp + cnt - 2;
    while (cnt > 1) {
        *(wpointer)dstp = *(wpointer)srcp;
        cnt             = cnt - 2;
        dstp            = dstp - 2;
        srcp            = srcp - 2;
    }
    if (cnt == 1)
        *(wpointer)dstp = (*(wpointer)dstp & 0xff) | (*(wpointer)srcp & 0xff00);
}

static void AdvNextDataInfo() {
    while (1) {
        AdvNxtInfo();
        if (curInfoP == 0)
            return;
        if (GetType() >= BYTE_T && GetType() <= STRUCT_T)
            return;
    }
}

static void DumpSymbols() {
    offset_t p;
    byte i;


    if (!b9B40 && !IXREF)
        return;
    p = topSymbol - 1;
    for (i = 0; i <= 63; i++) {
        curSymbolP = WordP(hashChainsP)[i];
        while (curSymbolP != 0) {
            if ((curInfoP = SymbolP(curSymbolP)->infoP) != 0 && High(curInfoP) != 0xff) {
                p = p - SymbolP(curSymbolP)->name.len - 1;
                while (curInfoP != 0) {
                    SetSymbol(p);
                    curInfoP = GetLinkOffset();
                }
                vfWbuf(&nmsf, (uint8_t *)&SymbolP(curSymbolP)->name, SymbolP(curSymbolP)->name.len + 1);
            }
            curSymbolP = SymbolP(curSymbolP)->link;
        }
    }
    vfWbyte(&nmsf, 0);
}

static void Sub_711F() {
    word p, q, r, s;

    curInfoP = botInfo + 2;

    AdvNextDataInfo();
    while (curInfoP != 0) {
        if (GetType() == STRUCT_T) {
            SetParentOffset(0); /* struct Size() is 0 */
        } else if (TestInfoFlag(F_MEMBER)) {
            r        = GetVarSize();
            p        = curInfoP;
            curInfoP = GetParentOffset(); /* get structure info */
            q        = GetParentOffset(); /* this gets Size() so far */
            if ((s = q + r) < r)          /* add in the new element */
                Sub_6EF6(ERR208);         /* LIMIT EXCEEDED: structure Size() */
            SetParentOffset(s);           /* store the running Size() */
            curInfoP = p;
            SetLinkVal(q); /* use link value for offset of this memeber */
        }
        AdvNextDataInfo();
    }
}

static void AllocVarAddress(wpointer arg1wP) {

    SetLinkVal(*arg1wP);     /* allocate this var's address */
    *arg1wP += GetVarSize(); /* reserve it's space */
    if (*arg1wP < GetVarSize())
        Sub_6EF6(ERR207); /* LIMIT EXCEEDED: SEGMENT Size() */
}

static void Sub_7323() {
    word p;
    wpointer q;

    p        = curInfoP;
    curInfoP = procInfo[High(GetScope())];
    q        = &InfoP(curInfoP)->parent; /* varsize union with parent */
    curInfoP = p;
    AllocVarAddress(q);
}

static void Sub_719D() {
    byte i;
    word p;

    /* allocate external data ids */
    curInfoP   = botInfo + 2;
    standAlone = haveModuleLevelUnit;

    while (curInfoP != 0) {
        if ((GetType() >= BYTE_T && GetType() <= STRUCT_T) || GetType() == PROC_T ||
            GetType() == LABEL_T) {
            if (TestInfoFlag(F_EXTERNAL)) {
                SetExternId(externalsCnt);
                externalsCnt = externalsCnt + 1;
                if (externalsCnt == 0)
                    Sub_6EF6(ERR219); /* LIMIT EXCEEDED: NUMBER OF EXTERNALS > 255 */
                SetLinkVal(0);
                standAlone = false;
            } else if (TestInfoFlag(F_PUBLIC))
                standAlone = false;
        }
        AdvNxtInfo();
    }

    /* allocate data variables */
    curInfoP = botInfo + 2; /* start at bottom */
    AdvNextDataInfo();      /* find next data */

    while (curInfoP != 0) {
        if (!(TestInfoFlag(F_MEMBER) || TestInfoFlag(F_AT) || TestInfoFlag(F_EXTERNAL))) {
            if (TestInfoFlag(F_PARAMETER)) {
                p        = curInfoP;
                curInfoP = procInfo[High(GetScope())];
                i        = TestInfoFlag(F_EXTERNAL); /* only allocate parameter if (! external */
                curInfoP = p;
            } else
                i = false;
            if (i)
                SetLinkVal(0); /* external parameter has 0 offset */
            else if (TestInfoFlag(F_BASED))
                SetLinkVal(0); /* based var has 0 offset */
            else if (TestInfoFlag(F_DATA))
                AllocVarAddress(&csegSize); /* allocate initialised data var */
            else if (TestInfoFlag(F_MEMORY))
                ;                               /* memory is predefined */
            else if (TestInfoFlag(F_AUTOMATIC)) /* allocate stack var */
                Sub_7323();
            else
                AllocVarAddress(&dsegSize); /* allocate uninitalised data */
        }
        AdvNextDataInfo(); /* get next data */
    }
}

union {
    byte str[256];
    struct {
        byte type;
        offset_t infoP;
        word stmt;
        offset_t infoOffset;
        word arrayIndex;
        word nestedArrayIndex;
        word val;
    };

} atFData;

word atOffset;
void RdAtHdr() {
    atFData.infoP = RdAtWord();
    atFData.stmt  = RdAtWord();
}
void RdAtData() {
    atFData.infoOffset       = RdAtWord();
    atFData.arrayIndex       = RdAtWord();
    atFData.nestedArrayIndex = RdAtWord();
    atFData.val              = RdAtWord();
}
static void Sub_7486() {
    byte i, j;
    if (atFData.infoOffset == 0)
        i = 0;
    else if (GetType() > STRUCT_T || GetType() < BYTE_T) {
        i = 0;
        Sub_6EF6(ERR211); /* INVALID IDENTIFIER IN 'at' RESTRICTED REFERENCE */
    } else if (TestInfoFlag(F_EXTERNAL)) {
        i = 1;
        j = GetExternId();
    } else if (TestInfoFlag(F_DATA))
        i = 2;
    else if (TestInfoFlag(F_AUTOMATIC))
        i = 3;
    else if (TestInfoFlag(F_MEMORY))
        i = 4;
    else if (TestInfoFlag(F_BASED)) {
        i = 0;
        Sub_6EF6(ERR212); /* INVALID RESTRICTED REFERENCE IN 'at' , BASE ILLEGAL */
    } else if (TestInfoFlag(F_ABSOLUTE))
        i = 0;
    else
        i = 5;
    curInfoP = atFData.infoP;

    while (1) {
        if (!TestInfoFlag(F_MEMBER)) {
            if (TestInfoFlag(F_DATA)) {
                ClrFlag(InfoP(curInfoP)->flag, F_DATA);
                SetInfoFlag(F_INITIAL);
            }
            SetLinkVal(atOffset);
            atOffset = atOffset + GetVarSize();
            switch (i) {
            case 0:
                SetInfoFlag(F_ABSOLUTE);
                break;
            case 1:
                if (TestInfoFlag(F_PUBLIC))
                    Sub_6EF6(ERR178); /* INVALID 'at' RESTRICTED REFERENCE, external
                                ATTRIBUTE CONFLICTS WITH public ATTRIBUTE */
                SetInfoFlag(F_EXTERNAL);
                SetExternId(j);
                break;
            case 2:
                SetInfoFlag(F_DATA);
                break;
            case 3:
                SetInfoFlag(F_AUTOMATIC);
                break;
            case 4:
                SetInfoFlag(F_MEMORY);
                break;
            case 5:
                break;
            }
        }
        AdvNextDataInfo();
        if (curInfoP == 0)
            return;
        if (!(TestInfoFlag(F_PACKED) || TestInfoFlag(F_MEMBER)))
            return;
    }
}
static void Sub_73DC() {
    RdAtHdr();
    RdAtData();
    if (atFData.infoP == 0)
        return;
    atFData.infoP = atFData.infoP + botInfo;
    atStmtNum     = atFData.stmt;
    atOffset      = atFData.val;
    if (atFData.infoOffset != 0) {
        curInfoP = atFData.infoOffset + botInfo;
        if (TestInfoFlag(F_MEMBER)) {
            atOffset = GetElementSize() * atFData.nestedArrayIndex + atOffset + GetLinkVal();
            curInfoP = GetParentOffset();
        }
        atOffset = GetLinkVal() + GetElementSize() * atFData.arrayIndex + atOffset;
        if (TestInfoFlag(F_AT))
            if (curInfoP >= atFData.infoP)
                Sub_6EF6(ERR213); /* UNDEFINED RESTRICTED REFERENCE IN 'at' */
    }
    Sub_7486();
}

static void ProcAtFile() {

    while (1) {
        atFData.type = RdAtByte();
        switch (atFData.type) {
        case 0:
            Sub_73DC();
            break; /* AT_AHDR */
        case 1:
            atFData.infoP = RdAtWord();
            atFData.stmt  = RdAtWord();
            break; /* AT_DHDR */
        case 2:
            atFData.val = RdAtWord();
            break; /* AT_2 */
        case 3:    /* AT_STRING */
            atFData.val = RdAtWord();
            vfRbuf(&atf, atFData.str, atFData.val); // effectively junk it.
            break;
        case 4:
            atFData.infoOffset       = RdAtWord();
            atFData.arrayIndex       = RdAtWord();
            atFData.nestedArrayIndex = RdAtWord();
            atFData.val              = RdAtWord();
            break; /* AT_DATA */
        case 5:
            break; /* AT_END */
        case 6:
            return; /* AT_EOF */
        }
    }
}

static void Sub_75F7() {
    botInfo = botMem + topMem - topInfo;
    topInfo = topMem;
    RevMemMov(ByteP(botMem), ByteP(botInfo), topInfo - botInfo + 1);
    helpersP     = botInfo - 117 * 2;
    localLabelsP = helpersP - (localLabelCnt + 1) * 2;
    w381E        = localLabelsP - (localLabelCnt + 1);
    w3822        = botInfo - 2;
    while (w3822 >= w381E) {
        *WordP(w3822) = 0;
        w3822         = w3822 - 2;
    }
    if (w3822 == w381E - 1)
        *WordP(w3822) &= 0xff;
    w3822 = w381E - 2;
}

static void Sub_7695() {
    if ((b9B40 = OBJECT || PRINT) || IXREF)
        vfRewind(&nmsf);
    vfRewind(&atf);
    csegSize = dsegSize = 0;
}

static void Sub_76D9() {
    byte i;
    byte T2_Eof = T2_EOF;

    if (b9B40 || IXREF)
        vfRewind(&nmsf);
    vfRewind(&atf); /* used for string data */
    Wr2Byte(T2_EOF);
    vfRewind(&utf2);
    for (i = 1; i <= procCnt; i++) {
        procInfo[i] = procInfo[i] - botMem;
    }
}

void Sub_6EE0() {
    Sub_7695();
    DumpSymbols();
    Sub_711F();
    Sub_719D();
    ProcAtFile();
    Sub_75F7();
    Sub_76D9();
} /* Sub_6EE0() */
