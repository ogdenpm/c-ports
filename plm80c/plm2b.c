/****************************************************************************
 *  plm2b.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

static byte nodeSize; // lifted to file scope

static void cvtFromRel(byte rel, word *ploc) {
    word p;

    if (*ploc) {
        if (rel > nodeSize)
            *ploc = 0;
        else {
            p     = *ploc;
            *ploc = tx2qp;

            while (p--) {
                // backup, adjusting for lineinfo with just block counts (extra)
                if (tx2[--*ploc].nodeType == T2_LINEINFO && tx2[*ploc].right == 0 &&
                    tx2[*ploc].extra != 0)
                    p -= (tx2[*ploc].extra - tx2[*ploc].left); // skip block nesting data
            }
        }
    }
} /* sub69EB */

static void DeRelNode() {
    nodeSize = nodeControlFlags & 3;
    if ((nodeControlFlags & 4)) {          // JMPFALSE CASEBLOCK
        tx2[1].right    = tx2[tx2qp].left; // set up dummy locallabel entry
        tx2[tx2qp].left = 1;               // reference the dummy
    } else
        cvtFromRel(1, &tx2[tx2qp].left);

    cvtFromRel(2, &tx2[tx2qp].right);
    if (nodeSize == 3) {
        if (curNodeType == T2_BYTEINDEX || curNodeType == T2_WORDINDEX) {
            word leftLoc = (byte)tx2[tx2qp].left;
            tx2[leftLoc].right += tx2[tx2qp].extra * GetElementSize(FromIdx(tx2[leftLoc].left));
        } else if (curNodeType != T2_CALL)    // T2_CALL convert to info pointer done elsewhere
            cvtFromRel(3, &tx2[tx2qp].extra); // otherwise convert from rel
    }
    tx2[tx2qp].exprAttr = LIT_A;
    tx2[tx2qp].exprLoc   = LOC_SPECIAL;
}

static byte typeExprMap[] = {
    LIT_A, LABEL_A, BYTE_A, ADDRESS_A, STRUCT_A, BYTE_A, BYTE_A
}; // index by info->type (excludes Macro, unk and condvar

static void simpleVal() {
    if (curNodeType == T2_IDENTIFIER) {
        info                = FromIdx(tx2[tx2qp].left);
        tx2[tx2qp].right    = info->linkVal;
        tx2[tx2qp].exprAttr = typeExprMap[info->type];
        tx2[tx2qp].exprLoc =
            (info->flag & (F_MEMBER | F_AUTOMATIC)) == F_AUTOMATIC ? LOC_STACK : LOC_MEM;
    } else if (curNodeType <= T2_BIGNUMBER) {
        tx2[tx2qp].nodeType = T2_NUMBER;
        tx2[tx2qp].right    = tx2[tx2qp].left;
        tx2[tx2qp].left     = 0;
        tx2[tx2qp].exprAttr = curNodeType == T2_BIGNUMBER ? ADDRESS_A : BYTE_A;
        tx2[tx2qp].exprLoc   = LOC_REG;
    } else {
        tx2[tx2qp].exprAttr = BYTE_A;
        tx2[tx2qp].right    = 0;
    }
}

void DeRelStmt() {
    for (tx2qp = 4; tx2qp < tx2qNxt; tx2qp++) {
        curNodeType      = tx2[tx2qp].nodeType;
        nodeControlFlags = nodeControlMap[curNodeType];
        if ((nodeControlFlags & 0xc0) == 0)
            // LT LE NE EQ GE GT ROL ROR SCL SCR SHL SHR JMPFALSE DOUBLE PLUSSIGN MINUSSIGN STAR
            // SLASH MOD AND OR XOR BASED BYTEINDEX WORDINDEX MEMBER UNARYMINUS NOT LOW HIGH
            // ADDRESSOF PLUS MINUS TIME STKBARG STKWARG DEC COLONEQUALS OUTPUT CASEBLOCK STKARG
            // MOVE RETURNBYTE RETURNWORD RETURN ADDW BEGMOVE CALL CALLVAR SETADDR
            DeRelNode();
        else if ((nodeControlFlags & 0xc0) == 0x40)
            // IDENTIFIER NUMBER BIGNUMBER LENGTH LAST SIZE
            simpleVal();
    }
}
