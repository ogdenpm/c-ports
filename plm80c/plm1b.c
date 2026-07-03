/****************************************************************************
 *  plm1b.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
 * plm1b.c — Phase-1 lexical-stream reader and expression-parse helpers
 *            (compiler phase 2 support)
 *
 * Overview
 * --------
 * This module bridges the phase-1 lexical stream (tx1 file) and phase-2
 * intermediate code (tx2 file).  It provides:
 *
 *  • GetTx1Item() / MatchTx1Item() / NotMatchTx1Item() / MatchTx2AuxFlag()
 *    – the token-consumption interface used by all phase-2 parser routines.
 *
 *  • Transparent handling of bookkeeping tokens that must be processed
 *    inline (T1_LINEINFO, T1_XREFUSE, T1_XREFDEF, pass-through tokens,
 *    null T1_STMTCNT records) so callers never see them.
 *
 *  • T1_TOKENERROR recovery: unknown identifiers encountered during phase 1
 *    are given a synthetic UNK_T info entry so phase 2 can continue.
 *
 *  • Identifier and structure-member resolution: ChkIdentifier() and
 *    ChkStructureMember() validate declared status and emit cross-reference
 *    records; GetVariable() combines them to handle qualified names
 *    (STRUCT.MEMBER).
 *
 *  • Error-recovery helpers that resync to a closing ')' or end of
 *    expression when a malformed sub-expression is detected.
 *
 *  • WrAtBuf() / WrAtByte() / WrAtWord() – thin wrappers that write to the
 *    attribute file (atf) used by later phases.
 *
 * Token stream model
 * ------------------
 * Tokens are read from the tx1 file via RdTx1Item() into the global
 * tx1Item structure.  The attribute word tx1Attr (set by RdTx1Item /
 * MapLToT2) carries flag bits (F_PASSTHROUGH, F_EXPRITEM, …) that classify
 * each token for dispatch.  UngetTx1Item() / regetTx1Item allow one-token
 * lookahead.
 */

#include "plm.h"

/*
 * GetTx1Item – read the next semantically significant token from the
 *              phase-1 stream into tx1Item.
 *
 * Transparent tokens are handled and consumed in a loop so the caller
 * always receives an actionable item:
 *
 *  T1_TOKENERROR   – an identifier that phase 1 could not resolve.
 *                    The symbol's infoChain is sanitised (keyword links are
 *                    cleared) and a UNK_T info entry is created if none
 *                    exists.  The item's dataw[1] is rewritten to the info
 *                    index, then MapLToT2() passes the token to phase 2.
 *                    Loop continues.
 *
 *  T1_XREFUSE      – recorded in the xref list when XREF is enabled.
 *                    Loop continues.
 *
 *  T1_XREFDEF      – recorded in the xref list when IXREF, XREF or
 *                    SYMBOLS is enabled.  Definition references are stored
 *                    as negative statement numbers (−curStmtNum).
 *                    Loop continues.
 *
 *  T1_LINEINFO     – updates the pending linfo record.  Any previously
 *                    pending linfo is flushed to the tx2 file first.
 *                    tx2LinfoPending is set so the record is written before
 *                    the next non-lineinfo item.  Loop continues.
 *
 *  F_PASSTHROUGH   – the token is mapped directly to tx2 via MapLToT2().
 *                    Loop continues.
 *
 *  T1_STMTCNT(0)   – null statement-count tokens are passed through to tx2.
 *                    Loop continues.
 *
 *  T1_XREFDEF /    – any remaining xref tokens not matched above are
 *  T1_XREFUSE        silently ignored.  Loop continues.
 *
 *  anything else   – breaks the loop; the token is returned to the caller.
 *
 * After the loop, if the item is T1_IDENTIFIER, curSym is set to
 * symtab + tx1Item.dataw[0].
 *
 * If regetTx1Item is true on entry the previous item is re-presented
 * without calling RdTx1Item() (one-item unget).
 */
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
            linfo.type      = T2_LINEINFO;    // update T2 code
            tx2LinfoPending = true;           // flag as pending
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

/*
 * MatchTx1Item – consume the next token if it matches type.
 *
 * Calls GetTx1Item().  Returns true and leaves the item consumed if
 * tx1Item.type == type.  Otherwise calls UngetTx1Item() and returns false.
 */
bool MatchTx1Item(uint8_t type) {
    GetTx1Item();
    if (tx1Item.type == type)
        return true;
    else {
        UngetTx1Item();
        return false;
    }
}

/*
 * NotMatchTx1Item – consume the next token if it does NOT match type.
 *
 * Returns true (and leaves the item un-consumed) when the next token does
 * not match type; returns false (item consumed) when it does match.
 * Logical inverse of MatchTx1Item().
 */
bool NotMatchTx1Item(uint8_t type) {
    return !MatchTx1Item(type);
}

/*
 * MatchTx2AuxFlag – consume the next token if its attribute flags include flag.
 *
 * Calls GetTx1Item() and tests tx1Attr against flag.  Returns true and
 * leaves the item consumed on a match.  Otherwise calls UngetTx1Item() and
 * returns false.  Used to dispatch on token categories (e.g. F_EXPRITEM)
 * rather than specific token types.
 */
bool MatchTx2AuxFlag(uint8_t flag) {
    GetTx1Item();
    if ((tx1Attr & flag))
        return true;
    else {
        UngetTx1Item();
        return false;
    }
}

/*
 * RecoverRPOrEndExpr – skip tokens until ')' or end of expression.
 *
 * Consumes tokens while the F_EXPRITEM flag is set and the token is not
 * T1_RPAREN, then un-gets the terminating token so the caller can decide
 * whether to consume it.  Used for error recovery inside sub-expressions.
 */
void RecoverRPOrEndExpr() {
    do {
        GetTx1Item();
    } while ((tx1Attr & F_EXPRITEM) && tx1Item.type != T1_RPAREN);
    UngetTx1Item();
}

/*
 * ResyncRParen – recover to a closing ')'.
 *
 * Calls RecoverRPOrEndExpr() to advance to a ')' or expression boundary,
 * then consumes the ')' if one is present.  Used after a malformed
 * parenthesised sub-expression to leave the stream in a consistent state.
 */
void ResyncRParen() {
    RecoverRPOrEndExpr();
    MatchTx1Item(T1_RPAREN); // consume the RP if seen
}

/*
 * ExpectRParen – require a closing ')'; report error and resync if absent.
 *
 * Attempts to match T1_RPAREN.  If not present, emits error arg1b via
 * Wr2TokError() and calls ResyncRParen() to restore the token stream to a
 * known state.
 */
void ExpectRParen(uint8_t arg1b) {
    if (NotMatchTx1Item(T1_RPAREN)) {
        Wr2TokError(arg1b);
        ResyncRParen();
    }
}

/*
 * ChkIdentifier – validate and resolve the current identifier (curSym).
 *
 * Calls FindInfo() to locate the symbol's info entry at the current scope.
 * If the symbol is not found, or if it refers to a LIT_T (recursive literal
 * expansion), a BYTE_T info entry is synthesised at scope 0x100 to allow
 * compilation to continue.
 *
 * Calls OptWrXrf() to emit a cross-reference record if required.
 *
 * Raises ERR105 (UNDECLARED IDENTIFIER) once per symbol if it is not a
 * built-in and neither F_LABEL nor F_DECLARED is set; F_DECLARED is then
 * set to suppress duplicate diagnostics.
 */
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

/*
 * ChkStructureMember – validate the current identifier as a structure member.
 *
 * Saves the parent structure's info pointer, then calls FindMemberInfo() to
 * locate the member within that structure.  If the member does not exist, a
 * synthetic BYTE_T entry is created, linked to the parent and marked
 * F_MEMBER, so compilation can continue.
 *
 * Raises ERR112 (UNDECLARED STRUCTURE MEMBER) once per member if F_LABEL
 * and F_DECLARED are both absent; F_DECLARED is set to suppress repeats.
 * Calls OptWrXrf() to emit a cross-reference record if required.
 */
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

/*
 * GetVariable – parse a (possibly qualified) variable reference.
 *
 * Calls ChkIdentifier() to validate the leading identifier.  If a '.'
 * follows (T1_PERIOD), the reference is a structure qualification:
 *   • ERR110 is raised if the leading variable is not a STRUCT_T.
 *   • ERR111 is raised if no identifier follows the '.'.
 *   • Otherwise ChkStructureMember() validates the member name.
 * On return, info points to the innermost resolved info entry (the member
 * for a qualified reference, the variable itself otherwise).
 */
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

/*
 * WrAtBuf – write a raw byte buffer to the attribute file (atf).
 *
 * Thin wrapper around vfWbuf().  cnt bytes starting at buf are appended to
 * atf for consumption by later compiler phases.
 */
void WrAtBuf(void const *buf, uint16_t cnt) {
    vfWbuf(&atf, buf, cnt);
}

/*
 * WrAtByte – write a single byte to the attribute file (atf).
 */
void WrAtByte(uint8_t arg1b) {
    vfWbyte(&atf, arg1b);
}

/*
 * WrAtWord – write a 16-bit word to the attribute file (atf).
 */
void WrAtWord(uint16_t arg1w) {
    vfWword(&atf, arg1w);
}