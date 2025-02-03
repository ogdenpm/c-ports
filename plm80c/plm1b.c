/****************************************************************************
 *  plm1b.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

void GetTx1Item() {
    if (regetTx1Item) {
        regetTx1Item = false;
        return;
    }
    while (1) {
        RdTx1Item();
        if (tx1Item.type == T1_TOKENERROR) {
            if ((curSym = symtab + tx1Item.dataw[1])) {     // check we have symbol info
                if (curSym->infoChain >= infotab + MAXINFO) // is it a keyword?
                    curSym->infoChain = NULL;               // reset info link
                if ((info = curSym->infoChain) == NULL)
                    CreateInfo(0, UNK_T, curSym); // allocate an UNK_T info block
                tx1Item.dataw[1] = ToIdx(info);   // replace sym ref with info offset
            }
            MapLToT2();
        } else if ((tx1Item.type == T1_XREFUSE && XREF) ||
                   (tx1Item.type == T1_XREFDEF && (IXREF || XREF || SYMBOLS))) {
            // write the T1_XREFUSE/T1_XREFDEF infoP and stmtNum to xrfFile
            info_t *xrInfo = FromIdx(tx1Item.dataw[0]);
            xrInfo->xref   = newXref(xrInfo->xref, tx1Item.type == T1_XREFDEF
                                                       ? -curStmtNum
                                                       : curStmtNum); /* make defn line -ve */
        } else if (tx1Item.type == T1_LINEINFO) {
            if (tx2LinfoPending) // write any pending lInfo
                Wr2Item(linfo.type, &linfo.lineCnt, sizeof(struct _linfo));
            memcpy(&linfo.lineCnt, tx1Item.dataw, sizeof(struct _linfo));
            linfo.type      = T2_LINEINFO;          // update T2 code
            tx2LinfoPending = true;                 // flag as pending
        } else if ((tx1Attr & F_PASSTHROUGH)) // pass through
            MapLToT2();
        else if (tx1Item.type == T1_STMTCNT && tx1Item.dataw[0] == 0) // pass through null stmt cnts
            MapLToT2();
        else if (tx1Item.type != T1_XREFDEF &&
                 tx1Item.type != T1_XREFUSE) // break if not handled (or ignored)
            break;
    }
    if (tx1Item.type == T1_IDENTIFIER) // set symbolP up
        curSym = symtab + tx1Item.dataw[0];
}

bool MatchTx1Item(byte type) // check for requested Lex item. If present return true else return
                             // false and don't consume item
{
    GetTx1Item();
    if (tx1Item.type == type)
        return true;
    else {
        UngetTx1Item();
        return false;
    }
}

bool NotMatchTx1Item(byte type) /// check for requested Lex item. If present consume item and
                                 /// return false else return true and don't consume item
{
    return !MatchTx1Item(type);
}

bool MatchTx2AuxFlag(byte flag) {
    GetTx1Item();
    if ((tx1Attr & flag))
        return true;
    else {
        UngetTx1Item();
        return false;
    }
}

void RecoverRPOrEndExpr() {
    do {
        GetTx1Item();
    } while ((tx1Attr & F_EXPRITEM) && tx1Item.type != T1_RPAREN);
    UngetTx1Item();
}

void ResyncRParen() {
    RecoverRPOrEndExpr();
    MatchTx1Item(T1_RPAREN); // consume the RP if seen
}

void ExpectRParen(byte arg1b) {
    if (NotMatchTx1Item(T1_RPAREN)) {
        Wr2TokError(arg1b);
        ResyncRParen();
    }
}

void ChkIdentifier() {

    if (!FindInfo() || info->type == LIT_T) // doesn't exist or appeared in recursive LIT!!
        CreateInfo(0x100, BYTE_T, curSym);  // assume a var was declared
    OptWrXrf();
    // check if declared, labels ok since forward ref allowed
    if (info->type != BUILTIN_T && !(info->flag & F_LABEL) && !(info->flag & F_DECLARED)) {
        Wr2TokError(105);         /* UNDECLARED IDENTIFIER */
        info->flag |= F_DECLARED; // flag as now declared
    }
}

void ChkStructureMember() {

    info_t *parent = info;             // save parent info
    FindMemberInfo();                  // get the member info
    if (!info) {                       // oops not there
        CreateInfo(0, BYTE_T, curSym); // create a member to allow compiler to continue
        info->parent = parent;
        info->flag |= F_MEMBER;
    }
    if (!(info->flag & F_LABEL) && !(info->flag & F_DECLARED)) { // warn once if not declared
        Wr2TokError(112);                                        /* UNDECLARED STRUCTURE MEMBER */
        info->flag |= F_DECLARED;
    }
    OptWrXrf();
}

void GetVariable() {
    ChkIdentifier();
    if (MatchTx1Item(T1_PERIOD)) {
        if (info->type != STRUCT_T)
            Wr2TokError(110); /* INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE */
        else if (NotMatchTx1Item(T1_IDENTIFIER))
            Wr2TokError(111); /* INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER */
        else
            ChkStructureMember();
    }
}

void WrAtBuf(void const *buf, word cnt) {
    vfWbuf(&atf, buf, cnt);
}

void WrAtByte(byte arg1b) {
    vfWbyte(&atf, arg1b);
}

void WrAtWord(word arg1w) {
    vfWword(&atf, arg1w);
}
