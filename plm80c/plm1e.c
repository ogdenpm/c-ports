/****************************************************************************
 *  plm1e.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
 * plm1e.c — Phase-2 statement parser and IR serialiser
 *
 * Overview
 * --------
 * This module forms the core of compiler phase 2.  It consumes the
 * phase-1 token stream (T1_xxx items) produced by plm0f.c / plm1b.c and
 * transforms each statement into phase-2 intermediate representation
 * (T2_xxx nodes) written to the tx2 file.
 *
 * Three major subsystems are provided:
 *
 *  1. Statement-level parsers (ParseLexItems and helpers)
 *     Read T1_xxx items from the token stream and dispatch to the
 *     appropriate handler for each statement type:
 *       assignment, CALL, RETURN, IF, DO/END (all variants), GOTO/GO TO,
 *       ENABLE, DISABLE, HALT, AT (locator), DATA/INITIAL, PROCEDURE,
 *       LABELDEF, LOCALLABEL, JMP, JMPFALSE, CASELABEL, EXTERNAL.
 *
 *  2. Expression builders (parseAssignment, ParseCall, Expression)
 *     Build an in-memory expression tree (tree[]) from the token stream
 *     using ExpressionStateMachine() (plm1d.c) and then hand it to
 *     SerialiseParse() for T2 output.
 *
 *  3. Tree serialiser (SerialiseParse and SerialiseParse0–15)
 *     Walk the expression tree in evaluation order and emit T2_xxx nodes
 *     to the tx2 file.  Uses its own explicit parse stack so recursive
 *     structures (calls, assignments, MOVE, subscripts) can be handled
 *     without deep C-call-stack recursion.
 *
 * Control-flow stack
 * ------------------
 * Nested DO blocks (simple, iterative, WHILE, CASE) and PROCEDURE bodies
 * are tracked by a pair of stacks:
 *
 *  controlStk[]   – block type (DO_PROC, SIMPLE_DO, DO_WHILE, DO_CASE,
 *                   DO_ITERATIVE).
 *  returnsVal[]   – whether a typed procedure's body contains a RETURN
 *                   with a value.
 *  infoStack[]    – saved curProc / loop-variable info for each nesting.
 *  hLabels[]      – head (entry) label for loops.
 *  eLabels[]      – end (exit) label for loops.
 *  sLabels[]      – step label for BY-clause iterative loops.
 *  byLabels[]     – by label for iterative loops with a BY clause.
 *
 * Scope is tracked by scopeChains[] / scopeSP (PushScope / PopScope).
 *
 * T2 output helpers
 * -----------------
 * Wr2Leaf(), Wr2Node(), Wr2ArgNode(), Wr2Simple() emit T2_xxx records and
 * return a sequential counter (t2Cnt) used by CvtToRel() to express
 * back-references as relative offsets.  MoveHead() stores the t2Cnt of the
 * root node back into the tree slot so parent nodes can reference it.
 */

#include "plm.h"

/*
 * parseAssignment – parse a PL/M assignment statement.
 *
 * Reads one or more comma-separated l-value references (identifiers with
 * optional subscripts, member qualifications and BASED indirection), then
 * '=' and the right-hand-side expression.
 *
 * For each l-value:
 *   • NotMatchTx1Item(T1_IDENTIFIER) → ERR128 (INVALID LEFT-HAND OPERAND), return false.
 *   • ChkIdentifier() resolves the symbol; ExpressionStateMachine() (state 11)
 *     handles any subscript / member / based qualification.
 *   • IsLValue() validates the operand; ERR128 and PopOperand() on failure.
 *   • Valid l-values increment assignCnt.
 *
 * After all l-values, '=' must follow (ERR116 if absent).  The RHS
 * expression is then parsed, and if any l-values were accepted an
 * X_COLONEQUALS node of arity (assignCnt+1) is built and moved into the
 * statement tree.  Returns true on success, false on error.
 */
bool parseAssignment() {
    ResetStacks();
    uint8_t assignCnt = 0;
    while (1) {
        if (NotMatchTx1Item(T1_IDENTIFIER)) {
            Wr2TokError(ERR128); /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
            return 0;
        }
        ChkIdentifier();
        PushParse(11);
        ExpressionStateMachine(); // evaluate any member / subscript / based elements
        if (!IsLValue(operandSP)) {
            Wr2TokError(ERR128); /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
            PopOperand();        // ignore the operand
        } else
            assignCnt++;
        if (NotMatchTx1Item(T1_COMMA)) // check for multiple assignments
            break;
    }

    if (MatchTx1Item(T1_EQ)) {
        PushParse(0); // <expression>
        ExpressionStateMachine();
        if (assignCnt) {
            PushComplexOperand(X_COLONEQUALS, assignCnt + 1);
            MoveOperandToTree();
            markedStSP = stSP;
            return true;
        } else
            return false;
    } else {
        Wr2TokError(ERR116); /* MISSING '=' IN ASSIGNMENT STATEMENT */
        return false;
    }
}

/*
 * ParseCall – parse a CALL statement.
 *
 * Reads the procedure name via GetVariable() and dispatches:
 *
 *  PROC_T    – validates with ChkIllegalCall(); ERR129 if typed.  Builds
 *              X_IDENTIFIER + arg-count slots + E_FUNCTION state.  If '('
 *              follows, queues E_ARGLIST / E_EXPRESSION.
 *
 *  BUILTIN_T – ERR129 if the built-in has a return type.  Builds icode +
 *              arg-count slots + E_BUILTIN.  If '(' follows, queues args.
 *
 *  ADDRESS_T (scalar) – indirect call via X_CALLVAR; builds call-var node
 *              via E_CALLVAR.  If '(' follows, queues args.
 *
 *  other     – ERR118 (NOT AN ADDRESS SCALAR), return false.
 *
 * On success, ExpressionStateMachine() finishes the call tree,
 * MoveOperandToTree() stores the root, and the function returns true.
 */
uint8_t ParseCall() {
    ResetStacks();
    if (NotMatchTx1Item(T1_IDENTIFIER)) {
        Wr2TokError(ERR117); /* MISSING procedure NAME IN call STATEMENT */
        return false;
    }
    GetVariable();

    if (info->type == PROC_T) {
        ChkIllegalCall();
        if (info->returnType)
            Wr2TokError(ERR129); /* ILLEGAL 'call' WITH TYPED procedure */
        PushSimpleOperand(X_IDENTIFIER, ToIdx(info));
        PushParse(info->paramCnt);
        PushOperator(0);
        PushParse(16);
        if (MatchTx1Item(T1_LPAREN)) {
            PushParse(15);
            PushParse(0);
        }
    } else if (info->type == BUILTIN_T) {
        if (info->returnType) {
            Wr2TokError(ERR129); /* ILLEGAL 'call' WITH TYPED procedure */
            return false;
        }
        PushParse(builtinsMap[info->builtinId]);
        PushParse(info->paramCnt);
        PushOperator(0);
        PushParse(17);
        if (MatchTx1Item(T1_LPAREN)) {
            PushParse(15);
            PushParse(0);
        }
    } else if (info->type != ADDRESS_T || (info->flag & F_ARRAY)) {
        Wr2TokError(ERR118); /* INVALID INDIRECT CALL, IDENTIFIER NOT AN ADDRESS SCALAR */
        return false;
    } else {
        PushVariable();
        PushOperator(0);
        PushParse(18);
        if (MatchTx1Item(T1_LPAREN)) {
            PushParse(15);
            PushParse(0);
        }
    }
    ExpressionStateMachine();
    MoveOperandToTree();
    markedStSP = stSP;
    return true;
}

/*
 * Expression – parse a standalone expression into the statement tree.
 *
 * Resets operand/operator/parse stacks, seeds state E_EXPRESSION (0) and
 * runs ExpressionStateMachine().  Moves the resulting root operand into the
 * statement tree and records markedStSP for SerialiseParse().
 */
void Expression() {
    ResetStacks();
    PushParse(0);
    ExpressionStateMachine();
    MoveOperandToTree();
    markedStSP = stSP;
}

/*
 * MoveHead – record the T2 output index for a tree node.
 *
 * Stores t2Cnt (the sequential index of the emitted T2 record) into
 * tree[node].val so that parent nodes can reference it via CvtToRel().
 */
static void MoveHead(uint16_t node, uint16_t t2Cnt) {
    tree[node].val = t2Cnt;
}

/*
 * SerialiseSimpleNode – emit a T2 leaf record for a tree node with no children.
 *
 * Maps the node's icode to the appropriate T2_xxx token:
 *   X_IDENTIFIER              → T2_IDENTIFIER  (info index as payload)
 *   X_NUMBER                  → T2_NUMBER or T2_BIGNUMBER (>= 0x100)
 *   X_INPUT                   → T2_INPUT        (port number)
 *   X_LENGTH / X_LAST / X_SIZE → mapped via iToTx2Map (info index)
 *   other                     → iToTx2Map lookup, no payload (Wr2Simple)
 * Calls MoveHead() to store the resulting t2Cnt in the tree node.
 */
static void SerialiseSimpleNode(uint16_t node) {
    uint8_t iCode = tree[node].icode;
    uint16_t t2Cnt;
    if (iCode == X_IDENTIFIER)
        t2Cnt = Wr2Leaf(T2_IDENTIFIER, tree[node].infoIdx); // info
    else if (iCode == X_NUMBER) {
        t2Cnt = Wr2Leaf(High(tree[node].val) ? T2_BIGNUMBER : T2_NUMBER,
                        tree[node].val); // big or small number
    } else if (iCode == X_INPUT)         // port
        t2Cnt = Wr2Leaf(T2_INPUT, tree[node].val);
    else if (iCode == X_LENGTH || iCode == X_LAST || iCode == X_SIZE)
        t2Cnt = Wr2Leaf(iToTx2Map[iCode], tree[node].infoIdx); // info
    else
        t2Cnt = Wr2Simple(iToTx2Map[iCode]); // no args
    MoveHead(node, t2Cnt);
}

/*
 * SerialiseParse0 – top-level serialise dispatch for one tree node.
 *
 * Pops the node index from the parse stack.  If the node is X_OUTPUT, it
 * is skipped (handled inline in SerialiseParse11).  Nodes with no children
 * (right == 0) are handled by SerialiseSimpleNode().
 *
 * Composite nodes push the node back plus a follow-up state:
 *   X_CALL        → state 3  (procedure call sequence)
 *   X_CALLVAR     → state 6  (indirect call sequence)
 *   X_COLONEQUALS → state 9  (assignment sequence)
 *   X_MOVE        → state 14 (MOVE built-in sequence)
 *   X_BYTEINDEX /
 *   X_WORDINDEX   → state 8  (subscript index node)
 *   other         → state 13 (binary / unary operator)
 * Child nodes are queued via states 1 and 2 for in-order traversal.
 */
static void SerialiseParse0() { // top level serialise
    uint8_t icode;

    uint16_t node = PopParse();

    if ((icode = tree[node].icode) == X_OUTPUT)
        return;
    if (tree[node].right == 0) // no supplementary args
        SerialiseSimpleNode(node);
    else {
        PushParse(node); // resave node
        if (icode == X_CALL)
            PushParse(3);
        else if (icode == X_CALLVAR) {
            PushParse(6);
            PushParse(tree[node].nodeIdx);
            PushParse(0);
        } else if (icode == X_COLONEQUALS)
            PushParse(9);
        else if (icode == X_MOVE) {
            PushParse(14);
            PushParse(tree[node].nodeIdx);
            PushParse(0); // recurse
        } else if (icode == X_BYTEINDEX || icode == X_WORDINDEX) {
            PushParse(8);
            PushParse(2);
            PushParse(tree[node].nodeIdx);
            PushParse(1);
        } else {
            PushParse(13);
            PushParse(tree[node].right);   /* num args */
            PushParse(tree[node].nodeIdx); /* loc of args */
            PushParse(1);
        }
    }
}

/*
 * SerialiseParse1 – serialise one child node.
 *
 * Peeks at the node index on top of the parse stack, pushes state 2
 * (advance-or-finish) above it, then pushes the node index and state 0 to
 * recursively serialise the child.
 */
static void SerialiseParse1() { /* serialise 1 node */
    uint16_t node = parseStack[parseSP];
    PushParse(2);    /* flag to check for more nodes */
    PushParse(node); /* serialise this node */
    PushParse(0);
}

/*
 * SerialiseParse2 – advance to the next sibling node or finish.
 *
 * Decrements the remaining-child counter at parseStack[parseSP-1].  If it
 * reaches zero, the counter and node-index slots are popped.  Otherwise the
 * node index is incremented and state 1 is pushed to serialise the next
 * sibling.
 */
static void SerialiseParse2() { /* check for any more leaves */
    if (--parseStack[parseSP - 1] == 0) {
        PopParse();
        PopParse();
    } else {
        parseStack[parseSP]++;
        PushParse(1);
    }
}

/*
 * SerialiseParse3 – serialise the arguments and begin a CALL sequence.
 *
 * Emits T2_BEGCALL to open the call frame.  If the call has arguments
 * (right > 1), queues:
 *   • State 4 (SerialiseParse4) to push all arguments beyond the first two
 *     onto the 8080 stack.
 *   • State 0 to serialise the first argument.
 * Then pushes state 5 (SerialiseParse5) to emit the final T2_CALL node
 * once all arguments are serialised.
 */
static void SerialiseParse3() {                              /* parse args */
    uint16_t argList = parseStack[parseSP];                  // the call node
    PushParse(5);                                            /* final call wrap up */
    if (tree[argList].right > 1) {                           /* any args */
        PushParse(tree[argList].right - 1);                  /* num args  */
        PushParse(tree[argList].nodeIdx + 1);                /* argList */
        info = FromIdx(tree[tree[argList].nodeIdx].infoIdx); /* from proc info adv to arginfo */
        AdvNxtInfo();
        PushParse(ToIdx(info));               // arg info
        PushParse(4);                         // serialise stack args
        PushParse(tree[argList].nodeIdx + 1); /* serialise 1st of argList */
        PushParse(0);
    }
    Wr2Simple(T2_BEGCALL); // initialise CALL sequence
}

/*
 * SerialiseParse4 – emit stack-argument T2 nodes (args 3 and beyond).
 *
 * For each argument after the first two (which are passed in BC/DE):
 *   • If info is NULL (CALLVAR variant), emits T2_STKARG with the
 *     argument's relative T2 address.
 *   • Otherwise emits T2_STKBARG (BYTE_T) or T2_STKWARG (ADDRESS/WORD)
 *     and advances info to the next parameter descriptor.
 * MoveHead() records the emitted t2Cnt in the argument tree slot.
 * When all stack arguments are processed the three control slots (info,
 * argList, argCnt) are popped; otherwise state 4 is re-queued for the
 * next argument.
 */
static void SerialiseParse4() {
    info             = FromIdx(parseStack[parseSP]);
    uint16_t argList = parseStack[parseSP - 1];
    uint8_t argCnt   = (uint8_t)parseStack[parseSP - 2];
    if (argCnt > 2) { /* all bar first 2 args to stack */
        uint16_t t2Cnt;
        if (!info) // call var variant
            t2Cnt = Wr2Leaf(T2_STKARG, CvtToRel(tree[argList].nodeIdx));
        else {
            t2Cnt = Wr2Leaf(info->type == BYTE_T ? T2_STKBARG : T2_STKWARG,
                            CvtToRel(tree[argList].nodeIdx));
            AdvNxtInfo();
            parseStack[parseSP] = ToIdx(info);
        }
        MoveHead(argList, t2Cnt);
    }
    argCnt--;
    if (argCnt == 0) { /* all done, clear working data */
        PopParse();    // info
        PopParse();    // arg list
        PopParse();    // arg count
    } else {
        parseStack[parseSP - 2] = argCnt;
        parseStack[parseSP - 1] = ++argList;
        PushParse(4); /* serialise stack args */
        PushParse(argList);
        PushParse(0);
    }
}

/*
 * SerialiseParse5 – finalise a CALL node.
 *
 * Pops the call node index and computes relative T2 references for the
 * first (BC) and second (DE) register arguments.  Emits a T2_CALL record
 * via Wr2ArgNode() with relBC, relDE, and the procedure identifier's T2
 * index.  MoveHead() records the result in the call node.
 */
static void SerialiseParse5() {
    uint16_t node    = PopParse();
    uint16_t argList = tree[node].nodeIdx;
    uint8_t argCnt   = tree[node].right - 1;
    uint16_t relBC   = 0;
    uint16_t relDE   = 0;
    if (argCnt > 1) {
        relBC = CvtToRel(tree[argList + argCnt - 1].nodeIdx);
        relDE = CvtToRel(tree[argList + argCnt].nodeIdx);
    } else if (argCnt > 0)
        relBC = CvtToRel(tree[argList + argCnt].nodeIdx);
    MoveHead(node, Wr2ArgNode(T2_CALL, relBC, relDE, tree[argList].nodeIdx));
}

/*
 * SerialiseParse6 – begin serialising an indirect CALL VAR sequence.
 *
 * Emits T2_BEGCALL, then queues state 4 to handle any arguments beyond the
 * first two and state 7 to emit the final T2_CALLVAR node.
 */
static void SerialiseParse6() {
    Wr2Simple(T2_BEGCALL);
    uint16_t node = parseStack[parseSP];
    PushParse(7); // finalise call var
    uint8_t argCnt = tree[node].right - 1;
    if (argCnt) {
        PushParse(argCnt); // count of args
        uint16_t argList = tree[node].nodeIdx + 1;
        PushParse(argList);
        PushParse(0); /* simple T2_STKARG */
        PushParse(4); /* serialise stack args */
        PushParse(argList);
        PushParse(0);
    }
}

/*
 * SerialiseParse7 – finalise an indirect CALL VAR node.
 *
 * Pops the call-var node, computes BC/DE relative references for up to two
 * register arguments, and emits T2_CALLVAR via Wr2ArgNode() with the
 * call-target variable's relative T2 address.  MoveHead() stores the
 * result.
 */
static void SerialiseParse7() {

    uint16_t node    = PopParse();
    uint8_t argCnt   = tree[node].right - 1; // number of args (-1 to zero base)
    uint16_t argList = tree[node].nodeIdx;   // location of args
    uint16_t relBC = 0, relDE = 0;
    if (argCnt > 1) { // 2 or more args
        relBC = CvtToRel(tree[argList + argCnt - 1].nodeIdx);
        relDE = CvtToRel(tree[argList + argCnt].nodeIdx);
    } else if (argCnt > 0) // 1 arg
        relBC = CvtToRel(tree[argList + argCnt].nodeIdx);
    uint16_t t2Cnt = Wr2ArgNode(T2_CALLVAR, relBC, relDE, CvtToRel(tree[argList].nodeIdx));
    MoveHead(node, t2Cnt);
}

/*
 * SerialiseParse8 – emit a subscript index (BYTEINDEX / WORDINDEX) node.
 *
 * Pops the index node, maps its icode via iToTx2Map, and emits a three-
 * argument T2 record (via Wr2ArgNode) holding the relative T2 addresses of
 * the array base, the subscript expression, and the element-size leaf.
 * MoveHead() records the t2Cnt.
 */
static void SerialiseParse8() {
    uint16_t node    = PopParse();
    uint8_t icode    = tree[node].icode;
    uint16_t leafIdx = tree[node].nodeIdx;
    uint16_t t2Cnt   = Wr2ArgNode(iToTx2Map[icode], CvtToRel(tree[leafIdx].nodeIdx),
                                  CvtToRel(tree[leafIdx + 1].nodeIdx), tree[leafIdx + 2].nodeIdx);
    MoveHead(node, t2Cnt);
}

/*
 * SerialiseParse9 – begin serialising a ':=' assignment node.
 *
 * Peeks at the assignment node index, then pushes state 10 (process all
 * LHS targets) and first queues state 0 to serialise the RHS expression
 * (the last child of the node).
 */
static void SerialiseParse9() { // parse :=
    uint16_t node = parseStack[parseSP];
    PushParse(10);                                        /* post serialise RHS */
    PushParse(tree[node].nodeIdx + tree[node].right - 1); // RHS node, i.e. past assignments
    PushParse(0);
}

/*
 * SerialiseParse10 – set up iteration over all LHS targets of ':='.
 *
 * Peeks at the assignment node, extracts assignCnt and the base index of
 * the LHS operands, then queues state 12 (mark LHS used) and state 11
 * (emit one assignment) for each LHS, starting with the first.  State 11
 * will re-queue itself for subsequent LHS targets.
 */
static void SerialiseParse10() {
    uint16_t node = parseStack[parseSP];
    PushParse(12);                              /* mark LHS as used at end */
    uint8_t assignCnt   = tree[node].right - 1; /* num LHS */
    uint16_t assignBase = tree[node].nodeIdx;   /* base LHS */
    PushParse(assignCnt);
    PushParse(assignBase);
    PushParse(assignBase + assignCnt); /* RHS */
    PushParse(11);                     /* after serialised leaf */
    PushParse(assignBase);             /* do the leaf */
    PushParse(0);
}

/*
 * SerialiseParse11 – emit one ':=' assignment T2 node.
 *
 * Recovers the current LHS index and the RHS index from the parse stack.
 *
 * If the LHS is X_OUTPUT (port assignment):
 *   Emits T2_NUMBER for the port number, then T2_OUTPUT with the port and
 *   the RHS relative T2 address.
 *
 * Otherwise:
 *   Emits T2_COLONEQUALS with LHS and RHS relative T2 addresses.
 *
 * Decrements the remaining-assignment counter; if zero, pops the three
 * control slots.  Otherwise advances the LHS index and re-queues state 11
 * (after serialising the next LHS via state 0).
 */
static void SerialiseParse11() { /* do one assignment */

    uint16_t assignIdx = parseStack[parseSP - 1]; // lvalue
    uint16_t rhsIdx    = parseStack[parseSP];     // rvalue
    if (tree[assignIdx].icode == X_OUTPUT) {
        uint16_t numberT2 = Wr2Leaf(T2_NUMBER, tree[assignIdx].val); // serialise the port number
        Wr2Node(T2_OUTPUT, CvtToRel(numberT2), CvtToRel(tree[rhsIdx].nodeIdx)); // and the output
    } else
        Wr2Node(T2_COLONEQUALS, CvtToRel(tree[assignIdx].nodeIdx), CvtToRel(tree[rhsIdx].nodeIdx));

    if (--parseStack[parseSP - 2] == 0) { // assignment count 0 -> done
        PopParse();                       // cleanup the control variables
        PopParse();
        PopParse();
    } else { /* no so do another */
        parseStack[parseSP - 1] = ++assignIdx;
        PushParse(11);        /* state 11 after serialise */
        PushParse(assignIdx); /* serialise next lvalue */
        PushParse(0);
    }
}

/*
 * SerialiseParse12 – mark the assignment node's T2 output index.
 *
 * Pops the assignment node, reads the t2Cnt of the RHS (the last child),
 * and stores it in the assignment node's val via MoveHead() so that any
 * parent node referencing this assignment gets the RHS result address.
 */
static void SerialiseParse12() { /* mark LHS as used */
    uint16_t node  = PopParse();
    uint16_t t2Cnt = tree[tree[node].nodeIdx + tree[node].right - 1].nodeIdx;
    MoveHead(node, t2Cnt); // move lhs to point to the rvalue tree node
}

/*
 * SerialiseParse13 – emit a binary or unary operator T2 node.
 *
 * Pops the operator node, maps its icode via iToTx2Map.  For arity 1
 * (unary), emits a T2 leaf with the single operand's relative address.
 * For arity 2 (binary), emits a T2 node with both operands' relative
 * addresses.  MoveHead() records the t2Cnt.
 */
static void SerialiseParse13() { /* binary or unary op */

    uint16_t node = PopParse();
    uint16_t sIdx = tree[node].nodeIdx;
    uint8_t iCode = iToTx2Map[tree[node].icode];
    uint16_t t2Cnt;
    if (tree[node].right == 1)
        t2Cnt = Wr2Leaf(iCode, CvtToRel(tree[sIdx].nodeIdx));
    else
        t2Cnt = Wr2Node(iCode, CvtToRel(tree[sIdx].nodeIdx), CvtToRel(tree[sIdx + 1].nodeIdx));
    MoveHead(node, t2Cnt);
}

/*
 * SerialiseParse14 – begin serialising a MOVE built-in call.
 *
 * Emits T2_BEGMOVE with the count argument's relative T2 address.  Queues
 * state 15 (SerialiseParse15) to emit the final T2_MOVE record, and state
 * 1 / 2 to serialise the two address arguments.
 */
static void SerialiseParse14() {
    uint16_t p = tree[parseStack[parseSP]].nodeIdx;
    /* emit the count leaf */
    uint16_t t2Cnt = Wr2Leaf(T2_BEGMOVE, CvtToRel(tree[p].nodeIdx));
    MoveHead(p, t2Cnt);
    PushParse(15); /* Move() post serialise */
    PushParse(2);  /* serialise the address leaves */
    PushParse(p + 1);
    PushParse(1);
}

/*
 * SerialiseParse15 – finalise a MOVE built-in node.
 *
 * Pops the MOVE node, reads the three child T2 addresses (count via
 * T2_BEGMOVE, source, destination) and emits T2_MOVE via Wr2ArgNode().
 * MoveHead() records the t2Cnt.
 */
static void SerialiseParse15() { /* rest of Move() */

    uint16_t node  = PopParse();
    uint16_t sIdx  = tree[node].nodeIdx;
    uint16_t t2Cnt = Wr2ArgNode(T2_MOVE, CvtToRel(tree[sIdx + 1].t2Cnt),
                                CvtToRel(tree[sIdx + 2].t2Cnt), CvtToRel(tree[sIdx].t2Cnt));
    MoveHead(node, t2Cnt);
}

/*
 * SerialiseParse – serialise the expression tree rooted at stmtSp.
 *
 * Entry point called by all statement-level parsers after the expression
 * tree has been built.  Initialises parseSP, seeds the stack with the root
 * node index and state 0, then loops until the stack is empty, dispatching
 * SerialiseParse0–15.  Returns the T2 output index of the root node.
 *
 * info is saved and restored around the traversal so callers' info state
 * is not disturbed by the tree walk.
 */
uint16_t SerialiseParse(uint16_t stmtSp) {
    info_t *savInfo = info;
    parseSP         = 0;
    PushParse(stmtSp);
    PushParse(0); // start with state 0
    while (parseSP != 0) {
        switch (parseStack[parseSP--]) {
        case 0:
            SerialiseParse0();
            break;
        case 1:
            SerialiseParse1();
            break;
        case 2:
            SerialiseParse2();
            break;
        case 3:
            SerialiseParse3();
            break; // CALL
        case 4:
            SerialiseParse4();
            break;
        case 5:
            SerialiseParse5();
            break;
        case 6:
            SerialiseParse6();
            break;
        case 7:
            SerialiseParse7();
            break;
        case 8:
            SerialiseParse8();
            break;
        case 9:
            SerialiseParse9();
            break; // COLONEQUALS
        case 10:
            SerialiseParse10();
            break;
        case 11:
            SerialiseParse11();
            break;
        case 12:
            SerialiseParse12();
            break;
        case 13:
            SerialiseParse13();
            break;
        case 14:
            SerialiseParse14();
            break;
        case 15:
            SerialiseParse15();
            break;
        }
    }
    info = savInfo;
    return tree[stmtSp].nodeIdx;
}

/*
 * Control-flow and scope stacks
 *
 * These arrays are indexed by controlSP (0-based, max depth 19).
 *
 *  controlStk[]  – block type token for each nesting level:
 *                    DO_PROC, SIMPLE_DO, DO_WHILE, DO_CASE, DO_ITERATIVE.
 *  returnsVal[]  – true if this level (or any nested level that bubbles up
 *                  via PopControl) has emitted a typed RETURN.
 *  infoStack[]   – saved curProc pointer (DO_PROC) or loop-variable info
 *                  pointer (DO_ITERATIVE) for the current level.
 *  hLabels[]     – head / entry label for WHILE and iterative DO loops.
 *  eLabels[]     – end / exit label for WHILE and iterative DO loops.
 *  sLabels[]     – step-code label for iterative DO loops with a BY clause
 *                  (0 if no BY).
 *  byLabels[]    – label placed after the step code (before the loop body)
 *                  for BY-clause iterative loops.
 */
static uint8_t controlStk[20];
static bool returnsVal[20];
static info_t *infoStack[20];
static uint16_t hLabels[20];
static uint16_t eLabels[20];
static uint16_t sLabels[20];
static uint16_t byLabels[20];
static uint16_t controlSP; /* current depth of the control stack (0 = empty) */

/*
 * PushScope – push a scope value onto the scope chain.
 *
 * ERR164 (SCOPE STACK OVERFLOW) is fatal if scopeSP already equals 34.
 */
static void PushScope(uint16_t arg1w) {
    if (scopeSP == 34)
        fatal_ov1(ERR164); /* COMPILER Error: SCOPE STACK OVERFLOW */
    else
        scopeChains[++scopeSP] = arg1w;
}

/*
 * PopScope – pop the top scope from the scope chain.
 *
 * ERR165 (SCOPE STACK UNDERFLOW) is fatal if scopeSP is already 0.
 */
static void PopScope() {
    if (scopeSP == 0)
        fatal_ov1(ERR165); /* COMPILER Error: SCOPE STACK UNDERFLOW */
    else
        scopeSP--;
}

/*
 * PushControl – push a new block-nesting level.
 *
 * ERR84 (LIMIT EXCEEDED: BLOCK NESTING) is fatal if controlSP already
 * equals 19.  Otherwise increments controlSP and initialises all fields
 * (controlType, returnsVal, infoStack, hLabels, eLabels, sLabels) to safe
 * defaults.
 */
static void PushControl(uint8_t controlType) {
    if (controlSP == 19)
        fatal_ov1(ERR84); /*  LIMIT EXCEEDED: BLOCK NESTING */
    else {
        controlStk[++controlSP] = controlType;
        returnsVal[controlSP]   = false;
        infoStack[controlSP]    = NULL;
        hLabels[controlSP]      = 0;
        eLabels[controlSP]      = 0;
        sLabels[controlSP]      = 0;
    }
}

/*
 * PopControl – pop the current block-nesting level.
 *
 * ERR167 (CONTROL STACK UNDERFLOW) is fatal if controlSP is 0.
 * If the block being popped is a non-zero type (i.e. not a bare DO),
 * its returnsVal is OR-ed into the parent level's returnsVal so that typed
 * procedure return-value tracking propagates upward.
 */
static void PopControl() {
    if (controlSP == 0)
        fatal_ov1(ERR167); /* COMPILER Error: CONTROL STACK UNDERFLOW */
    else if (controlStk[controlSP--])
        returnsVal[controlSP] |= returnsVal[controlSP + 1];
}

/*
 * NewLocalLabel – allocate a new compiler-generated local label.
 *
 * Increments and returns localLabelCnt.  Labels are referenced by index in
 * T2_LOCALLABEL, T2_JMP, T2_JMPFALSE etc.
 */
static uint16_t NewLocalLabel() {
    return ++localLabelCnt;
}

/*
 * WrTx2VarId – emit a T2_IDENTIFIER leaf (and T2_MEMBER node if needed).
 *
 * For a structure member (F_MEMBER set), emits the parent identifier first,
 * then the member identifier, and combines them into a T2_MEMBER node.
 * Sets info to the parent info entry in that case, or to varInfo otherwise.
 * Returns the T2 index of the resulting leaf or node.
 */
static uint16_t WrTx2VarId(info_t *varInfo) {
    uint16_t varT2 = Wr2Leaf(T2_IDENTIFIER, ToIdx(varInfo));
    if ((varInfo->flag & F_MEMBER)) {
        uint16_t memberNode = Wr2Leaf(T2_IDENTIFIER, ToIdx(varInfo->parent));
        varT2               = Wr2Node(T2_MEMBER, CvtToRel(memberNode), CvtToRel(varT2));
        info                = varInfo->parent;
    } else
        info = varInfo;
    return varT2;
}

/*
 * WrTx2Var – emit a variable reference, inserting T2_BASED indirection if needed.
 *
 * Calls WrTx2VarId() to emit the identifier (and member node if required).
 * If the variable is BASED (F_BASED), also emits the base pointer via
 * WrTx2VarId() and wraps both in a T2_BASED node.  Returns the T2 index of
 * the resulting reference.
 */
static uint16_t WrTx2Var(info_t *varInfo) {
    uint16_t varT2 = WrTx2VarId(varInfo);
    if (!(info->flag & F_BASED))
        return varT2;
    return Wr2Node(T2_BASED, CvtToRel(WrTx2VarId(info->baseInfo)), CvtToRel(varT2));
}

/*
 * ChkEndOfStmt – consume and report any spurious expression tokens after a statement.
 *
 * If the next token carries F_EXPRITEM (i.e. is part of an expression),
 * ERR32 (INVALID SYNTAX, TEXT IGNORED UNTIL ';') is raised and all
 * expression items are discarded until a non-expression token is reached.
 * The terminating token is un-gotten so the main dispatch loop sees it.
 */
static void ChkEndOfStmt() {
    if (MatchTx2AuxFlag(F_EXPRITEM)) {
        Wr2TokError(ERR32);              /* INVALID SYNTAX, TEXT IGNORED UNTIL ';' */
        while ((tx1Attr & F_EXPRITEM)) { // skip Expression items
            GetTx1Item();
        }
        UngetTx1Item();
    }
}

/*
 * EndIterDo – emit the loop-back and end-label IR for an iterative DO block.
 *
 * Called from ParseEND() when controlStk[controlSP] == DO_ITERATIVE.
 *
 * Without a BY clause (sLabels[controlSP] == 0):
 *   Emits an inline increment of the loop variable:
 *     BYTE  loop var: T2_LOCALLABEL (guard), add 1 via T2_PLUSSIGN,
 *           T2_COLONEQUALS, T2_JNZ headLabel.
 *     WORD  loop var: add 1 via T2_ADDW, T2_COLONEQUALS, T2_JNC headLabel.
 *
 * With a BY clause:
 *   Emits T2_JMP stepLabel (the step code was already emitted by
 *   IterativeDoStatement()).
 *
 * In both cases emits T2_LOCALLABEL endLabel to mark the loop exit.
 */
static void EndIterDo() {
    uint16_t stepLoop;

    info              = infoStack[controlSP];
    uint16_t headLoop = hLabels[controlSP];
    uint16_t endLoop  = eLabels[controlSP];
    if ((stepLoop = sLabels[controlSP]) == 0) {
        if (info->type == BYTE_T)
            Wr2Leaf(T2_LOCALLABEL, NewLocalLabel());
        uint16_t loopVarT2  = WrTx2Var(info);
        uint16_t loopTmpT2  = WrTx2Var(info);
        uint16_t loopStepT2 = Wr2Leaf(T2_NUMBER, 1);
        if (info->type == BYTE_T) {
            loopTmpT2 = Wr2Node(T2_PLUSSIGN, CvtToRel(loopTmpT2), CvtToRel(loopStepT2));
            Wr2Node(T2_COLONEQUALS, CvtToRel(loopVarT2), CvtToRel(loopTmpT2));
            Wr2Node(T2_JNZ, headLoop, 3);
        } else {
            loopTmpT2 = Wr2Node(T2_ADDW, CvtToRel(loopTmpT2), CvtToRel(loopStepT2));
            Wr2Node(T2_COLONEQUALS, CvtToRel(loopVarT2), CvtToRel(loopTmpT2));
            Wr2Leaf(T2_JNC, headLoop);
        }
    } else
        Wr2Leaf(T2_JMP, stepLoop);
    Wr2Leaf(T2_LOCALLABEL, endLoop);
}

/*
 * SerialiseExpression – parse and immediately serialise one expression.
 *
 * Calls Expression() to build the tree, then SerialiseParse(markedStSP)
 * to emit the T2 records.  Returns the T2 index of the root node.
 */
static uint16_t SerialiseExpression() {
    Expression();
    return SerialiseParse(markedStSP);
}

/*
 * SerialiseIF – serialise an IF condition and emit T2_JMPFALSE.
 *
 * Calls SerialiseExpression() to emit the condition, then ChkEndOfStmt()
 * to consume any trailing tokens.  Reads the T1_JMPFALSE item inserted by
 * plm0f.c (which carries the target label index) and emits T2_JMPFALSE
 * with the label and the condition's relative T2 address.
 * ERR168 (BRANCH MISSING IN 'IF' STATEMENT) is fatal if T1_JMPFALSE is
 * absent (should never happen with a well-formed phase-1 stream).
 */
static void SerialiseIF() {
    uint16_t exprT2 = SerialiseExpression();
    ChkEndOfStmt();
    if (MatchTx1Item(T1_JMPFALSE))
        Wr2Node(T2_JMPFALSE, tx1Item.dataw[0], CvtToRel(exprT2));
    else
        fatal_ov1(ERR168); /* COMPILER ERROR: BRANCH MISSING IN 'IF' STATEMENT */
}

/*
 * ParseStmtcnt – process a T1_STMTCNT item.
 *
 * Resets stmtT2Cnt (the per-statement T2 item counter), passes the item
 * through to phase 2 via MapLToT2(), and syncs curStmtNum to the statement
 * number carried in the item.
 */
static void ParseStmtcnt() {
    stmtT2Cnt = 0;                 // reset stmt item counter
    MapLToT2();                    // pass through stmt info
    curStmtNum = tx1Item.dataw[0]; // sync up stmt number
}

/*
 * ParseIf – handle a T1_IF item.
 *
 * Thin wrapper that calls SerialiseIF() to emit the condition and the
 * conditional branch.
 */
static void ParseIf() {
    SerialiseIF();
}

/*
 * ParseProcedure – handle a T1_PROCEDURE item.
 *
 * Passes the item through to phase 2 via MapLToT2(), pushes a DO_PROC
 * control level, saves the current curProc in infoStack[], and updates
 * curProc / info to the new procedure's info entry (from dataw[0]).
 * Sets F_DECLARED on the procedure info.
 */
static void ParseProcedure() {
    MapLToT2();                                 // map through the input
    PushControl(DO_PROC);                       // enter region
    infoStack[controlSP] = curProc;             // save current proc
    curProc = info = FromIdx(tx1Item.dataw[0]); // proc info
    info->flag |= F_DECLARED;                   // flag declared
}

/*
 * ParseWhile – handle a T1_WHILE (DO WHILE) block header.
 *
 * Pushes a DO_WHILE control level, emits T2_LOCALLABEL for the loop-back
 * point (stored in hLabels[]), allocates an end label (eLabels[]), then
 * calls SerialiseExpression() to emit the condition.  Emits T2_JMPFALSE to
 * the end label.
 *
 * The emitted IR sequence is:
 *   T2_LOCALLABEL headLabel
 *   <condition expression>
 *   T2_JMPFALSE   endLabel  rel(condition)
 *
 * T2_JMP headLabel and T2_LOCALLABEL endLabel are appended by ParseEND().
 */
static void ParseWhile() {
    PushControl(DO_WHILE);                                        // enter region
    Wr2Leaf(T2_LOCALLABEL, hLabels[controlSP] = NewLocalLabel()); // loop label
    uint16_t endLabel = eLabels[controlSP] = NewLocalLabel();     // jmp to end when done
    Wr2Node(T2_JMPFALSE, endLabel, CvtToRel(SerialiseExpression()));
    ChkEndOfStmt();
}

/*
 * ParseCASE – handle a T1_CASE (DO CASE) block header.
 *
 * Pushes a DO_CASE control level, passes the T1_CASE item through via
 * MapLToT2(), then serialises the switch expression and emits T2_CASEBLOCK
 * with a new end label (stored in hLabels[]) and the expression's relative
 * T2 address.
 */
static void ParseCASE() {
    PushControl(DO_CASE);                    // enter region
    MapLToT2();                              // pass through the input
    uint16_t exprT2 = SerialiseExpression(); // case switch expression
    Wr2Node(T2_CASEBLOCK, hLabels[controlSP] = NewLocalLabel(), CvtToRel(exprT2));
    ChkEndOfStmt();
}

/*
 * IterativeDoStatement – handle a T1_DOLOOP (DO var = y TO z [BY s]) header.
 *
 * Emitted IR sequence (without BY):
 *   a: serialise var               (loop variable lvalue)
 *   b: serialise expression y      (initial value)
 *   c: T2_COLONEQUALS rel(a) rel(b)
 *   d: T2_LOCALLABEL headLabel
 *   e: serialise var               (loop variable rvalue, for comparison)
 *   f: serialise expression z      (limit)
 *   g: T2_LE rel(e) rel(f)
 *   h: T2_JMPFALSE endLabel rel(g)
 *
 * With BY:
 *   i: T2_JMP byLabel              (skip step code first time)
 *   j: T2_LOCALLABEL stepLabel
 *   k: serialise var
 *   l: serialise var
 *   m: serialise expression s      (step)
 *   n: T2_ADDW rel(l) rel(m)
 *   o: T2_COLONEQUALS rel(k) rel(n)
 *      T2_JNC headLabel
 *      T2_JMP endLabel
 *      T2_LOCALLABEL byLabel       (loop body starts here)
 *
 * Error handling:
 *   ERR138 – missing index variable.
 *   ERR139 – index variable not BYTE or ADDRESS scalar.
 *   ERR140 – missing '=' after index variable.
 *   ERR141 – missing TO clause.
 */
static void IterativeDoStatement() {

    PushControl(SIMPLE_DO); // enter region, reverting to simple do if "var =" not present
    if (NotMatchTx1Item(T1_IDENTIFIER)) { // oops not do name
        Wr2Error(ERR138);                 /* MISSING INDEX VARIABLE */
        return;
    }
    GetVariable();

    info_t *pInfo = infoStack[controlSP] = info;
    if ((info->type != BYTE_T && info->type != ADDRESS_T) || (info->flag & F_ARRAY)) {
        Wr2TokError(ERR139); /* INVALID INDEX VARIABLE TYPE, NOT BYTE OR ADDRESS */
        return;
    }
    if (NotMatchTx1Item(T1_EQ)) {
        Wr2TokError(ERR140); /* MISSING '=' FOLLOWING INDEX VARIABLE */
        return;
    }
    controlStk[controlSP] = DO_ITERATIVE;          // update the region type
    uint16_t lhsT2        = WrTx2Var(pInfo);       // encode looping var
    uint16_t rhsT2        = SerialiseExpression(); // and initial expression
    Wr2Node(T2_COLONEQUALS, CvtToRel(lhsT2), CvtToRel(rhsT2));
    uint16_t headLabel = NewLocalLabel();
    hLabels[controlSP] = headLabel; // encode the looping point
    Wr2Leaf(T2_LOCALLABEL, headLabel);
    lhsT2 = WrTx2Var(pInfo); // encode looping  var

    if (MatchTx1Item(T1_TO))
        rhsT2 = SerialiseExpression(); // encode the to expression
    else {
        Wr2TokError(ERR141);           /* MISSING 'to' CLAUSE */
        rhsT2 = Wr2Leaf(T2_NUMBER, 0); // on error set to expression to 0
    }

    lhsT2              = Wr2Node(T2_LE, CvtToRel(lhsT2), CvtToRel(rhsT2)); // compare
    uint16_t endLabel  = NewLocalLabel();                                  // allocate the end label
    eLabels[controlSP] = endLabel;
    Wr2Node(T2_JMPFALSE, endLabel, CvtToRel(lhsT2)); // jmp to end when done

    if (NotMatchTx1Item(T1_BY)) // check for optional by
        return;
    uint16_t byLabel    = NewLocalLabel(); // label after step code
    byLabels[controlSP] = byLabel;
    Wr2Leaf(T2_JMP, byLabel);             // skip step code
    uint16_t stepLabel = NewLocalLabel(); // allocate step code label
    sLabels[controlSP] = stepLabel;
    Wr2Leaf(T2_LOCALLABEL, stepLabel);                                     // encode code label
    lhsT2           = WrTx2Var(pInfo);                                     // looping var
    rhsT2           = WrTx2Var(pInfo);                                     // looping var
    uint16_t stepT2 = SerialiseExpression();                               // step expression
    rhsT2           = Wr2Node(T2_ADDW, CvtToRel(rhsT2), CvtToRel(stepT2)); // loop var + step
    Wr2Node(T2_COLONEQUALS, CvtToRel(lhsT2), CvtToRel(rhsT2)); // loop var = loop var + step
    Wr2Leaf(T2_JNC, headLabel);                                // still more to do
    Wr2Leaf(T2_JMP, endLabel);                                 // all done
    Wr2Leaf(T2_LOCALLABEL, byLabel);                           // label after step code
}

/*
 * ParseEND – handle a T1_END item, closing the current block.
 *
 * Dispatches on controlStk[controlSP]:
 *
 *  DO_PROC       – sets F_DEFINED on the procedure; emits T2_ENDPROC.
 *                  ERR156 if a typed procedure has no RETURN with a value.
 *                  Restores curProc from infoStack[].
 *
 *  SIMPLE_DO     – no T2 output required.
 *
 *  DO_WHILE      – emits T2_JMP headLabel then T2_LOCALLABEL endLabel.
 *
 *  DO_CASE       – emits T2_LOCALLABEL headLabel (the end-of-case label
 *                  used by T2_CASEBLOCK) and T2_ENDCASE.
 *
 *  DO_ITERATIVE  – calls EndIterDo() to emit the increment / jump-back
 *                  sequence and the end label.
 *
 * Calls PopControl() and PopScope() to restore the enclosing context.
 */
static void ParseEND() {
    switch (controlStk[controlSP]) { // what to end
    case DO_PROC:
        info = curProc; // finish definition
        info->flag |= F_DEFINED;
        if (info->returnType && !returnsVal[controlSP]) // check if expected return
            Wr2Error(ERR156);           /* MISSING RETURN STATEMENT IN TYPED PROCEDURE */
        Wr2Simple(T2_ENDPROC);          // mark end
        curProc = infoStack[controlSP]; // restore containing proc
        break;
    case SIMPLE_DO: // nothing to do
        break;
    case DO_WHILE:
        Wr2Leaf(T2_JMP, hLabels[controlSP]);        // jump to test
        Wr2Leaf(T2_LOCALLABEL, eLabels[controlSP]); // add end label
        break;
    case DO_CASE:
        Wr2Leaf(T2_LOCALLABEL, hLabels[controlSP]); // add end label
        Wr2Simple(T2_ENDCASE);                      // mark end
        break;
    case DO_ITERATIVE:
        EndIterDo(); // write appropriate tail IR code
        break;
    }
    PopControl();
    PopScope();
}

/*
 * ParseStatement – handle a T1_STATEMENT (assignment) item.
 *
 * Calls parseAssignment() to build the assignment tree; if successful,
 * calls SerialiseParse(markedStSP) to emit the T2 records.  Checks for
 * any trailing expression tokens via ChkEndOfStmt().
 */
static void ParseStatement() {
    if (parseAssignment())
        SerialiseParse(markedStSP);
    ChkEndOfStmt();
}

/*
 * SerialiseCall – handle a T1_CALL item.
 *
 * Calls ParseCall() to build the call tree; if successful, calls
 * SerialiseParse(markedStSP) to emit the T2 records.  Checks for trailing
 * expression tokens.
 */
static void SerialiseCall() {
    if (ParseCall())
        SerialiseParse(markedStSP);
    ChkEndOfStmt();
}

/*
 * ReturnStatement – handle a T1_RETURN item.
 *
 * ERR155 if there is no enclosing procedure (RETURN in main program).
 * Otherwise examines the current procedure's returnType:
 *
 *  Expression follows (F_EXPRITEM):
 *    Untyped procedure (returnType == 0) → ERR136 (value illegal).
 *    BYTE_T return → emits T2_RETURNBYTE with the expression's relative T2.
 *    ADDRESS/WORD  → emits T2_RETURNWORD.
 *
 *  No expression:
 *    Typed procedure (returnType != 0) → ERR137 (missing value).
 *    Untyped → emits T2_RETURN.
 *
 * Sets returnsVal[controlSP] = true and calls ChkEndOfStmt().
 */
static void ReturnStatement() {
    if (!curProc) {
        Wr2Error(ERR155); /* INVALID RETURN IN MAIN PROGRAM */
        return;
    }
    info            = curProc;
    uint8_t retType = info->returnType;
    if (MatchTx2AuxFlag(F_EXPRITEM)) { /* there is an Expression item */
        UngetTx1Item();
        if (retType == 0)     // untyped procedure
            Wr2Error(ERR136); /* INVALID RETURN FOR UNTYPED PROCEDURE, VALUE ILLEGAL */
        Wr2Leaf(retType == BYTE_T ? T2_RETURNBYTE : T2_RETURNWORD, CvtToRel(SerialiseExpression()));
    } else {
        if (retType != 0)     // typed procedure
            Wr2Error(ERR137); /* MISSING VALUE IN RETURN FOR TYPED PROCEDURE */
        Wr2Simple(T2_RETURN);
    }
    returnsVal[controlSP] = true;
    ChkEndOfStmt();
}

/*
 * I8080DependentStatement – handle ENABLE, DISABLE and HALT.
 *
 * These 8080-specific statements carry no operands.  MapLToT2() passes the
 * T1_xxx item directly to the T2 stream, then ChkEndOfStmt() consumes any
 * spurious trailing tokens.
 */
static void I8080DependentStatement() {
    MapLToT2();
    ChkEndOfStmt();
}

/*
 * GotoStatement – handle a T1_GOTO item.
 *
 * Reads the target identifier via ChkIdentifier().  ERR142 if absent.
 * ERR143 if the symbol is not a LABEL_T.
 * Otherwise validates the scope:
 *   • Module-level target reached from a local scope → sets F_MODGOTO.
 *   • Module-to-module or local-to-local → emits T2_GOTO with the info
 *     index; ChkEndOfStmt() cleans up.
 *   • Cross-scope (local to a different local) → ERR144.
 */
static void GotoStatement() {
    if (NotMatchTx1Item(T1_IDENTIFIER)) // no goto identifier
        Wr2Error(ERR142);               /* MISSING IDENTIFIER FOLLOWING GOTO */
    else {
        ChkIdentifier(); // can only goto label
        if (info->type != LABEL_T)
            Wr2TokError(ERR143); /* INVALID REFERENCE FOLLOWING GOTO, NOT A LABEL */
        else {
            if (High(info->scope) == 1 &&
                High(scopeChains[scopeSP]) != 1) // goto is from local to a module level target
                info->flag |= F_MODGOTO;         // flag target as module level label
            if (High(info->scope) == 1 ||
                High(info->scope) == High(scopeChains[scopeSP])) { // its module level to module
                                                                   // level or local to local
                Wr2Leaf(T2_GOTO, ToIdx(info)); // write lex token with normalised infoOffset
                ChkEndOfStmt();
            } else
                Wr2TokError(ERR144); /* INVALID GOTO LABEL, NOT AT LOCAL OR MODULE LEVEL */
        }
    }
}

/*
 * GoStatement – handle a T1_GO item (the 'GO' of 'GO TO').
 *
 * Reads the mandatory T1_TO token (ERR145 if absent) then delegates to
 * GotoStatement() to handle the label identifier.
 */
static void GoStatement() {
    if (NotMatchTx1Item(T1_TO))
        Wr2Error(ERR145); /* MISSING 'TO' FOLLOWING 'GO' */
    GotoStatement();      // handle as normal GOTO
}

/*
 * Locator – handle a T1_AT item (AT clause of a variable declaration).
 *
 * On entry tx1Item.dataw[0] holds the info index of the variable being
 * located.  Consumes the '(' token that follows, then calls
 * RestrictedExpression() to parse the location expression.  Writes an
 * ATI_AHDR record to the attribute file (atf) containing:
 *   varInfoIdx       – info index of the located variable
 *   curStmtNum       – statement number for error reporting
 *   var.infoIdx      – info index of the identifier in the AT expression
 *   var.subscript    – array subscript (if any)
 *   var.memberSubscript – structure member subscript (if any)
 *   var.val          – the additive offset (+/- delta)
 * Finally calls ExpectRParen(ERR146) to consume the closing ')'.
 */
static void Locator() {
    index_t varInfoIdx = tx1Item.dataw[0]; // info for variable

    GetTx1Item();           // skip '('
    RestrictedExpression(); // get the AT Expression
    WrAtByte(ATI_AHDR);
    WrAtWord(varInfoIdx);          // info index
    WrAtWord(curStmtNum);          // statement number
    WrAtWord(var.infoIdx);         // info offset for identifier in Expression
    WrAtWord(var.subscript);       // any array index
    WrAtWord(var.memberSubscript); // ditto for any structure element
    WrAtWord(var.val);             // the +/- delta
    ExpectRParen(ERR146);          /* MISSING ') ' AFTER 'AT' RESTRICTED Expression */
}

/*
 * Initialization – handle a T1_DATA or T1_INITIAL item.
 *
 * tx1Item.dataw[0] holds the info index of the variable being initialised.
 * Advances to the next item (the opening '('), then calls
 * InitialValueList() to parse the initialiser list and return the number of
 * values supplied.  If the variable has F_STARDIM (star dimension), updates
 * info->dim to the count so later phases know the actual array size.
 */
void Initialization() {
    index_t varInfoIdx = tx1Item.dataw[0];       // info of variable being initialised
    GetTx1Item();                                // get next item
    uint16_t cnt = InitialValueList(varInfoIdx); // parse the initialisation list
    info         = FromIdx(varInfoIdx);
    if (info && (info->flag & F_STARDIM)) // update * dim
        info->dim = cnt;
}

/*
 * ParseLexItems – top-level phase-2 statement dispatcher.
 *
 * Initialises controlSP and enters a loop that reads T1_xxx items and
 * dispatches each to the appropriate handler until T1_EOF is seen.
 *
 * Item        Handler / action
 * ----------  ----------------
 * T1_STMTCNT  ParseStmtcnt()       – sync statement counter, pass through
 * T1_SCOPE    PushScope()          – enter new lexical scope
 * T1_END      ParseEND()           – close DO/PROC block
 * T1_IF       ParseIf()            – serialise IF condition + T2_JMPFALSE
 * T1_DOLOOP   IterativeDoStatement() + ChkEndOfStmt()
 * T1_WHILE    ParseWhile()         – DO WHILE header
 * T1_DO       PushControl(SIMPLE_DO)
 * T1_PROCEDURE ParseProcedure()    – procedure header
 * T1_CASE     ParseCASE()          – DO CASE header
 * T1_STATEMENT ParseStatement()    – assignment
 * T1_CALL     SerialiseCall()      – CALL statement
 * T1_RETURN   ReturnStatement()
 * T1_GOTO     GotoStatement()
 * T1_GO       GoStatement()
 * T1_SEMICOLON (no action)
 * T1_ENABLE /
 * T1_DISABLE /
 * T1_HALT     I8080DependentStatement()
 * T1_AT       Locator()
 * T1_DATA /
 * T1_INITIAL  Initialization()
 * T1_LABELDEF /
 * T1_LOCALLABEL /
 * T1_JMP /
 * T1_JMPFALSE /
 * T1_CASELABEL MapLToT2()          – pass through unchanged
 * T1_EXTERNAL  set F_DECLARED | F_DEFINED on the procedure info
 * (other)      silently ignored
 */
void ParseLexItems() {
    controlSP = 0;
    GetTx1Item();
    while (tx1Item.type != T1_EOF) {
        info = NULL;
        switch (tx1Item.type) {
        case T1_STMTCNT:
            ParseStmtcnt();
            break;
        case T1_SCOPE: // T1_SCOPE newScope
            PushScope(tx1Item.dataw[0]);
            break;
        case T1_END:
            ParseEND();
            break;
        case T1_IF:
            ParseIf();
            break;
        case T1_DOLOOP:
            IterativeDoStatement();
            ChkEndOfStmt();
            break;
        case T1_WHILE:
            ParseWhile();
            break;
        case T1_DO:
            PushControl(SIMPLE_DO);
            break;
        case T1_PROCEDURE:
            ParseProcedure();
            break;
        case T1_CASE:
            ParseCASE();
            break;
        case T1_STATEMENT:
            ParseStatement();
            break;
        case T1_CALL:
            SerialiseCall();
            break;
        case T1_RETURN:
            ReturnStatement();
            break;
        case T1_GOTO:
            GotoStatement();
            break;
        case T1_GO:
            GoStatement();
            break;
        case T1_SEMICOLON:
            break;
        case T1_ENABLE:
        case T1_DISABLE:
        case T1_HALT:
            I8080DependentStatement();
            break;
        case T1_AT:
            Locator();
            break;
        case T1_DATA:
        case T1_INITIAL:
            Initialization();
            break;
        default:
            break; /* simple items */
        case T1_LABELDEF:
        case T1_LOCALLABEL:
        case T1_JMP:
        case T1_JMPFALSE:
        case T1_CASELABEL:
            MapLToT2();
            break;
        case T1_EXTERNAL:
            info = FromIdx(tx1Item.dataw[0]);
            info->flag |= F_DECLARED | F_DEFINED; // flag proc as declared & defined
            break;
        }
        GetTx1Item();
    }
}