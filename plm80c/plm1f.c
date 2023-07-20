/****************************************************************************
 *  plm1f.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"

static byte externalsCnt = 0;
static word atStmtNum    = 0;

static void Sub_6EF6(word arg) {
    hasErrors = true;
    Wr2Byte(T2_ERROR);
    Wr2Word(arg);
    Wr2Word(infoIdx);
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

static index_t AdvNextDataInfo(index_t idx) {
    while (1) {
        idx = AdvNxtInfo(idx);
        if (idx == 0 || infotab[idx].type >= BYTE_T && infotab[idx].type <= STRUCT_T)
            return idx;
    }
}


static void Sub_711F() {
    word p, q, r, s;

    for (infoIdx = AdvNextDataInfo(0); infoIdx != 0; infoIdx = AdvNextDataInfo(infoIdx)) {
        if (GetType() == STRUCT_T) {
            SetParentOffset(0); /* struct Size() is 0 */
        } else if (TestInfoFlag(F_MEMBER)) {
            r        = GetVarSize();
            p        = infoIdx;
            infoIdx = GetParentOffset(); /* get structure info */
            q        = GetParentOffset(); /* this gets Size() so far */
            if ((s = q + r) < r)          /* add in the new element */
                Sub_6EF6(ERR208);         /* LIMIT EXCEEDED: structure Size() */
            SetParentOffset(s);           /* store the running Size() */
            infoIdx = p;
            SetLinkVal(q); /* use link value for offset of this memeber */
        }
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

    p        = infoIdx;
    infoIdx = procInfo[High(GetScope())];
    q        = &infotab[infoIdx].parent; /* varsize union with parent */
    infoIdx = p;
    AllocVarAddress(q);
}

static void Sub_719D() {
    byte i;
    word p;

    /* allocate external data ids */

    standAlone = haveModuleLevelUnit;

    for (infoIdx = AdvNxtInfo(0); infoIdx != 0; infoIdx = AdvNxtInfo(infoIdx)) {
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
    }

    /* allocate data variables */

    for (infoIdx = AdvNextDataInfo(0); infoIdx != 0; infoIdx = AdvNextDataInfo(infoIdx)) {
        if (!(TestInfoFlag(F_MEMBER) || TestInfoFlag(F_AT) || TestInfoFlag(F_EXTERNAL))) {
            if (TestInfoFlag(F_PARAMETER)) {
                p        = infoIdx;
                infoIdx = procInfo[High(GetScope())];
                i        = TestInfoFlag(F_EXTERNAL); /* only allocate parameter if (! external */
                infoIdx = p;
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
    }
}

union {
    byte str[256];
    struct {
        byte type;
        offset_t infoP;
        word stmt;
        var_t var;
    };

} atFData;

word atOffset;
void RdAtHdr() {
    atFData.infoP = RdAtWord();
    atFData.stmt  = RdAtWord();
}
void RdAtData() {
    vfRbuf(&atf, (uint8_t *)&atFData.var, sizeof(var_t));
}
static void Sub_7486() {
    byte i, j;
    if (atFData.var.infoOffset == 0)
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
    infoIdx = atFData.infoP;

    while (1) {
        if (!TestInfoFlag(F_MEMBER)) {
            if (TestInfoFlag(F_DATA)) {
                ClrFlag(infotab[infoIdx].flag, F_DATA);
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
        infoIdx = AdvNextDataInfo(infoIdx);
        if (infoIdx == 0 || !(TestInfoFlag(F_PACKED) || TestInfoFlag(F_MEMBER)))
            return;
    }
}
static void Sub_73DC() {
    RdAtHdr();
    RdAtData();
    if (atFData.infoP == 0)
        return;
    atFData.infoP = atFData.infoP;
    atStmtNum     = atFData.stmt;
    atOffset      = atFData.var.val;
    if (atFData.var.infoOffset != 0) {
        infoIdx = atFData.var.infoOffset;
        if (TestInfoFlag(F_MEMBER)) {
            atOffset = GetElementSize() * atFData.var.nestedArrayIndex + atOffset + GetLinkVal();
            infoIdx = GetParentOffset();
        }
        atOffset = GetLinkVal() + GetElementSize() * atFData.var.arrayIndex + atOffset;
        if (TestInfoFlag(F_AT))
            if (infoIdx >= atFData.infoP)
                Sub_6EF6(ERR213); /* UNDEFINED RESTRICTED REFERENCE IN 'at' */
    }
    Sub_7486();
}

static void ProcAtFile() {

    while (1) {
        atFData.type = RdAtByte();
        switch (atFData.type) {
        case ATI_AHDR:
            Sub_73DC();
            break;
        case ATI_DHDR:
            atFData.infoP = RdAtWord();
            atFData.stmt  = RdAtWord();
            break;
        case ATI_2:
            atFData.var.val = RdAtWord();
            break;
        case ATI_STRING:
            atFData.var.val = RdAtWord();
            vfRbuf(&atf, atFData.str, atFData.var.val); // effectively junk it.
            break;
        case ATI_DATA:
            RdAtData();
            break;
        case ATI_END:
            break;
        case ATI_EOF:
            return;
        }
    }
}

static void Sub_75F7() {
    localLabels = xmalloc((localLabelCnt + 1) * sizeof(offset_t));
    procIds     = xmalloc((localLabelCnt + 1) * sizeof(byte));
    memset(localLabels, 0, (localLabelCnt + 1) * sizeof(offset_t));
    memset(procIds, 0, (localLabelCnt + 1) * sizeof(byte));
}

static void Sub_7695() {
    vfRewind(&atf);
    csegSize = dsegSize = 0;
}

static void Sub_76D9() {
    byte T2_Eof = T2_EOF;

    vfRewind(&atf); /* used for string data */
    Wr2Byte(T2_EOF);
    vfRewind(&utf2);
}

void Sub_6EE0() {
    Sub_7695();
    //   DumpSymbols();
    Sub_711F();
    Sub_719D();
    ProcAtFile();
    Sub_75F7();
    Sub_76D9();
} /* Sub_6EE0() */
