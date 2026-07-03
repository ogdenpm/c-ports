/****************************************************************************
 *  plm1f.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
 * plm1f.c — Data-allocation and AT-clause fixup pass (phase 2, late stage)
 *
 * Overview
 * --------
 * This module performs the final data-layout work of compiler phase 2
 * after the expression trees have been serialised to the tx2 file.  It is
 * driven by the single public entry point AllocateVars(), which:
 *
 *  1. SetStructSizes()   – walk every STRUCT_T / member info entry and
 *                          compute member offsets and total structure sizes.
 *
 *  2. DataAllocation()   – assign addresses to every data variable,
 *                          procedure and label, partitioned into:
 *                            • EXTERNAL / PUBLIC identifiers (extId assigned,
 *                              standAlone updated)
 *                            • CODE-segment variables  (F_DATA  → csegSize)
 *                            • AUTOMATIC stack variables (→ procedure's
 *                              totalSize)
 *                            • DATA-segment variables  (F_DSEG  → dsegSize)
 *
 *  3. ProcAtFile()       – replay the attribute file (atf) written during
 *                          expression parsing to resolve all AT clauses:
 *                          variables with AT(expr) have their storage class,
 *                          address and extId fixed up by ApplyAt() /
 *                          FixDataLoc().
 *
 *  4. InitLabelAndProc() – allocate the localLabels[] and procIds[] arrays
 *                          (sized by localLabelCnt) for later code-generation
 *                          passes.
 *
 *  5. EndT2File()        – write the T2_EOF sentinel to close the tx2 file.
 *
 * Attribute file (atf) record types
 * ----------------------------------
 *  ATI_AHDR    – AT clause header; followed by infoP (target var), stmt
 *                number, and a var_t (source reference + offset).
 *                Processed by ApplyAt().
 *  ATI_DHDR    – DATA/INITIAL header (infoP + stmt).  Read but not acted on
 *                in this pass.
 *  ATI_2       – a 16-bit value (stored in atFData.var.val).
 *  ATI_STRING  – a string initialiser; length word followed by bytes.
 *                Discarded in this pass.
 *  ATI_DATA    – a var_t record (read into atFData.var).
 *  ATI_END     – end of a record group.
 *  ATI_EOF     – end of the attribute file; terminates ProcAtFile().
 *
 * Error codes used
 * ----------------
 *  ERR207  LIMIT EXCEEDED: SEGMENT SIZE
 *  ERR208  LIMIT EXCEEDED: STRUCTURE SIZE
 *  ERR211  INVALID IDENTIFIER IN 'AT' RESTRICTED REFERENCE
 *  ERR212  INVALID RESTRICTED REFERENCE IN 'AT', BASE ILLEGAL
 *  ERR213  UNDEFINED RESTRICTED REFERENCE IN 'AT'
 *  ERR178  INVALID 'AT' RESTRICTED REFERENCE, EXTERNAL CONFLICTS WITH PUBLIC
 */

#include "os.h"
#include "plm.h"

/* externalsCnt – running count of EXTERNAL identifiers; used to assign
   unique extId values to each external symbol. */
static uint16_t externalsCnt = 0;

/* atStmtNum – statement number of the AT clause currently being processed;
   used when reporting errors via ParseError(). */
static uint16_t atStmtNum = 0;

/*
 * ParseError – record a data-allocation or AT-clause error.
 *
 * Sets hasErrors, then writes a T2_ERROR record to the tx2 stream
 * containing the error number, the current info index, and the statement
 * number of the AT clause being processed (atStmtNum).
 */
static void ParseError(uint16_t errNum) {
    hasErrors = true;
    Wr2Byte(T2_ERROR);
    Wr2Word(errNum);
    Wr2Word(ToIdx(info));
    Wr2Word(atStmtNum);
}

/*
 * RdAtByte – read one byte from the attribute file (atf).
 *
 * Returns the byte value as int32_t (EOF returns a negative value from
 * the underlying vfRbyte helper).
 */
int32_t RdAtByte() {
    return vfRbyte(&atf);
}

/*
 * RdAtWord – read a 16-bit word from the attribute file (atf).
 */
int32_t RdAtWord() {
    return vfRword(&atf);
}

/*
 * GetElementSize – return the size in bytes of a single element of pInfo.
 *
 *   BYTE_T    → 1
 *   ADDRESS_T → 2
 *   STRUCT_T  → pInfo->totalSize  (computed by SetStructSizes)
 *   other     → 0
 */
uint16_t GetElementSize(info_t *pInfo) {
    switch (pInfo->type) {
    case BYTE_T:
        return 1; /* uint8_t */
    case ADDRESS_T:
        return 2; /* address */
    case STRUCT_T:
        return pInfo->totalSize;
    }
    return 0;
}

/*
 * GetVarSize – return the total byte size of the variable described by info.
 *
 * For arrays (F_ARRAY): dim * GetElementSize(info).
 * For scalars:          GetElementSize(info).
 */
static uint16_t GetVarSize() {
    return info->flag & F_ARRAY ? info->dim * GetElementSize(info) : GetElementSize(info);
}

/*
 * AdvNextDataInfo – advance info to the next BYTE_T / ADDRESS_T / STRUCT_T entry.
 *
 * Calls AdvNxtInfo() repeatedly until info is NULL or its type falls in the
 * range [BYTE_T, STRUCT_T].  This skips over non-data info entries (labels,
 * procedures, literals, etc.) during data-allocation walks.
 */
static void AdvNextDataInfo() {
    do {
        AdvNxtInfo();
    } while (info && (info->type < BYTE_T || STRUCT_T < info->type));
}

/*
 * SetStructSizes – compute structure member offsets and total sizes.
 *
 * Walks every data info entry in the info table:
 *
 *  STRUCT_T  → resets totalSize to 0 (accumulated by its members below).
 *
 *  F_MEMBER  → adds the member's GetVarSize() to its parent structure's
 *              totalSize.  Stores the pre-addition parent size in
 *              info->linkVal as the byte offset of this member within the
 *              structure.  ERR208 if the cumulative size overflows uint16_t.
 *
 * This pass must run before DataAllocation() because GetElementSize()
 * for STRUCT_T references totalSize.
 */
static void SetStructSizes() {
    uint16_t oldSize, varSize, newSize;
    info_t *parent;
    info = infotab;
    AdvNextDataInfo();
    while (info) {
        if (info->type == STRUCT_T) {
            info->totalSize = 0; /* struct Size() is 0 */
        } else if ((info->flag & F_MEMBER)) {
            varSize = GetVarSize();
            parent  = info->parent;
            oldSize = parent->totalSize;                 /* this gets Size() so far */
            if ((newSize = oldSize + varSize) < varSize) /* add in the new element */
                ParseError(ERR208);                      /* LIMIT EXCEEDED: structure Size() */
            parent->totalSize = newSize;                 /* store the running Size() */
            info->linkVal     = oldSize; /* use link value for offset of this member */
        }
        AdvNextDataInfo();
    }
}

/*
 * AllocVar – assign an address to the current info entry and advance addr.
 *
 * Stores addr in info->addr and returns addr + GetVarSize().  ERR207 is
 * raised if the addition wraps (segment size overflow).
 */
static uint16_t AllocVar(uint16_t addr) {
    info->addr = addr;    /* allocate this var's address */
    addr += GetVarSize(); /* reserve it's space */
    if (addr < GetVarSize())
        ParseError(ERR207); /* LIMIT EXCEEDED: SEGMENT Size() */
    return addr;
}

/*
 * AllocStkVar – allocate an AUTOMATIC variable on its procedure's stack frame.
 *
 * Locates the enclosing procedure's info entry via procInfo[High(info->scope)]
 * and calls AllocVar() against that procedure's totalSize (which doubles as
 * the running frame-size counter).
 */
static void AllocStkVar() {
    info_t *pInfo    = procInfo[High(info->scope)];
    pInfo->totalSize = AllocVar(pInfo->totalSize);
}

/*
 * DataAllocation – assign addresses and external IDs to all data symbols.
 *
 * Pass 1 – EXTERNAL / PUBLIC accounting:
 *   Walks every BYTE_T / ADDRESS_T / STRUCT_T / PROC_T / LABEL_T info entry.
 *   EXTERNAL symbols receive a unique extId from externalsCnt and have their
 *   address zeroed; both EXTERNAL and PUBLIC symbols clear standAlone
 *   (a module with external linkage cannot stand alone).
 *
 * Pass 2 – address assignment for non-member, non-AT, non-external data:
 *   F_PARAMETER of EXTERNAL proc / F_BASED → linkVal = 0 (no offset).
 *   F_DATA (initialised)                   → csegSize via AllocVar().
 *   F_MEMORY (predefined address)          → no action.
 *   F_AUTOMATIC (stack)                    → AllocStkVar().
 *   otherwise (uninitialised static)       → dsegSize via AllocVar().
 */
static void DataAllocation() {
    /* allocate external data ids */

    standAlone = haveModuleLevelUnit;
    info       = infotab;
    AdvNxtInfo();
    while (info) {
        if ((BYTE_T <= info->type && info->type <= STRUCT_T) || info->type == PROC_T ||
            info->type == LABEL_T) {
            if ((info->flag & F_EXTERNAL)) {
                info->extId = externalsCnt++; // other limits will kick in before this wraps
                info->addr  = 0;
                standAlone  = false;
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
            if (((info->flag & F_PARAMETER) && (procInfo[High(info->scope)]->flag & F_EXTERNAL)) ||
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
}

/*
 * atFData – working buffer for one attribute-file record.
 *
 * Overlaid as a raw byte array (for ATI_STRING bulk reads) and as a
 * structured view holding the record type, the target variable's info
 * index, the source statement number, and the parsed var_t (source
 * reference + signed offset).
 */
union {
    uint8_t str[256];
    struct {
        uint8_t type;  /* ATI_xxx record type byte */
        index_t infoP; /* info index of the target variable */
        uint16_t stmt; /* source statement number */
        var_t var;     /* source reference: infoIdx, subscript, memberSubscript, val */
    };
} atFData;

/* atOffset – running byte offset accumulated while assigning addresses to
   the packed variables of an AT clause. */
uint16_t atOffset;

/*
 * RdAtHdr – read the infoP and stmt fields of an ATI_AHDR or ATI_DHDR record.
 */
void RdAtHdr() {
    atFData.infoP = RdAtWord();
    atFData.stmt  = RdAtWord();
}

/*
 * RdAtData – read a var_t record from the attribute file into atFData.var.
 */
void RdAtData() {
    vfRbuf(&atf, (uint8_t *)&atFData.var, sizeof(var_t));
}

/*
 * FixDataLoc – apply the storage-class and address of the AT source to all
 *              packed variables of the AT target.
 *
 * Classifies the source variable (info, set by ApplyAt) into one of six
 * location types:
 *
 *  0 – absolute / unknown or BASED (ERR212) or invalid type (ERR211).
 *      Target variables get F_ABSOLUTE; their addresses come from atOffset.
 *  1 – EXTERNAL source.  Target variables inherit F_EXTERNAL and the
 *      source's extId.  ERR178 if the target is also PUBLIC.
 *  2 – initialised data (F_DATA).    Targets get F_DATA.
 *  3 – AUTOMATIC (stack).            Targets get F_AUTOMATIC.
 *  4 – MEMORY.                       Targets get F_MEMORY.
 *  5 – uninitialised static data.    No extra flag.
 *
 * For each target variable in the packed group (identified by F_AT start
 * and continuing while F_PACKED or F_MEMBER is set):
 *   • F_DATA is converted to F_INITIAL (the variable may not live in the
 *     normal DATA segment once its address is overridden).
 *   • info->addr is set to atOffset; atOffset advances by GetVarSize().
 *   • The storage-class flag (above) is applied.
 *   • Structure members are skipped (they inherit from the parent).
 */
static void FixDataLoc() {
    uint8_t locType;
    if (atFData.var.infoIdx == 0)
        locType = 0;
    else if (info->type < BYTE_T || info->type > STRUCT_T) { // not BYTE, ADDRESS, STRUCTURE
        locType = 0;
        ParseError(ERR211); /* INVALID IDENTIFIER IN 'AT' RESTRICTED REFERENCE */
    } else if ((info->flag & F_EXTERNAL))
        locType = 1;
    else if ((info->flag & F_DATA))
        locType = 2;
    else if ((info->flag & F_AUTOMATIC))
        locType = 3;
    else if ((info->flag & F_MEMORY))
        locType = 4;
    else if ((info->flag & F_BASED)) {
        locType = 0;
        ParseError(ERR212); /* INVALID RESTRICTED REFERENCE IN 'AT' , BASE ILLEGAL */
    } else if ((info->flag & F_ABSOLUTE))
        locType = 0;
    else
        locType = 5;

    info_t *savInfo = info;                   // in case external
    info            = FromIdx(atFData.infoP); // start of restricted references

    // process any packed declarations. structure members are skipped
    do {
        if (!(info->flag & F_MEMBER)) {  // members inherit parent info and retain offsets
            if ((info->flag & F_DATA)) { // convert DATA to INITIAL
                info->flag &= ~F_DATA;   // as may not be in usual DATA space
                info->flag |= F_INITIAL;
            }
            info->addr = atOffset;
            atOffset += GetVarSize();
            switch (locType) {
            case 0:
                info->flag |= F_ABSOLUTE;
                break;
            case 1:
                if ((info->flag & F_PUBLIC))
                    ParseError(ERR178); /* INVALID 'AT' RESTRICTED REFERENCE, EXTERNAL
                                ATTRIBUTE CONFLICTS WITH PUBLIC ATTRIBUTE */
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

/*
 * ApplyAt – resolve one ATI_AHDR record from the attribute file.
 *
 * Reads the target variable's info index (atFData.infoP) and the source
 * reference (var_t) written by plm1e.c's Locator().
 *
 * If atFData.infoP is 0, nothing needs to be done (empty AT clause).
 *
 * Otherwise the base address (atOffset) of the packed group is computed
 * from atFData.var.val plus adjustments for:
 *   • structure member offset and member subscript (info->linkVal,
 *     GetElementSize * memberSubscript)
 *   • variable subscript (linkVal + GetElementSize * subscript)
 *
 * ERR213 is raised if the source variable itself has F_AT but its info
 * index is at or after the target group (circular / undefined reference).
 *
 * FixDataLoc() is then called to propagate the resolved address and
 * storage class to all packed target variables.
 */
static void ApplyAt() {
    RdAtHdr();
    RdAtData();
    if (atFData.infoP == 0) // nothing to do
        return;
    atStmtNum = atFData.stmt;
    atOffset  = atFData.var.val;                 // at location offset
    if ((info = FromIdx(atFData.var.infoIdx))) { // using reference
        if ((info->flag & F_MEMBER)) {           // if member adjust offset from structure
            atOffset += GetElementSize(info) * atFData.var.memberSubscript + info->linkVal;
            info = info->parent;
        }
        atOffset +=
            info->linkVal + GetElementSize(info) * atFData.var.subscript; // adjust for indexing
        if ((info->flag & F_AT) && info >= FromIdx(atFData.infoP))        // AT but no at location
            ParseError(ERR213); /* UNDEFINED RESTRICTED REFERENCE IN 'AT' */
    }
    FixDataLoc(); // with AT, DATA/INITIAL locations need fixup
}

/*
 * ProcAtFile – replay the attribute file, processing all AT clauses.
 *
 * Reads ATI_xxx type bytes and dispatches:
 *
 *  ATI_AHDR    → ApplyAt() (full AT clause: read header + var_t, resolve)
 *  ATI_DHDR    → read infoP + stmt into atFData (DATA/INITIAL header; not
 *                acted on in this pass)
 *  ATI_2       → read a 16-bit value into atFData.var.val
 *  ATI_STRING  → read length + bytes into atFData.str (discarded)
 *  ATI_DATA    → RdAtData() into atFData.var
 *  ATI_END     → no action (end-of-group marker)
 *  ATI_EOF     → return (end of attribute file)
 */
static void ProcAtFile() {
    while (1) {
        atFData.type = RdAtByte();
        switch (atFData.type) {
        case ATI_AHDR:
            ApplyAt();
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

/*
 * InitLabelAndProc – allocate the localLabels and procIds arrays.
 *
 * Both arrays are sized to (localLabelCnt + 1) entries and zero-initialised.
 * localLabels[] maps each compiler-generated label index to its code offset
 * (filled in by later code-generation passes).
 * procIds[] maps each label index to its enclosing procedure ID.
 */
static void InitLabelAndProc() {
    localLabels = safeMalloc((localLabelCnt + 1) * sizeof(offset_t));
    procIds     = safeMalloc((localLabelCnt + 1) * sizeof(uint8_t));
    memset(localLabels, 0, (localLabelCnt + 1) * sizeof(offset_t));
    memset(procIds, 0, (localLabelCnt + 1) * sizeof(uint8_t));
}

/*
 * InitAllocation – prepare for the data-allocation pass.
 *
 * Rewinds the attribute file to the beginning so ProcAtFile() can replay
 * it, and resets csegSize and dsegSize to zero.
 */
static void InitAllocation() {
    vfRewind(&atf);
    csegSize = dsegSize = 0;
}

/*
 * EndT2File – write the T2_EOF sentinel to the tx2 file.
 *
 * Signals later passes that there are no more T2 records.
 */
static void EndT2File() {
    Wr2Byte(T2_EOF);
}

/*
 * AllocateVars – top-level data-allocation entry point (phase 2 finalisation).
 *
 * Executes the full data-layout pipeline in order:
 *  1. InitAllocation()   – rewind atf, zero segment size counters.
 *  2. SetStructSizes()   – compute structure member offsets and total sizes.
 *  3. DataAllocation()   – assign addresses to all variables and external IDs.
 *  4. ProcAtFile()       – resolve AT clauses, fixing up addresses and
 *                           storage classes for AT-located variables.
 *  5. InitLabelAndProc() – allocate localLabels[] and procIds[] arrays.
 *  6. EndT2File()        – write T2_EOF to close the tx2 stream.
 */
void AllocateVars() {
    InitAllocation();
    SetStructSizes();
    DataAllocation();
    ProcAtFile();
    InitLabelAndProc();
    EndT2File();
}