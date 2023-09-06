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

static word externalsCnt = 0;
static word atStmtNum    = 0;

static void Sub_6EF6(word errNum) {
    hasErrors = true;
    Wr2Byte(T2_ERROR);
    Wr2Word(errNum);
    Wr2Word(infoIdx);
    Wr2Word(atStmtNum);
}

int32_t RdAtByte() {
    return vfRbyte(&atf);
}

int32_t RdAtWord() {
    return vfRword(&atf);
}

word GetElementSize() {
    switch (info->type) {
    case BYTE_T:
        return 1; /* byte */
    case ADDRESS_T:
        return 2; /* address */
    case STRUCT_T:
        return info->totalSize;
    }
    return 0;
}

static word GetVarSize() {
    return info->flag & F_ARRAY ? info->dim * GetElementSize() : GetElementSize();
}

static void AdvNextDataInfo() {
    do {
        AdvNxtInfo();
    } while (info && (info->type < BYTE_T || STRUCT_T < info->type));
}

/* determine structure sizes and member offsets */
static void SetStructSizes() {
    word oldSize, varSize, newSize;
    info_t *parent;
    info = infotab;
    AdvNextDataInfo();
    while (info) {
        if (info->type == STRUCT_T) {
            info->totalSize = 0; /* struct Size() is 0 */
        } else if ((info->flag & F_MEMBER)) {
            varSize = GetVarSize();
            parent  = &infotab[info->parent];
            oldSize = parent->totalSize;                 /* this gets Size() so far */
            if ((newSize = oldSize + varSize) < varSize) /* add in the new element */
                Sub_6EF6(ERR208);                        /* LIMIT EXCEEDED: structure Size() */
            parent->totalSize = newSize;                 /* store the running Size() */
            info->linkVal     = oldSize; /* use link value for offset of this member */
        }
        AdvNextDataInfo();
    }
}

static word AllocVar(word addr) {
    info->addr = addr; /* allocate this var's address */
    addr += GetVarSize(); /* reserve it's space */
    if (addr < GetVarSize())
        Sub_6EF6(ERR207); /* LIMIT EXCEEDED: SEGMENT Size() */
    return addr;
}

static void AllocStkVar() {
    info_t *pInfo = procInfo[High(info->scope)];
    pInfo->totalSize = AllocVar(pInfo->totalSize);
}

static void DataAllocation() {
    /* allocate external data ids */

    standAlone = haveModuleLevelUnit;
    info       = infotab;
    AdvNxtInfo();
    while (info) {
        if ((BYTE_T <= info->type && info->type <= STRUCT_T) || info->type == PROC_T ||
            info->type == LABEL_T) {
            if ((info->flag & F_EXTERNAL)) {
                info->extId = externalsCnt++;   // other limits will kick in before this wraps
                info->addr = 0;
                standAlone    = false;
            } else if ((info->flag & F_PUBLIC))
                standAlone = false;
        }
        AdvNxtInfo();
    }

    /* allocate data variables */
    info = infotab;
    AdvNextDataInfo();
    while (info) {
        if (!(info->flag & (F_MEMBER | F_AT | F_EXTERNAL))) {
            if (((info->flag & F_PARAMETER) &&
                 (procInfo[High(info->scope)]->flag & F_EXTERNAL)) ||
                (info->flag & F_BASED))
                info->linkVal = 0; /* external parameter and based var have 0 offset */
            else if ((info->flag & F_DATA))
                csegSize = AllocVar(csegSize); /* allocate initialised data var */
            else if ((info->flag & F_MEMORY))
                ;                                /* memory is predefined */
            else if ((info->flag & F_AUTOMATIC)) /* allocate stack var */
                AllocStkVar();
            else
                dsegSize = AllocVar(dsegSize); /* allocate uninitalised data */
        }
        AdvNextDataInfo();
    }
    infoIdx = 0;
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
    byte atType;
    if (atFData.var.infoOffset == 0)
        atType = 0;
    else if (info->type > STRUCT_T || info->type < BYTE_T) {
        atType = 0;
        Sub_6EF6(ERR211); /* INVALID IDENTIFIER IN 'at' RESTRICTED REFERENCE */
    } else if ((info->flag & F_EXTERNAL))
        atType = 1;
    else if ((info->flag & F_DATA))
        atType = 2;
    else if ((info->flag & F_AUTOMATIC))
        atType = 3;
    else if ((info->flag & F_MEMORY))
        atType = 4;
    else if ((info->flag & F_BASED)) {
        atType = 0;
        Sub_6EF6(ERR212); /* INVALID RESTRICTED REFERENCE IN 'at' , BASE ILLEGAL */
    } else if ((info->flag & F_ABSOLUTE))
        atType = 0;
    else
        atType = 5;

    info_t *savInfo = info; // incase external
    SetInfo(atFData.infoP);

    do {
        if (!(info->flag & F_MEMBER)) {
            if ((info->flag & F_DATA)) { // convert DATA to INITIAL
                info->flag &= ~F_DATA;
                info->flag |= F_INITIAL;
            }
            info->linkVal = atOffset;
            atOffset += GetVarSize();
            switch (atType) {
            case 0:
                info->flag |= F_ABSOLUTE;
                break;
            case 1:
                if ((info->flag & F_PUBLIC))
                    Sub_6EF6(ERR178); /* INVALID 'at' RESTRICTED REFERENCE, external
                                ATTRIBUTE CONFLICTS WITH public ATTRIBUTE */
                info->flag |= F_EXTERNAL;
                info->extId = savInfo->extId;
                break;
            case 2:
                info->flag |= F_DATA;
                break;
            case 3:
                info->flag |= F_AUTOMATIC;
                break;
            case 4:
                info->flag |= F_MEMORY;
                break;
            case 5:
                break;
            }
        }
        AdvNextDataInfo();
    } while (info && (info->flag & (F_PACKED | F_MEMBER)));
}
static void Sub_73DC() {
    RdAtHdr();
    RdAtData();
    if (atFData.infoP == 0)
        return;
    atFData.infoP = atFData.infoP;
    atStmtNum     = atFData.stmt;
    atOffset      = atFData.var.val;
    if (atFData.var.infoOffset) {
        SetInfo(atFData.var.infoOffset);
        if ((info->flag & F_MEMBER)) {
            atOffset += GetElementSize() * atFData.var.nestedArrayIndex + info->linkVal;
            SetInfo(info->parent);
        }
        atOffset += info->linkVal + GetElementSize() * atFData.var.arrayIndex;
        if ((info->flag & F_AT) && infoIdx >= atFData.infoP)
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

static void InitLabelAndProc() {
    localLabels = xmalloc((localLabelCnt + 1) * sizeof(offset_t));
    procIds     = xmalloc((localLabelCnt + 1) * sizeof(byte));
    memset(localLabels, 0, (localLabelCnt + 1) * sizeof(offset_t));
    memset(procIds, 0, (localLabelCnt + 1) * sizeof(byte));
}

static void InitAllocation() {
    vfRewind(&atf);
    csegSize = dsegSize = 0;
}

static void EndT2File() {
    vfRewind(&atf); /* used for string data */
    Wr2Byte(T2_EOF);
    vfRewind(&utf2);
}

void Sub_6EE0() {
    InitAllocation();
    SetStructSizes();
    DataAllocation();
    ProcAtFile();
    InitLabelAndProc();
    EndT2File();
} /* Sub_6EE0() */
