/****************************************************************************
 *  plm1d.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
 * plm1d.c — PL/M-80 expression parser (phase-2 state machine)
 *
 * Overview
 * --------
 * This module implements the expression parser for compiler phase 2.  Like
 * the program parser in plm0f.c, it is driven by an explicit parse stack
 * (parseStack[]) rather than recursive descent.  The public entry point is
 * ExpressionStateMachine(), which loops popping and dispatching parse states
 * until the stack is empty.
 *
 * The parser implements a shift-reduce operator-precedence scheme layered
 * over the state machine:
 *
 *  • Operands are pushed onto operandStack[] as X_xxx intermediate-code
 *    nodes via PushSimpleOperand() / PushComplexOperand().
 *  • Operators are pushed onto operatorStack[] via PushOperator() and
 *    reduced by ApplyOperator() when a lower-or-equal precedence operator
 *    is seen (ExprParse3).
 *  • An X_MARKER sentinel is pushed at the start of each expression to
 *    bound the reduce loop.
 *
 * Grammar and state mapping
 * -------------------------
 *  State constant   Handler         Grammar element / action
 *  --------------   -------         ------------------------
 *  E_EXPRESSION     ExprParse0      <expression>  – push marker, start
 *  E_EMBED          ExprParse1      check for embedded := assignment
 *  E_COLONEQUALS    ExprParse2      emit X_COLONEQUALS node
 *  E_OPERATOR       ExprParse3      shift / reduce binary operator
 *  E_UNARY          ExprParse4      optional unary '-' or NOT, then primary
 *  E_PRIMARY        ExprParse5      <primary> – literal, identifier, '(' …
 *  E_RPAREN         ExprParse6      closing ')' of sub-expression
 *  E_ADDRESSOF      ExprParse7      emit X_ADDRESSOF node
 *  E_LOCREF         ExprParse8      <location reference> after '.'
 *  E_DOTVAR         ExprParse9      <. variable reference> incl. subscript
 *  E_MEMBERSPEC     ExprParse10     <member specifier> after '.'
 *  E_VARREF         ExprParse11     <variable reference> → builtin/proc/var
 *  E_DATAREF        ExprParse12     <data reference> with array/subscript
 *  E_DOT            ExprParse13     member reference via '.'
 *  E_MEMBER         ExprParse14     emit X_MEMBER node, fixup based
 *  E_ARGLIST        ExprParse15     <argument list> for calls
 *  E_FUNCTION       ExprParse16     reduce function call to X_CALL node
 *  E_BUILTIN        ExprParse17     reduce built-in call node
 *  E_CALLVAR        ExprParse18     reduce indirect call (not used in PL/M)
 *  E_SUBSCRIPT      ExprParse19     <subscript expression>
 *  E_ENDSUBSCRIPT   ExprParse20     end of subscript, build index node
 *
 * Error handling
 * --------------
 * Syntax errors are reported via Wr2TokError() with specific ERR codes.
 * After an error the parser either inserts a synthetic X_NUMBER(0) operand
 * or calls ResyncRParen() / RecoverRPOrEndExpr() to resynchronise the token
 * stream before resuming.
 */

#include "plm.h"

/*
 * ExprParse0 – begin parsing an <expression>.
 *
 * Pushes an X_MARKER sentinel onto the operator stack to bound the
 * subsequent shift-reduce loop.  Then peeks at the next token:
 *   • T1_IDENTIFIER → push E_EMBED (check for ':=') and E_VARREF after
 *                     calling ChkIdentifier() to resolve the symbol.
 *   • anything else → push E_OPERATOR (reduce) and E_UNARY (optional
 *                     unary operator followed by a primary).
 */
static void ExprParse0() {
    PushOperator(X_MARKER);            // put end marker
    if (MatchTx1Item(T1_IDENTIFIER)) { // starts with identifier
        PushParse(E_EMBED);
        ChkIdentifier();
        PushParse(E_VARREF);
    } else {                   // should be NOT or '-'
        PushParse(E_OPERATOR); // shift / reduce operator
        PushParse(E_UNARY);    // <unaryOp>? <primary>
    }
}

/*
 * ExprParse1 – check for an embedded ':=' assignment.
 *
 * If ':=' follows the leading identifier, the current operand must be an
 * l-value (IsLValue); ERR128 is raised otherwise and the operand is
 * discarded.  E_COLONEQUALS is pushed to emit the assignment node, followed
 * by E_OPERATOR and E_UNARY to parse the right-hand side.
 *
 * If ':=' is absent and the operand is an OUTPUT function reference,
 * ERR130 is raised (OUTPUT cannot appear as an r-value), the bad operand is
 * replaced with X_NUMBER(0), and E_OPERATOR is pushed to continue.
 */
static void ExprParse1() {
    if (MatchTx1Item(T1_COLONEQUALS)) {
        if (!IsLValue(operandSP)) {
            Wr2TokError(ERR128); /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
            PopOperand();        // junk the assignment
        } else
            PushParse(E_COLONEQUALS);
        PushParse(E_OPERATOR); // shift / reduce operator
        PushParse(E_UNARY);    // <unaryOp>? <primary>
    } else {
        if (operandStack[operandSP].icode == X_OUTPUT) {
            Wr2Error(ERR130); /*  ILLEGAL REFERENCE to OUTPUT FUNCTION */
            PopOperand();
            PushSimpleOperand(X_NUMBER, 0);
        }
        PushParse(E_OPERATOR);
    }
}

/*
 * ExprParse2 – emit the X_COLONEQUALS assignment node.
 *
 * Called after the right-hand side of an embedded ':=' has been parsed.
 * Combines the two top operands (target and value) into an X_COLONEQUALS
 * node with arity 2.
 */
static void ExprParse2() { // := ...
    PushComplexOperand(X_COLONEQUALS, 2);
}

/*
 * ExprParse3 – shift/reduce binary operator.
 *
 * Peeks at the next token via MatchTx2AuxFlag(F_BINOP):
 *
 * Binary operator present:
 *   Reduce all stacked operators whose precedence is >= the new operator's
 *   precedence (standard operator-precedence parsing).  ERR218 is raised if
 *   two relational operators appear consecutively (both have precedence 50).
 *   The new operator is then pushed and E_OPERATOR / E_UNARY are queued to
 *   process the next operand.
 *
 * No binary operator:
 *   Reduce all remaining operators down to (but not including) the X_MARKER
 *   sentinel, then pop the marker.  This ends the expression.
 */
static void ExprParse3() {
    if (MatchTx2AuxFlag(F_BINOP)) { // binary operator
        while (precedence[tx1XCode] <= precedence[operatorStack[operatorSP]]) {
            if (precedence[tx1XCode] == 50 && precedence[operatorStack[operatorSP]] == 50)
                Wr2TokError(ERR218); /* ILLEGAL SUCCESSIVE USES OF RELATIONAL OPERATORS */
            ApplyOperator();         // reduce previous operators
        }
        PushOperator(tx1XCode); // add this new operator
        PushParse(E_OPERATOR);
        PushParse(E_UNARY); // parse <unaryOp>? <primary>
    } else {
        while (operatorStack[operatorSP] != X_MARKER)
            ApplyOperator(); // reduce remaining operators
        PopOperator();       // pop marker
    }
}

/*
 * ExprParse4 – parse an optional unary operator followed by a primary.
 *
 * If '-' is the next token, pushes X_UNARYMINUS onto the operator stack.
 * If NOT is the next token, pushes X_NOT.
 * In all cases E_PRIMARY is pushed to parse the primary operand; the unary
 * operator (if any) will be reduced by the next ExprParse3 call.
 */
static void ExprParse4() {
    if (MatchTx1Item(T1_MINUSSIGN)) // starts with - so convert to unary minus
        PushOperator(X_UNARYMINUS);
    else if (MatchTx1Item(T1_NOT)) // starts with NOT
        PushOperator(X_NOT);
    PushParse(E_PRIMARY); // <primary>
}

/*
 * ExprParse5 – parse a <primary> operand.
 *
 * Dispatches on the next token:
 *
 *  T1_IDENTIFIER  – resolve via ChkIdentifier(); if the identifier is the
 *                   OUTPUT built-in, report ERR130 and push X_NUMBER(0).
 *                   Otherwise push E_VARREF.
 *
 *  T1_NUMBER      – push X_NUMBER with the literal value.
 *
 *  T1_STRING      – convert a 1- or 2-character string constant to a
 *                   numeric value (ERR100 for longer strings) and push
 *                   X_NUMBER.  Two-character strings are stored high-byte
 *                   first; the bytes are swapped to produce the PL/M word.
 *
 *  T1_LPAREN      – push E_RPAREN and E_EXPRESSION for a parenthesised
 *                   sub-expression.
 *
 *  T1_PERIOD      – address-of operator '.':
 *                     '.identifier' → push E_ADDRESSOF and E_LOCREF.
 *                     '.(…)'        → call ConstantList(), push E_ADDRESSOF.
 *                     anything else → ERR101, re-try as primary.
 *
 *  (other)        – ERR102 (MISSING PRIMARY OPERAND); push X_NUMBER(0).
 */
static void ExprParse5() {
    if (MatchTx1Item(T1_IDENTIFIER)) {
        ChkIdentifier();
        if (info->type == BUILTIN_T && info->builtinId == 9) { /* 9 -> OUTPUT */
            Wr2TokError(ERR130);         /* ILLEGAL REFERENCE TO OUTPUT FUNCTION */
            if (MatchTx1Item(T1_LPAREN)) // skip any (...)
                ResyncRParen();
            PushSimpleOperand(X_NUMBER, 0); // error so assume 0
        } else
            PushParse(E_VARREF);
    } else if (MatchTx1Item(T1_NUMBER))
        PushSimpleOperand(X_NUMBER, tx1Item.dataw[0]); // push X_NUMBER & value
    else if (MatchTx1Item(T1_STRING)) {
        uint16_t val;
        if (tx1Item.dataw[0] == 1)
            val = Low(tx1Item.dataw[1]);                             // single char
        else if (tx1Item.dataw[0] == 2)                              // double char
            val = (tx1Item.dataw[1] >> 8) + (tx1Item.dataw[1] << 8); // swap bytes;
        else {
            Wr2TokError(ERR100); /* INVALID STRING CONSTANT IN EXPRESSION  */
            val = 0;             // error so use 0
        }
        PushSimpleOperand(X_NUMBER, val); // push X_NUMBER & string constant value
    } else if (MatchTx1Item(T1_LPAREN)) {
        PushParse(E_RPAREN);                  // closing ')' of subexpression
        PushParse(E_EXPRESSION);              // <expression>
    } else if (MatchTx1Item(T1_PERIOD)) {     // addressof
        if (MatchTx1Item(T1_IDENTIFIER)) {    // <location reference> := .<variable reference>
            PushParse(E_ADDRESSOF);           // <location reference> make addressof node
            PushParse(E_LOCREF);              // <variable reference>
        } else if (MatchTx1Item(T1_LPAREN)) { // <location reference> := .<constant list>
            ConstantList();                   // collect the constant list
            PushParse(E_ADDRESSOF);           // <location reference> make addressof node
        } else {
            Wr2TokError(ERR101);  /* INVALID ITEM FOLLOWS DOT OPERATOR */
            PushParse(E_PRIMARY); // <primary>
        }
    } else {
        Wr2TokError(ERR102);            /* MISSING PRIMARY OPERAND */
        PushSimpleOperand(X_NUMBER, 0); // assume a 0
    }
}

/*
 * ExprParse6 – consume the closing ')' of a parenthesised sub-expression.
 *
 * Calls ExpectRParen(ERR103); error and resync if ')' is absent.
 */
static void ExprParse6() {
    ExpectRParen(ERR103); /* MISSING ') ' AT END OF SUBEXPRESSION */
}

/*
 * ExprParse7 – emit an X_ADDRESSOF node.
 *
 * Called after the location reference has been fully parsed.  Wraps the top
 * operand into an X_ADDRESSOF node with arity 1.
 */
static void ExprParse7() {
    PushComplexOperand(X_ADDRESSOF, 1);
}

/*
 * ExprParse8 – parse a <location reference> (the operand of '.').
 *
 * Resolves the current identifier via ChkIdentifier().
 *
 *  Built-in  → ERR123 (built-in illegal after '.'); skip any '(…)', push
 *              X_NUMBER(0).
 *  PROC_T    → record X_IDENTIFIER operand; if '(' follows, ERR104 (illegal
 *              procedure invocation with '.') and ResyncRParen().
 *  LABEL_T   → record X_IDENTIFIER operand; ERR158 (label illegal after
 *              '.'); fall through to E_DOTVAR.
 *  other     → record X_IDENTIFIER operand and push E_DOTVAR to handle
 *              optional subscript and member qualifier.
 */
static void ExprParse8() {
    ChkIdentifier();
    if (info->type == BUILTIN_T) { // can't be a builtin
        PopParse();
        Wr2TokError(ERR123); /* INVALID DOT OPERAND, BUILT-IN PROCEDURE ILLEGAL */
        if (MatchTx1Item(T1_LPAREN))
            ResyncRParen();
        PushSimpleOperand(X_NUMBER, 0); // assume a 0
    } else {
        PushSimpleOperand(X_IDENTIFIER, ToIdx(info)); // record the variable
        if (info->type == PROC_T) {                   // if proc then can't have (...) invocation
            if (MatchTx1Item(T1_LPAREN)) {
                Wr2TokError(ERR104); /* ILLEGAL PROCEDURE INVOCATION WITH DOT OPERATOR */
                ResyncRParen();      // skip invocation
            }
        } else { // labels are also bad
            if (info->type == LABEL_T)
                Wr2TokError(ERR158); /* INVALID DOT OPERAND, LABEL ILLEGAL */
            PushParse(E_DOTVAR);     // <. variable reference>
        }
    }
}

/*
 * ExprParse9 – parse a <. variable reference> (subscript and/or member).
 *
 * Saves the current info index on the parse stack for later use by
 * ExprParse10 (member specifier fixup).  Pushes E_MEMBERSPEC to handle an
 * optional '.' qualifier.  If '(' follows and the variable is an array,
 * pushes E_SUBSCRIPT; ERR127 is raised if '(' is present but the variable
 * is not an array.
 */
static void ExprParse9() {
    PushParse(ToIdx(info));        // save name info
    PushParse(E_MEMBERSPEC);       // <member specifier>
    if (MatchTx1Item(T1_LPAREN)) { // start of <subscript>
        if (!(info->flag & F_ARRAY))
            Wr2TokError(ERR127); /* INVALID SUBSCRIPT ON NON-ARRAY */
        PushParse(E_SUBSCRIPT);  // <subscript>
    }
}

/*
 * ExprParse10 – parse a <member specifier> (optional '.' qualification).
 *
 * Pops the saved info index pushed by ExprParse9.
 *
 * If '.' follows:
 *   • The saved info must be STRUCT_T (ERR110 otherwise).
 *   • An identifier must follow (ERR111 otherwise).
 *   • ChkStructureMember() resolves the member; X_IDENTIFIER for the member
 *     is pushed, the parent info and E_MEMBER are queued.
 *   • If the member is an array, E_SUBSCRIPT is pushed if '(' follows.
 *
 * If no '.' follows, FixupBased() is called to handle BASED variable
 * pointer indirection for the saved info entry.
 */
static void ExprParse10() {
    info_t *nameInfo = FromIdx(PopParse());

    if (MatchTx1Item(T1_PERIOD)) { // start of <member specifier>
        info = nameInfo;
        if (info->type != STRUCT_T)
            Wr2TokError(ERR110); /* INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE */
        else if (NotMatchTx1Item(T1_IDENTIFIER))
            Wr2TokError(ERR111); /* INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER */
        else {
            ChkStructureMember();
            PushSimpleOperand(X_IDENTIFIER, ToIdx(info));
            PushParse(ToIdx(nameInfo)); // member info
            PushParse(E_MEMBER);        // make member node
            if (MatchTx1Item(T1_LPAREN))
                PushParse(E_SUBSCRIPT); // <subscript>
        }
    } else
        FixupBased(nameInfo);
}

/*
 * ExprParse11 – dispatch a <variable reference> to the appropriate handler.
 *
 * Examines info->type to distinguish three cases:
 *
 *  BUILTIN_T  – maps the built-in ID to its X_xxx icode via builtinsMap[]:
 *                 X_INPUT / X_OUTPUT  → ParsePortNum() to parse port number.
 *                 X_LENGTH / X_LAST / X_SIZE → ParseSizing() for array info.
 *                 other built-ins     → push icode, expected arg count,
 *                                       actual arg count (0), E_BUILTIN, and
 *                                       ChkTypedProcedure().  If '(' follows,
 *                                       push E_ARGLIST and E_EXPRESSION.
 *
 *  PROC_T     – ChkIllegalCall() validates the call context.  Push
 *               X_IDENTIFIER, ChkTypedProcedure(), expected/actual arg
 *               counts, E_FUNCTION.  If '(' follows, push E_ARGLIST and
 *               E_EXPRESSION.
 *
 *  other      – push E_DATAREF for a plain data variable reference.
 */
static void ExprParse11() {
    if (info->type == BUILTIN_T) {
        uint16_t icode = builtinsMap[info->builtinId];
        if (icode == X_INPUT || icode == X_OUTPUT)
            ParsePortNum((uint8_t)icode);
        else if (icode == X_LENGTH || icode == X_LAST || icode == X_SIZE)
            ParseSizing((uint8_t)icode);
        else {
            PushParse(icode);          // builtIn icode
            PushParse(info->paramCnt); // expected number of args
            PushOperator(0);           // actual number of args
            PushParse(E_BUILTIN);
            ChkTypedProcedure();
            if (MatchTx1Item(T1_LPAREN)) {
                PushParse(E_ARGLIST);    // <arg list>
                PushParse(E_EXPRESSION); // <expression>
            }
        }
    } else if (info->type == PROC_T) {
        ChkIllegalCall();
        PushSimpleOperand(X_IDENTIFIER, ToIdx(info));
        ChkTypedProcedure();
        PushParse(info->paramCnt); // expected arg cnt
        PushOperator(0);           // actual arg cnt
        PushParse(E_FUNCTION);
        if (MatchTx1Item(T1_LPAREN)) {
            PushParse(E_ARGLIST);
            PushParse(E_EXPRESSION); // <expression>
        }
    } else
        PushParse(E_DATAREF);
}

/*
 * ExprParse12 – parse a <data reference> (plain variable or array element).
 *
 * Pushes X_IDENTIFIER for the variable and saves the info index for the
 * subsequent member/based check (E_DOT).
 *
 * ERR132 is raised if the variable is a LABEL_T (labels cannot be used as
 * data).
 *
 * Array variables:
 *   F_ARRAY set and '(' present → push E_SUBSCRIPT.
 *   F_ARRAY set but no '('      → ERR133 (unsubscripted array reference).
 *
 * Non-array variables:
 *   '(' present → ERR127 (subscript on non-array), push E_SUBSCRIPT anyway
 *                 to consume the spurious argument list.
 */
static void ExprParse12() {
    PushSimpleOperand(X_IDENTIFIER, ToIdx(info));
    PushParse(ToIdx(info)); // for member access and based fixup
    if (info->type == LABEL_T)
        Wr2TokError(ERR132);      /* ILLEGAL USE OF label */
    PushParse(E_DOT);             // member check
    if ((info->flag & F_ARRAY)) { // check for subscript
        if (MatchTx1Item(T1_LPAREN))
            PushParse(E_SUBSCRIPT);
        else
            Wr2TokError(ERR133); /* ILLEGAL REFERENCE to UNSUBSCRIPTED ARRAY */
    } else if (MatchTx1Item(T1_LPAREN)) {
        Wr2TokError(ERR127);    /* INVALID SUBSCRIPT ON NON-ARRAY */
        PushParse(E_SUBSCRIPT); // accept subscript anyway
    }
}

/*
 * ExprParse13 – parse a member reference via '.'.
 *
 * Pops the saved info index (pushed by ExprParse12) to recover the parent.
 *
 * If '.' follows:
 *   • ERR110 if the parent is not STRUCT_T.
 *   • ERR111 if no identifier follows.
 *   • Otherwise ChkStructureMember() resolves the member; X_IDENTIFIER and
 *     E_MEMBER are queued.  Array members require a subscript (ERR134 if
 *     absent); non-array members may not have one (ERR127).
 *
 * If no '.':
 *   • ERR135 if the parent is a STRUCT_T used without qualification.
 *   • FixupBased() handles BASED pointer indirection.
 */
static void ExprParse13() {
    info           = FromIdx(PopParse());
    info_t *parent = info;

    if (MatchTx1Item(T1_PERIOD)) {
        if (info->type != STRUCT_T)
            Wr2TokError(ERR110); /* INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE */
        else if (NotMatchTx1Item(T1_IDENTIFIER))
            Wr2TokError(ERR111); /* INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER */
        else {
            ChkStructureMember();
            PushSimpleOperand(X_IDENTIFIER, ToIdx(info)); // member
            PushParse(ToIdx(parent));                     // parent
            PushParse(E_MEMBER);
            if ((info->flag & F_ARRAY)) {
                if (MatchTx1Item(T1_LPAREN))
                    PushParse(E_SUBSCRIPT);
                else
                    Wr2TokError(ERR134); /* ILLEGAL REFERENCE to UNSUBSCRIPTED MEMBER ARRAY */
            } else if (MatchTx1Item(T1_LPAREN))
                Wr2TokError(ERR127); /* INVALID SUBSCRIPT ON NON-ARRAY */
        }
    } else {
        if (info->type == STRUCT_T)
            Wr2TokError(ERR135); /* ILLEGAL REFERENCE to AN UNQUALIFIED structure */
        FixupBased(parent);
    }
}

/*
 * ExprParse14 – emit an X_MEMBER node and apply based-variable fixup.
 *
 * Combines the two top operands (parent identifier and member identifier)
 * into an X_MEMBER node with arity 2 via PushComplexOperand().  Then pops
 * the saved parent info index and calls FixupBased() to insert any required
 * pointer-indirection node for BASED variables.
 */
static void ExprParse14() {
    PushComplexOperand(X_MEMBER, 2); // make member node
    FixupBased(FromIdx(PopParse())); // parent (might be based)
}

/*
 * ExprParse15 – parse one argument in a <argument list>.
 *
 * Increments the actual argument count (held in operatorStack[operatorSP]).
 * If ',' follows, pushes E_ARGLIST and E_EXPRESSION to consume the next
 * argument.  Otherwise calls ExpectRParen(ERR113) to close the argument
 * list.
 */
static void ExprParse15() {
    operatorStack[operatorSP]++; // actual argCnt
    if (MatchTx1Item(T1_COMMA)) {
        PushParse(E_ARGLIST);    //  get another arg
        PushParse(E_EXPRESSION); // <expression>
    } else
        ExpectRParen(ERR113); /* MISSING ') ' at end OF ARGUMENT LIST */
}

/*
 * ExprParse16 – reduce a typed procedure call to an X_CALL node.
 *
 * Retrieves the actual argument count via GetCallArgCnt() (which also pops
 * the arg-count slot from the operator stack) and combines all arguments
 * plus the procedure identifier into an X_CALL node of arity argCnt+1.
 */
static void ExprParse16() {
    PushComplexOperand(X_CALL, GetCallArgCnt() + 1);
}

/*
 * ExprParse17 – reduce a built-in call to its specific X_xxx node.
 *
 * On entry the parse stack holds (top-to-bottom): expected arg count,
 * built-in icode.  The operator stack top holds the actual arg count.
 * GetCallArgCnt() retrieves and pops the actual count; PopParse() retrieves
 * the icode.  PushComplexOperand() then assembles the node.
 */
static void ExprParse17() {
    uint8_t argCnt = GetCallArgCnt(); // separate out to avoid sequence point issues. Uses top args
    PushComplexOperand((uint8_t)PopParse(), argCnt);
}

/*
 * ExprParse18 – reduce an indirect variable call to X_CALLVAR.
 *
 * Builds an X_CALLVAR node from the actual argument count (popped from
 * the operator stack) plus 1 for the call target.
 *
 * Note: this state is never reached in standard PL/M-80 because a
 * variable followed by '(' is always parsed as a subscript by ExprParse12;
 * it is retained for completeness and potential future use.
 */
static void ExprParse18() {
    PushComplexOperand(X_CALLVAR, PopOperator() + 1);
}

/*
 * ExprParse19 – begin parsing a <subscript expression>.
 *
 * Pushes E_ENDSUBSCRIPT (to finalise the index node after the expression)
 * and E_EXPRESSION to parse the subscript value.
 */
static void ExprParse19() {
    PushParse(E_ENDSUBSCRIPT); // end of <subscript>
    PushParse(E_EXPRESSION);   // <expression>
}

/*
 * ExprParse20 – close a <subscript> and build the index node.
 *
 * If ',' follows (multiple subscripts), ERR114 is raised and the extra
 * subscript expression is discarded via RecoverRPOrEndExpr().
 * ExpectRParen(ERR115) consumes the closing ')'.
 * MkIndexNode() combines the array variable and subscript operands into an
 * indexed-access node.
 */
static void ExprParse20() {
    if (MatchTx1Item(T1_COMMA)) {
        Wr2TokError(ERR114); /* INVALID SUBSCRIPT, MULTIPLE SUBSCRIPTS ILLEGAL */
        RecoverRPOrEndExpr();
    }
    ExpectRParen(ERR115); /* MISSING ') ' at end OF SUBSCRIPT */
    MkIndexNode();
}

/*
 * ExpressionStateMachine – top-level expression-parse entry point.
 *
 * Pops parse-state words from parseStack[] and dispatches each to its
 * handler function until the stack is empty.  The caller is responsible for
 * seeding the stack (typically by pushing E_EXPRESSION) before calling this
 * function.
 *
 * State values in the range of valid E_xxx constants are dispatched by the
 * switch below.  Values outside that range are info-table indices or other
 * opaque data pushed by individual handlers and recovered by PopParse()
 * within those handlers; they must never reach this dispatch loop.
 */
void ExpressionStateMachine(void) {
    while (parseSP) {
        switch (parseStack[parseSP--]) {
        case E_EXPRESSION:
            ExprParse0(); // <expression>
            break;
        case E_EMBED:
            ExprParse1(); // check embedded :=
            break;
        case E_COLONEQUALS:
            ExprParse2(); // := ...
            break;
        case E_OPERATOR:
            ExprParse3(); // shift / reduce operator
            break;
        case E_UNARY:
            ExprParse4(); // <unaryOp>? <primary>
            break;
        case E_PRIMARY:
            ExprParse5(); // <primary>
            break;
        case E_RPAREN:
            ExprParse6(); // closing ')' of <subexpression>
            break;
        case E_ADDRESSOF:
            ExprParse7(); // <location reference> make addressof node
            break;
        case E_LOCREF:
            ExprParse8(); // <location reference>
            break;
        case E_DOTVAR:
            ExprParse9(); // <. variable reference>
            break;
        case E_MEMBERSPEC:
            ExprParse10(); // <member specifier>
            break;
        case E_VARREF:
            ExprParse11(); // <variable reference>-><builtin> | <proc> | <variable>
            break;
        case E_DATAREF:
            ExprParse12(); // <data reference>
            break;
        case E_DOT:
            ExprParse13(); // <member reference>
            break;
        case E_MEMBER:
            ExprParse14(); // make member node
            break;
        case E_ARGLIST:
            ExprParse15(); // arg list
            break;
        case E_FUNCTION:
            ExprParse16(); // <function>
            break;
        case E_BUILTIN:
            ExprParse17(); // <builtin>
            break;
        case E_CALLVAR:    // not used
            ExprParse18(); // call var - n
            break;
        case E_SUBSCRIPT:
            ExprParse19(); // <subscript expression>
            break;
        case E_ENDSUBSCRIPT:
            ExprParse20(); // <subscript end>
            break;
        }
    }
}