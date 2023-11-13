/****************************************************************************
 *  plm2c.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

static byte bC259, bC25A; // lifted to file scope

static void Sub_6C54(byte nodeLoc) {
    if (nodeLoc)
        tx2[nodeLoc].cnt++;
}

static void OptimiseNodes(byte nodeLoc) {
    if (OPTIMIZE)
        while (tx2qp > ++nodeLoc) {
            if (tx2[nodeLoc].nodeType == curNodeType && tx2[nodeLoc].left == tx2[tx2qp].left &&
                tx2[nodeLoc].right == tx2[tx2qp].right && tx2[nodeLoc].exprAttr == tx2[tx2qp].exprAttr) {
                if (tx2[nodeLoc].extra != 0xff00) {
                    tx2[tx2qp].nodeType = T2_OPTBACKREF;
                    tx2[tx2qp].left = nodeLoc;
                    return;
                }
            }
        }
    if ((nodeControlFlags & 0xc0) == 0) {
        // LT LE NE EQ GE GT ROL ROR SCL SCR SHL SHR JMPFALSE DOUBLE PLUSSIGN MINUSSIGN STAR
        // SLASH MOD AND OR XOR BASED BYTEINDEX WORDINDEX MEMBER UNARYMINUS NOT LOW HIGH
        // ADDRESSOF PLUS MINUS TIME STKBARG STKWARG DEC COLONEQUALS OUTPUT CASEBLOCK STKARG
        // MOVE RETURNBYTE RETURNWORD RETURN ADDW BEGMOVE CALL CALLVAR SETADDR
        Sub_6C54((byte)tx2[tx2qp].left);
        Sub_6C54((byte)tx2[tx2qp].right);
    }
    tx2[tx2qp].extra = tx2qp;
}

static bool isExpensive;

static void FixBackRef(wpointer pNodeIdx) {
    if (*pNodeIdx && tx2[*pNodeIdx].nodeType == T2_OPTBACKREF)
        *pNodeIdx = tx2[*pNodeIdx].left;
}

static void chkComponentAccess(byte nodeLoc) {
    byte nodeType;
    if ((nodeType = tx2[nodeLoc].nodeType) == T2_BASED)
        isExpensive = true;
    else if (nodeType == T2_BYTEINDEX || nodeType == T2_WORDINDEX) {
        if (tx2[tx2[nodeLoc].right].nodeType != T2_NUMBER)
            isExpensive = true;
        else {
            info = FromIdx(tx2[tx2[nodeLoc].left].left);
            if (tx2[tx2[nodeLoc].right].right >= info->dim || (info->flag & F_AT))
                isExpensive = true;
        }
    } else if (nodeType == T2_IDENTIFIER) {
        info = FromIdx(tx2[nodeLoc].left);
        if ((info->flag & F_AT))
            isExpensive = true;
    }
} /* Sub_6F20() */

static void chkVarAccess(byte nodeLoc) {
    isExpensive = false;
    if (tx2[nodeLoc].nodeType == T2_MEMBER) {
        chkComponentAccess((byte)tx2[nodeLoc].left);
        chkComponentAccess((byte)tx2[nodeLoc].right);
    } else
        chkComponentAccess(nodeLoc);
}

static byte bC263;

static void Sub_7018(byte arg1b) {
    if (tx2[arg1b].nodeType != T2_CALL)
        arg1b = (byte)tx2[arg1b].extra;
    if (arg1b && arg1b > bC263)
        bC263 = arg1b;
}

static byte Sub_6FE2() {
    bC263 = isExpensive ? bC259 : bC25A;

    Sub_7018((byte)tx2[tx2qp].left);
    Sub_7018((byte)tx2[tx2qp].right);
    return bC263;
}

static bool sub_70BC(byte nodeLoc) {
    chkVarAccess((byte)tx2[nodeLoc].left);
    if (isExpensive)
        return true;
    /* PMO - code modified to use the nodeType, 
       previously the code checked tx2[nodeLoc].left == T2_IDENTIFIER (172)
       except for complex statements this should never be true. With complex
       statements with > 172 nodes, it is unclear what would happen
       with this fix the code for reentrant code is sometimes improved
    */
    if (tx2[tx2[nodeLoc].left].nodeType == T2_IDENTIFIER) {
        info = FromIdx(tx2[tx2[nodeLoc].left].left);
        if ((info->flag & F_AUTOMATIC))
            return true;
    }
    return false;
}

void Sub_7055() {
    if (sub_70BC(tx2qp))
        bC25A = tx2qp;
    if (tx2[tx2[bC259 = tx2qp].left].extra != 0xff00)
        tx2[tx2[tx2qp].left].extra = tx2qp;
    Sub_6C54((byte)tx2[tx2qp].left);
    Sub_6C54((byte)tx2[tx2qp].right);
}

static void Sub_6D52() {
    FixBackRef(&tx2[tx2qp].left);
    FixBackRef(&tx2[tx2qp].right);
    if (curNodeType == T2_COLONEQUALS)
        Sub_7055();
    else if (procCallDepth > 0) {
        Sub_6C54((byte)tx2[tx2qp].left);
        Sub_6C54((byte)tx2[tx2qp].right);
        if (curNodeType == T2_CALL)
            procCallDepth--;
        else if (curNodeType == T2_MOVE || curNodeType == T2_CALLVAR) {
            procCallDepth--;
            FixBackRef(&tx2[tx2qp].extra);
            Sub_6C54((byte)tx2[tx2qp].extra);
        } else
            tx2[tx2qp].extra = 0xff00;
    } else if (curNodeType == T2_OUTPUT || curNodeType == T2_TIME) {
        Sub_6C54((byte)tx2[tx2qp].left);
        Sub_6C54((byte)tx2[tx2qp].right);
    } else {
        tx2[tx2qp].extra = 0;
        chkVarAccess(tx2qp);
        OptimiseNodes(Sub_6FE2());
        if (curNodeType == T2_JMPFALSE && tx2[tx2qp - 1].nodeType == T2_NOT) {
            boC20F        = true;
            tx2[tx2qp].right = tx2[tx2qp - 1].left;
            Sub_56A0(tx2qp, tx2qp - 1);
            tx2[tx2qp].nodeType = T2_SEMICOLON;
        }
    }
}

static void Sub_7111() {
    if (procCallDepth > 0)
        tx2[tx2qp].extra = 0xff00;
    else {
        tx2[tx2qp].extra = 0;
        if (curNodeType == T2_IDENTIFIER) {
            info = FromIdx(tx2[tx2qp].left);
            OptimiseNodes((info->flag & F_AT) ? bC259 : bC25A);
        } else
            OptimiseNodes(bC25A);
    }
}

void Sub_6BD6() {
    bC259 = 4;
    bC25A = 4;
    for (tx2qp = 4; tx2qp <= tx2qNxt - 1; tx2qp++) {
        tx2[tx2qp].cnt = 0;
        curNodeType          = tx2[tx2qp].nodeType;
        nodeControlFlags          = nodeControlMap[curNodeType];
        if ((nodeControlFlags & 0xc0) == 0)
            // LT LE NE EQ GE GT ROL ROR SCL SCR SHL SHR JMPFALSE DOUBLE PLUSSIGN MINUSSIGN STAR
            // SLASH MOD AND OR XOR BASED BYTEINDEX WORDINDEX MEMBER UNARYMINUS NOT LOW HIGH
            // ADDRESSOF PLUS MINUS TIME STKBARG STKWARG DEC COLONEQUALS OUTPUT CASEBLOCK STKARG
            // MOVE RETURNBYTE RETURNWORD RETURN ADDW BEGMOVE CALL CALLVAR SETADDR
            Sub_6D52();
        else if ((nodeControlFlags & 0xc0) == 0x40)
            Sub_7111();
        if (curNodeType == T2_BEGCALL || curNodeType == T2_BEGMOVE)
            procCallDepth++;
    }
}
