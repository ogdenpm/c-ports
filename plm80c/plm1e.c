/****************************************************************************
 *  plm1e.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

bool parseAssignment() {
    ResetStacks();
    byte assignCnt = 0;
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
            PushComplexOperand(I_COLONEQUALS, assignCnt + 1);
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

byte ParseCall() {
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
        PushSimpleOperand(I_IDENTIFIER, ToIdx(info));
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

void Expression() {
    ResetStacks();
    PushParse(0);
    ExpressionStateMachine();
    MoveOperandToTree();
    markedStSP = stSP;
}

static void MoveHead(word node, word t2Cnt) {
    //   sStack[node].icode = 255;
    tree[node].val = t2Cnt;
}

static void SerialiseSimpleNode(word node) {
    byte iCode = tree[node].icode;
    word t2Cnt;
    if (iCode == I_IDENTIFIER)
        t2Cnt = Wr2Leaf(T2_IDENTIFIER, tree[node].infoIdx); // info
    else if (iCode == I_NUMBER) {
        t2Cnt = Wr2Leaf(High(tree[node].val) ? T2_BIGNUMBER : T2_NUMBER,
                        tree[node].val); // big or small number
    } else if (iCode == I_INPUT)         // port
        t2Cnt = Wr2Leaf(T2_INPUT, tree[node].val);
    else if (iCode == I_LENGTH || iCode == I_LAST || iCode == I_SIZE)
        t2Cnt = Wr2Leaf(iToTx2Map[iCode], tree[node].infoIdx); // info
    else
        t2Cnt = Wr2Simple(iToTx2Map[iCode]); // no args
    MoveHead(node, t2Cnt);
}

static void SerialiseParse0() { // top level serialise
    byte icode;

    word node = PopParse();

    if ((icode = tree[node].icode) == I_OUTPUT)
        return;
    if (tree[node].right == 0) // no supplementary args
        SerialiseSimpleNode(node);
    else {
        PushParse(node); // resave node
        if (icode == I_CALL)
            PushParse(3);
        else if (icode == I_CALLVAR) {
            PushParse(6);
            PushParse(tree[node].nodeIdx);
            PushParse(0);
        } else if (icode == I_COLONEQUALS)
            PushParse(9);
        else if (icode == I_MOVE) {
            PushParse(14);
            PushParse(tree[node].nodeIdx);
            PushParse(0); // recurse
        } else if (icode == I_BYTEINDEX || icode == I_WORDINDEX) {
            PushParse(8);
            PushParse(2);
            PushParse(tree[node].nodeIdx);
            PushParse(1);
        } else {
            PushParse(13);
            PushParse(tree[node].right);     /* num args */
            PushParse(tree[node].nodeIdx); /* loc of args */
            PushParse(1);
        }
    }
}

static void SerialiseParse1() { /* serialise 1 node */
    word node = parseStack[parseSP];
    PushParse(2);    /* flag to check for more nodes */
    PushParse(node); /* serialise this node */
    PushParse(0);
}

static void SerialiseParse2() { /* check for any more leaves */
    if (--parseStack[parseSP - 1] == 0) {
        PopParse();
        PopParse();
    } else {
        parseStack[parseSP]++;
        PushParse(1);
    }
}

static void SerialiseParse3() {                              /* parse args */
    word argList = parseStack[parseSP];                      // the call node
    PushParse(5);                                            /* final call wrap up */
    if (tree[argList].right > 1) {                             /* any args */
        PushParse(tree[argList].right - 1);                    /* num args  */
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

// serialise the stack arguments
static void SerialiseParse4() {
    info         = FromIdx(parseStack[parseSP]);
    word argList = parseStack[parseSP - 1];
    byte argCnt  = (byte)parseStack[parseSP - 2];
    if (argCnt > 2) { /* all bar first 2 args to stack */
        word t2Cnt;
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

// finalise call

static void SerialiseParse5() {
    word node    = PopParse();
    word argList = tree[node].nodeIdx;
    byte argCnt  = tree[node].right - 1;
    word relBC   = 0;
    word relDE   = 0;
    if (argCnt > 1) {
        relBC = CvtToRel(tree[argList + argCnt - 1].nodeIdx);
        relDE = CvtToRel(tree[argList + argCnt].nodeIdx);
    } else if (argCnt > 0)
        relBC = CvtToRel(tree[argList + argCnt].nodeIdx);
    MoveHead(node, Wr2ArgNode(T2_CALL, relBC, relDE, tree[argList].nodeIdx));
}

// serialise call var[(...)]

static void SerialiseParse6() {
    Wr2Simple(T2_BEGCALL);
    word node = parseStack[parseSP];
    PushParse(7); // finalise call var
    byte argCnt = tree[node].right - 1;
    if (argCnt) {
        PushParse(argCnt); // count of args
        word argList = tree[node].nodeIdx + 1;
        PushParse(argList);
        PushParse(0); /* simple T2_STKARG */
        PushParse(4); /* serialise stack args */
        PushParse(argList);
        PushParse(0);
    }
}

// finalise call var[(....)]

static void SerialiseParse7() {

    word node    = PopParse();
    byte argCnt  = tree[node].right - 1; // number of args (-1 to zero base)
    word argList = tree[node].nodeIdx; // location of args
    word relBC = 0, relDE = 0;
    if (argCnt > 1) { // 2 or more args
        relBC = CvtToRel(tree[argList + argCnt - 1].nodeIdx);
        relDE = CvtToRel(tree[argList + argCnt].nodeIdx);
    } else if (argCnt > 0) // 1 arg
        relBC = CvtToRel(tree[argList + argCnt].nodeIdx);
    word t2Cnt = Wr2ArgNode(T2_CALLVAR, relBC, relDE, CvtToRel(tree[argList].nodeIdx));
    MoveHead(node, t2Cnt);
}

static void SerialiseParse8() {
    word node    = PopParse();
    byte icode   = tree[node].icode;
    word leafIdx = tree[node].nodeIdx;
    word t2Cnt   = Wr2ArgNode(iToTx2Map[icode], CvtToRel(tree[leafIdx].nodeIdx),
                              CvtToRel(tree[leafIdx + 1].nodeIdx), tree[leafIdx + 2].nodeIdx);
    MoveHead(node, t2Cnt);
}

static void SerialiseParse9() { // parse :=
    word node = parseStack[parseSP];
    PushParse(10);                                      /* post serialise RHS */
    PushParse(tree[node].nodeIdx + tree[node].right - 1); // RHS node, i.e. past assignments
    PushParse(0);
}

static void SerialiseParse10() {
    word node = parseStack[parseSP];
    PushParse(12);                        /* mark LHS as used at end */
    byte assignCnt  = tree[node].right - 1; /* num LHS */
    word assignBase = tree[node].nodeIdx; /* base LHS */
    PushParse(assignCnt);
    PushParse(assignBase);
    PushParse(assignBase + assignCnt); /* RHS */
    PushParse(11);                     /* after serialised leaf */
    PushParse(assignBase);             /* do the leaf */
    PushParse(0);
}

static void SerialiseParse11() { /* do one assignment */

    word assignIdx = parseStack[parseSP - 1]; // lvalue
    word rhsIdx    = parseStack[parseSP];     // rvalue
    if (tree[assignIdx].icode == I_OUTPUT) {
        word numberT2 = Wr2Leaf(T2_NUMBER, tree[assignIdx].val); // serialise the port number
        Wr2Node(T2_OUTPUT, CvtToRel(numberT2), CvtToRel(tree[rhsIdx].nodeIdx)); // and the output
    } else
        Wr2Node(T2_COLONEQUALS, CvtToRel(tree[assignIdx].nodeIdx), CvtToRel(tree[rhsIdx].nodeIdx));

    if (--parseStack[parseSP - 2] == 0) { // assignment count 0 -> done
        PopParse();                       // cleanup the control variables
        PopParse();
        PopParse();
    } else {                                  /* no so do another */
        parseStack[parseSP - 1] = ++assignIdx;
        PushParse(11);                        /* state 11 after serialise */
        PushParse(assignIdx); /* serialise next lvalue */
        PushParse(0);
    }
}

static void SerialiseParse12() { /* mark LHS as used */
    word node  = PopParse();
    word t2Cnt = tree[tree[node].nodeIdx + tree[node].right - 1].nodeIdx;
    MoveHead(node, t2Cnt);  // move lhs to point to the rvalue tree node
}

static void SerialiseParse13() { /* binary or unary op */

    word node  = PopParse();
    word sIdx  = tree[node].nodeIdx;
    byte iCode = iToTx2Map[tree[node].icode];
    word t2Cnt;
    if (tree[node].right == 1)
        t2Cnt = Wr2Leaf(iCode, CvtToRel(tree[sIdx].nodeIdx));
    else
        t2Cnt = Wr2Node(iCode, CvtToRel(tree[sIdx].nodeIdx), CvtToRel(tree[sIdx + 1].nodeIdx));
    MoveHead(node, t2Cnt);
}

static void SerialiseParse14() {
    word p = tree[parseStack[parseSP]].nodeIdx;
    /* emit the count leaf */
    word t2Cnt = Wr2Leaf(T2_BEGMOVE, CvtToRel(tree[p].nodeIdx));
    MoveHead(p, t2Cnt);
    PushParse(15); /* Move() post serialise */
    PushParse(2);  /* serialise the address leaves */
    PushParse(p + 1);
    PushParse(1);
}

static void SerialiseParse15() { /* rest of Move() */

    word node  = PopParse();
    word sIdx  = tree[node].nodeIdx;
    word t2Cnt = Wr2ArgNode(T2_MOVE, CvtToRel(tree[sIdx + 1].t2Cnt), CvtToRel(tree[sIdx + 2].t2Cnt),
                            CvtToRel(tree[sIdx].t2Cnt));
    MoveHead(node, t2Cnt);
}

word SerialiseParse(word stmtSp) {
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

static byte controlStk[20];
static bool returnsVal[20];
static info_t *infoStack[20];
static word hLabels[20];
static word eLabels[20];
static word sLabels[20];
static word byLabels[20];
static word controlSP;

static void PushScope(word arg1w) {
    if (scopeSP == 34)
        fatal_ov1(ERR164); /* COMPILER Error: SCOPE STACK OVERFLOW */
    else
        scopeChains[++scopeSP] = arg1w;
}

static void PopScope() {
    if (scopeSP == 0)
        fatal_ov1(ERR165); /* COMPILER Error: SCOPE STACK UNDERFLOW */
    else
        scopeSP--;
}

static void PushControl(byte controlType) {
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

static void PopControl() {
    if (controlSP == 0)
        fatal_ov1(ERR167); /* COMPILER Error: CONTROL STACK UNDERFLOW */
    else if (controlStk[controlSP--])
        returnsVal[controlSP] |= returnsVal[controlSP + 1];
}

static word NewLocalLabel() {
    return ++localLabelCnt;
}

static word WrTx2VarId(info_t *varInfo) {
    word varT2 = Wr2Leaf(T2_IDENTIFIER, ToIdx(varInfo));
    if ((varInfo->flag & F_MEMBER)) {
        word memberNode = Wr2Leaf(T2_IDENTIFIER, ToIdx(varInfo->parent));
        varT2           = Wr2Node(T2_MEMBER, CvtToRel(memberNode), CvtToRel(varT2));
        info            = varInfo->parent;
    } else
        info = varInfo;
    return varT2;
}

static word WrTx2Var(info_t *varInfo) {
    word varT2 = WrTx2VarId(varInfo);
    if (!(info->flag & F_BASED))
        return varT2;
    return Wr2Node(T2_BASED, CvtToRel(WrTx2VarId(info->baseInfo)), CvtToRel(varT2));
}

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
    Serialise end of DO x = y to z [by s]
    if no by clause
        if x is byte
            T2_LOCALLABEL newlabel
        a: serialise x
        b: serialise x
        c: T2_NUMBER 1
        if x is byte
            d: T2_PLUSSIGN rel(b) rel(c)
            e: T2_COLONEQUALS rel(a) rel(d)
            f: T2_JNZ headLoop
        else
            d: T2_ADDW rel(b) rel(c)
            e: T2_COLONEQUALS rel(a) rel(d)
            f: T2_JNC headLoop
    else
        T2_JMP  stepLoop
    T2_LOCALLABEL endLoop
*/

static void EndIterDo() {
    word stepLoop;

    info          = infoStack[controlSP];
    word headLoop = hLabels[controlSP];
    word endLoop  = eLabels[controlSP];
    if ((stepLoop = sLabels[controlSP]) == 0) {
        if (info->type == BYTE_T)
            Wr2Leaf(T2_LOCALLABEL, NewLocalLabel());
        word loopVarT2  = WrTx2Var(info);
        word loopTmpT2  = WrTx2Var(info);
        word loopStepT2 = Wr2Leaf(T2_NUMBER, 1);
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

static word SerialiseExpression() {
    Expression();
    return SerialiseParse(markedStSP);
}

static void SerialiseIF() {
    word exprT2 = SerialiseExpression();
    ChkEndOfStmt();
    if (MatchTx1Item(T1_JMPFALSE))
        Wr2Node(T2_JMPFALSE, tx1Item.dataw[0], CvtToRel(exprT2));
    else
        fatal_ov1(ERR168); /* COMPILER ERROR: BRANCH MISSING IN 'IF' STATEMENT */
}

static void ParseStmtcnt() {
    stmtT2Cnt = 0;                 // reset stmt item counter
    MapLToT2();                    // pass through stmt info
    curStmtNum = tx1Item.dataw[0]; // sync up stmt number
}

static void ParseIf() {
    SerialiseIF();
}

// T1_PROCEDURE info

static void ParseProcedure() {
    MapLToT2();                                 // map through the input
    PushControl(DO_PROC);                       // enter region
    infoStack[controlSP] = curProc;             // save current proc
    curProc = info = FromIdx(tx1Item.dataw[0]); // proc info
    info->flag |= F_DECLARED;                   // flag declared
}

/*
    Serialise WHILE
    do while (cond);

    a: T2_LOCALLABEL headLabel
    b: Serialise expression cond
    c: T2_JMPFALSE endlabel, rel(b)

*/
static void ParseWhile() {
    PushControl(DO_WHILE);                                        // enter region
    Wr2Leaf(T2_LOCALLABEL, hLabels[controlSP] = NewLocalLabel()); // loop label
    word endLabel = eLabels[controlSP] = NewLocalLabel();         // jmp to end when done
    Wr2Node(T2_JMPFALSE, endLabel, CvtToRel(SerialiseExpression()));
    ChkEndOfStmt();
}

/*
    Serialise Case block
    a: serialise expression
       T2_CASEBLOCK endlabel rel(a)
*/
static void ParseCASE() {
    PushControl(DO_CASE);                // enter region
    MapLToT2();                          // pass through the input
    word exprT2 = SerialiseExpression(); // case switch expression
    Wr2Node(T2_CASEBLOCK, hLabels[controlSP] = NewLocalLabel(), CvtToRel(exprT2));
    ChkEndOfStmt();
}

/*
    write the tree for a do x = y to z [by s]

    a:  serialise var x
    b:  serialise expression y
    c:  T2_COLONEQUALS rel(a) rel(b)
    d:  T2_LOCALLABEL headLabel
    e:  serialise var x
    f:  serialise expression z
    g:  T2_LE rel(e) rel(f)
    h:  T2_JMPFALSE endLabel rel(g)

    if BY
    i:  T2_JMP byLabel
    j:  T2_LOCALLABEL stepLabel
    k:  serialise var x
    l:  serialise var x
    m:  serialise expression s
    n:  T2_ADDW rel(l) rel(m)
    o:  T2_COLONEQUALS rel(k) rel(n)
        T2_JNC headlabel
        T2_JMP endLabel
        T2_LOCALLABEL byLabel

    local labels allocated in order headLabel, endLabel, byLabel, stepLabel
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
    word lhsT2            = WrTx2Var(pInfo);       // encode looping var
    word rhsT2            = SerialiseExpression(); // and initial expression
    Wr2Node(T2_COLONEQUALS, CvtToRel(lhsT2), CvtToRel(rhsT2));
    word headLabel     = NewLocalLabel();
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
    word endLabel      = NewLocalLabel();                                  // allocate the end label
    eLabels[controlSP] = endLabel;
    Wr2Node(T2_JMPFALSE, endLabel, CvtToRel(lhsT2)); // jmp to end when done

    if (NotMatchTx1Item(T1_BY)) // check for optional by
        return;
    word byLabel        = NewLocalLabel(); // label after step code
    byLabels[controlSP] = byLabel;
    Wr2Leaf(T2_JMP, byLabel);             // skip step code
    word stepLabel     = NewLocalLabel(); // allocate step code label
    sLabels[controlSP] = stepLabel;
    Wr2Leaf(T2_LOCALLABEL, stepLabel);                                 // encode code label
    lhsT2       = WrTx2Var(pInfo);                                     // looping var
    rhsT2       = WrTx2Var(pInfo);                                     // looping var
    word stepT2 = SerialiseExpression();                               // step expression
    rhsT2       = Wr2Node(T2_ADDW, CvtToRel(rhsT2), CvtToRel(stepT2)); // loop var + step
    Wr2Node(T2_COLONEQUALS, CvtToRel(lhsT2), CvtToRel(rhsT2));         // loop var = loop var + step
    Wr2Leaf(T2_JNC, headLabel);                                        // still more to do
    Wr2Leaf(T2_JMP, endLabel);                                         // all done
    Wr2Leaf(T2_LOCALLABEL, byLabel);                                   // label after step code
}

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

static void ParseStatement() {
    if (parseAssignment())
        SerialiseParse(markedStSP);
    ChkEndOfStmt();
}

static void SerialiseCall() {
    if (ParseCall())
        SerialiseParse(markedStSP);
    ChkEndOfStmt();
}

static void ReturnStatement() // return already seen
{
    if (!curProc) {
        Wr2Error(ERR155); /* INVALID RETURN IN MAIN PROGRAM */
        return;
    }
    info         = curProc;
    byte retType = info->returnType;
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

// <8080 dependent statement>
static void I8080DependentStatement() {
    MapLToT2();
    ChkEndOfStmt();
}

// <goto statement>
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

// <goto statement>
static void GoStatement() {
    if (NotMatchTx1Item(T1_TO))
        Wr2Error(ERR145); /* MISSING 'TO' FOLLOWING 'GO' */
    GotoStatement();      // handle as normal GOTO
}

// <locator> (AT already parsed)
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

void Initialization() {
    index_t varInfoIdx = tx1Item.dataw[0];   // info of variable being initialised
    GetTx1Item();                            // get next item
    word cnt = InitialValueList(varInfoIdx); // parse the initialisation list
    info     = FromIdx(varInfoIdx);
    if (info && (info->flag & F_STARDIM)) // update * dim
        info->dim = cnt;
}

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
