/****************************************************************************
 *  plm1b.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

void GetTx1Item() {
    if (regetTx1Item) {
        regetTx1Item = false;
        return;
    }
    while (1) {
        RdTx1Item();
        if (tx1Item.type == L_TOKENERROR) {
            if ((curSym = tx1Item.dataw[1])) {       // check we have symbol info
                if (symtab[curSym].infoIdx >= 0xff00) // is it a keyword?
                    symtab[curSym].infoIdx = 0;           // reset info link
                if ((infoIdx = symtab[curSym].infoIdx) == 0)
                    CreateInfo(0, UNK_T, curSym); // allocate an UNK_T info block
                tx1Item.dataw[1] = infoIdx;       // replace sym ref with info offset
            }
            MapLToT2();
        } else if ((tx1Item.type == L_XREFUSE && XREF) ||
                   (tx1Item.type == L_XREFDEF && (IXREF || XREF || SYMBOLS))) {
            tx1Item.dataw[1] = curStmtNum;
            vfWbyte(&xrff,
                    tx1Item.type); // write the L_XREFUSE/L_XREFDEF infoP and stmtNum to xrfFile
            vfWword(&xrff, tx1Item.dataw[0]);
            vfWword(&xrff, tx1Item.dataw[1]);
        } else if (tx1Item.type == L_LINEINFO) {
            if (tx2LinfoPending) // write any pending lInfo
                Wr2Item(linfo.type, &linfo.lineCnt, sizeof(struct _linfo));
            memcpy(&linfo.lineCnt, tx1Item.dataw, sizeof(struct _linfo));
            linfo.type      = T2_LINEINFO; // update T2 code
            tx2LinfoPending = true;        // flag as pending
        } else if ((tx1Aux2 & 0x20) != 0)  // pass through
            MapLToT2();
        else if (tx1Item.type == L_STMTCNT && tx1Item.dataw[0] == 0) // pass through null stmt cnts
            MapLToT2();
        else if (tx1Item.type != L_XREFDEF &&
                 tx1Item.type != L_XREFUSE) // break if not handled (or ignored)
            break;
    }
    if (tx1Item.type == L_IDENTIFIER) // set symbolP up
        curSym = tx1Item.dataw[0];
}

bool MatchTx1Item(byte arg1b) // check for requested Lex item. If present return true else return
                              // false and don't consume item
{
    GetTx1Item();
    if (tx1Item.type == arg1b)
        return true;
    else {
        SetRegetTx1Item();
        return false;
    }
}

bool NotMatchTx1Item(byte arg1b) /// check for requested Lex item. If present consume item and
                                 /// return false else return true and don't consume item
{
    return !MatchTx1Item(arg1b);
}

bool MatchTx2AuxFlag(byte arg1b) {
    GetTx1Item();
    if ((tx1Aux2 & arg1b) != 0)
        return true;
    else {
        SetRegetTx1Item();
        return false;
    }
}

void RecoverRPOrEndExpr() {
    do {
        GetTx1Item();
    } while ((tx1Aux2 & 0x80) && tx1Item.type != L_RPAREN);
    SetRegetTx1Item();
}

void ResyncRParen() {
    RecoverRPOrEndExpr();
    MatchTx1Item(L_RPAREN); // consume the RP if seen
}

void ExpectRParen(byte arg1b) {
    if (NotMatchTx1Item(L_RPAREN)) {
        WrTx2ExtError(arg1b);
        ResyncRParen();
    }
}

void ChkIdentifier() {

    if (!FindInfo() || info->type == LIT_T) // doesn't exist or appeared in recursive LIT!!
        CreateInfo(0x100, BYTE_T, curSym);  // assume a var was declared
    OptWrXrf();
    // check if declared, labels ok since forward ref allowed
    if (info->type != BUILTIN_T && !(info->flag & F_LABEL) && !(info->flag & F_DECLARED)) {
        WrTx2ExtError(105);       /* UNDECLARED IDENTIFIER */
        info->flag |= F_DECLARED; // flag as now declared
    }
}

void ChkStructureMember() {
    offset_t parent;

    parent = infoIdx;                  // save parent info
    FindMemberInfo();                  // get the member info
    if (infoIdx == 0) {                // oops not there
        CreateInfo(0, BYTE_T, curSym); // create a member to allow compiler to continue
        info->parent = parent;
        info->flag |= F_MEMBER;
    }
    if (!(info->flag & F_LABEL) && !(info->flag & F_DECLARED)) { // warn once if not declared
        WrTx2ExtError(112);                                      /* UNDECLARED STRUCTURE MEMBER */
        info->flag |= F_DECLARED;
    }
    OptWrXrf();
}

void GetVariable() {
    ChkIdentifier();
    if (MatchTx1Item(L_PERIOD)) {
        if (info->type != STRUCT_T)
            WrTx2ExtError(110); /* INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE */
        else if (NotMatchTx1Item(L_IDENTIFIER))
            WrTx2ExtError(111); /* INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER */
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
