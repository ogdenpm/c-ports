/****************************************************************************
 *  accessors.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

byte GetBuiltinId() {

    return infotab[infoIdx].builtinId;
}

void SetBuiltinId(byte id) {

    infotab[infoIdx].builtinId = id;
}

offset_t GetBaseOffset() {
    return infotab[infoIdx].baseOff;
}

void SetBaseOffset(offset_t baseP) {
    infotab[infoIdx].baseOff = baseP;
}

word GetBaseVal() {
    return infotab[infoIdx].baseVal;
}

void SetBaseVal(word val) {
    infotab[infoIdx].baseVal = val;
}

byte GetCondFlag() {
    return infotab[infoIdx].condFlag;
}

void SetCondFlag(byte flag) {
    infotab[infoIdx].condFlag = flag;
}

word GetDimension() {
    return infotab[infoIdx].dim;
}

word GetDimension2() {
    return infotab[infoIdx].dim;
}

void SetDimension(word dim) {
    infotab[infoIdx].dim = dim;
}

byte GetDataType() {
    if (infotab[infoIdx].type == PROC_T)
        return infotab[infoIdx].dtype;
    else
        return infotab[infoIdx].dataType;
}

void SetDataType(byte dtype) {
    if (infotab[infoIdx].type == PROC_T)
        infotab[infoIdx].dtype = dtype;
    else
        infotab[infoIdx].dataType = dtype;
}

byte GetExternId() {
    return infotab[infoIdx].extId;
}

void SetExternId(byte id) {
    infotab[infoIdx].extId = id;
    ;
}

void SetInfoFlag(byte flag) {
    SetFlag(infotab[infoIdx].flag, flag);
}

byte GetIntrNo() {
    return infotab[infoIdx].intno;
}

void SetIntrNo(byte intNo) {
    infotab[infoIdx].intno = intNo;
}

pstr_t const *GetLit() {
    return infotab[infoIdx].lit;
}

void SetLit(pstr_t const *litAddr) {
    infotab[infoIdx].lit = litAddr;
}

offset_t GetLinkOffset() {
    return infotab[infoIdx].ilink;
}

void SetLinkOffset(offset_t link) {
    infotab[infoIdx].ilink = link;
}

word GetLinkVal() {
    return infotab[infoIdx].linkVal;
}

void SetLinkVal(word val) {
    infotab[infoIdx].linkVal = val;
}

offset_t GetParentOffset() {
    return infotab[infoIdx].parent; // special handling of STRUCT_T no longer required
}

void SetParentOffset(offset_t parent) {
    infotab[infoIdx].parent = parent;    // special handling of STRUCT_T no longer required
}

word GetParentVal() {
    return infotab[infoIdx].parent;
}

byte GetParamCnt() {
    if (infotab[infoIdx].type == PROC_T)
        return infotab[infoIdx].pcnt;
    else
        return infotab[infoIdx].paramCnt;
}

void SetParamCnt(byte cnt) {
    if (infotab[infoIdx].type == PROC_T)
        infotab[infoIdx].pcnt = cnt;
    else
        infotab[infoIdx].paramCnt = cnt;
}

byte GetProcId() {
    return infotab[infoIdx].procId;
}

void SetProcId(byte id) {
    infotab[infoIdx].procId = id;
}

word GetScope() {
    return infotab[infoIdx].scope;
}

void SetScope(word scope) {
    infotab[infoIdx].scope = scope;
}

index_t GetSymbol() {
    return infotab[infoIdx].sym;
}

void SetSymbol(index_t symbol) {
    infotab[infoIdx].sym = symbol;
}

byte GetType() {
    return infotab[infoIdx].type;
}

void SetType(byte type) {
    infotab[infoIdx].type = type;
}

// flag access functions
/* F_PUBLIC = 0, F_EXTERNAL = 1, F_BASED = 2, (F_INITIAL = 3, F_REENTRANT = 4), (F_DATA = 5,
              F_INTERRUPT = 6), F_AT = 7, F_ARRAY = 8, F_STARDIM = 9, F_PARAMETER = 10,
              F_MEMBER = 11, F_LABEL = 12, F_AUTOMATIC = 13, F_PACKED = 14, F_ABSOLUTE = 15,
              F_MEMORY = 16, F_DECLARED = 17, F_DEFINED = 18,
              F_MODGOTO = 19 */
static byte tblOffsets[]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 };
static byte tblBitFlags[] = { 0x80, 0x40, 0x20, 0x10, 0x10, 8, 8, 4, 2,    1,
                              0x80, 0x40, 0x20, 0x10, 8,    4, 2, 1, 0x80, 0x40 };

void ClrFlag(pointer base, byte flag) {
    byte off, mask;

    off  = tblOffsets[flag];
    mask = ~tblBitFlags[flag];
    base[off] &= mask;
}

void ClrFlags(pointer base) {
    byte i;

    for (i = 0; i <= 2; i++) {
        base[i] = 0;
    }
}

// void ClrInfoFlag(byte flag)
//{
//	ClrFlag(InfoP(curInfoP)->flag, flag);
// }

void CpyFlags(pointer base) {
    byte i;

    for (i = 0; i <= 2; i++) {
        infotab[infoIdx].flag[i] = base[i];
    }
}

void SetFlag(pointer base, byte flag) {
    byte off, bit;

    off = tblOffsets[flag];
    bit = tblBitFlags[flag];
    base[off] |= bit;
}

bool TestFlag(pointer base, byte flag) {
    byte off, bit;

    off = tblOffsets[flag];
    bit = tblBitFlags[flag];
    if ((base[off] & bit) != 0)
        return true;
    else
        return false;
} /* TestFlag() */

byte TestInfoFlag(byte flag) {
    return TestFlag(infotab[infoIdx].flag, flag);
}
