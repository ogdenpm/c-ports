/****************************************************************************
 *  plm1e.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

bool parseAssignment() {
    ResetStacks();
    byte assignCnt = 0;
    while (1) {
        if (NotMatchTx1Item(L_IDENTIFIER)) {
            WrTx2ExtError(ERR128); /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
            return 0;
        }
        ChkIdentifier();
        PushParse(11);
        ExpressionStateMachine();
        if (ChkRValue(exSP)) {
            WrTx2ExtError(ERR128); /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
            ExprPop();
        } else
            assignCnt++;
        if (NotMatchTx1Item(L_COMMA))
            break;
    }

    if (MatchTx1Item(L_EQ)) {
        PushParse(0); // <expression>
        ExpressionStateMachine();
        if (assignCnt) {
            ExprMakeNode(I_COLONEQUALS, assignCnt + 1);
            MoveExpr2Stmt();
            markedStSP = stSP;
            return true;
        } else
            return false;
    } else {
        WrTx2ExtError(ERR116); /* MISSING '=' IN ASSIGNMENT STATEMENT */
        return false;
    }
}

byte Sub_59D4() {
    ResetStacks();
    if (NotMatchTx1Item(L_IDENTIFIER)) {
        WrTx2ExtError(ERR117); /* MISSING procedure NAME IN call STATEMENT */
        return false;
    }
    GetVariable();

    if (info->type == PROC_T) {
        Sub_50D5();
        if (info->returnType)
            WrTx2ExtError(ERR129); /* ILLEGAL 'call' WITH TYPED procedure */
        ExprPush2(I_IDENTIFIER, infoIdx);
        PushParse(info->paramCnt);
        PushOperator(0);
        PushParse(16);
        if (MatchTx1Item(L_LPAREN)) {
            PushParse(15);
            PushParse(0);
        }
    } else if (info->type == BUILTIN_T) {
        if (info->returnType) {
            WrTx2ExtError(ERR129); /* ILLEGAL 'call' WITH TYPED procedure */
            return false;
        }
        PushParse(builtinsMap[info->builtinId]);
        PushParse(info->paramCnt);
        PushOperator(0);
        PushParse(17);
        if (MatchTx1Item(L_LPAREN)) {
            PushParse(15);
            PushParse(0);
        }
    } else if (info->type != ADDRESS_T || (info->flag & F_ARRAY)) {
        WrTx2ExtError(ERR118); /* INVALID INDIRECT CALL, IDENTIFIER NOT AN ADDRESS SCALAR */
        return false;
    } else {
        Sub_4D2C();
        PushOperator(0);
        PushParse(18);
        if (MatchTx1Item(L_LPAREN)) {
            PushParse(15);
            PushParse(0);
        }
    }
    ExpressionStateMachine();
    MoveExpr2Stmt();
    markedStSP = stSP;
    return true;
}

void Expression() {
    ResetStacks();
    PushParse(0);
    ExpressionStateMachine();
    MoveExpr2Stmt();
    markedStSP = stSP;
}

// lifted to file scope
static word topNode;

static void RedirectNode(word node, word t2Cnt) {
    sStack[node].icode = 255;
    sStack[node].val   = t2Cnt;
}

static void Sub_5BF5(word node) {
    byte iCode = sStack[node].icode;
    word t2Cnt;
    if (iCode == I_IDENTIFIER)
        t2Cnt = WrTx2Item1Arg(T2_IDENTIFIER, sStack[node].infoIdx);
    else if (iCode == I_NUMBER) {
        t2Cnt = WrTx2Item1Arg(High(sStack[node].val) ? T2_BIGNUMBER : T2_NUMBER, sStack[node].val);
    } else if (iCode == I_INPUT)
        t2Cnt = WrTx2Item1Arg(T2_INPUT, sStack[node].val);
    else if (iCode == I_LENGTH || iCode == I_LAST || iCode == I_SIZE)
        t2Cnt = WrTx2Item1Arg(icodeToTx2Map[iCode], sStack[node].infoIdx);
    else
        t2Cnt = WrTx2Item(icodeToTx2Map[iCode]);
    RedirectNode(node, t2Cnt);
}

static void SerialiseParse0() {
    byte icode;

    topNode = PopParse();

    if ((icode = sStack[topNode].icode) == I_OUTPUT)
        return;
    if (sStack[topNode].op2 == 0)
        Sub_5BF5(topNode);
    else {
        PushParse(topNode);
        if (icode == I_CALL)
            PushParse(3);
        else if (icode == I_CALLVAR) {
            PushParse(6);
            PushParse(sStack[topNode].sNodeIdx);
            PushParse(0);
        } else if (icode == I_COLONEQUALS)
            PushParse(9);
        else if (icode == I_MOVE) {
            PushParse(14);
            PushParse(sStack[topNode].sNodeIdx);
            PushParse(0);
        } else if (icode == I_BYTEINDEX || icode == I_WORDINDEX) {
            PushParse(8);
            PushParse(2);
            PushParse(sStack[topNode].sNodeIdx);
            PushParse(1);
        } else {
            PushParse(13);
            PushParse(sStack[topNode].op2);      /* num args */
            PushParse(sStack[topNode].sNodeIdx); /* loc of args */
            PushParse(1);
        }
    }
}

static void SerialiseParse1() { /* serialise 1 leaf) check */
    topNode = parseStack[parseSP];
    PushParse(2);       /* flag to check for more leaves */
    PushParse(topNode); /* serialise this leaf */
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

static void SerialiseParse3() { /* parse args */
    topNode = parseStack[parseSP];
    PushParse(5);                                          /* final call wrap up */
    if (sStack[topNode].op2 > 1) {                         /* any args */
        PushParse(sStack[topNode].op2 - 1);                /* num args  */
        PushParse(sStack[topNode].sNodeIdx + 1);           /* index of arg info */
        SetInfo(sStack[sStack[topNode].sNodeIdx].infoIdx); /* from proc info adv to arginfo */
        AdvNxtInfo();
        PushParse(infoIdx);
        PushParse(4);
        PushParse(sStack[topNode].sNodeIdx + 1); /* index of arg info */
        PushParse(0);
    }
    WrTx2Item(T2_BEGCALL);
}

static void SerialiseParse4() {
    SetInfo(parseStack[parseSP]);
    topNode     = parseStack[parseSP - 1];
    byte argCnt = (byte)parseStack[parseSP - 2];
    if (argCnt > 2) { /* all bar first 2 args to stack */
        word t2Cnt;
        if (!info)
            t2Cnt = WrTx2Item1Arg(T2_STKARG, CvtToRel(sStack[topNode].sNodeIdx));
        else {
            t2Cnt = WrTx2Item1Arg(info->type == BYTE_T ? T2_STKBARG : T2_STKWARG,
                                  CvtToRel(sStack[topNode].sNodeIdx));
            AdvNxtInfo();
            parseStack[parseSP] = infoIdx;
        }
        RedirectNode(topNode, t2Cnt);
    }
    argCnt--;
    if (argCnt == 0) { /* all done, clear working data */
        PopParse();
        PopParse();
        PopParse();
    } else {
        parseStack[parseSP - 2] = argCnt;
        parseStack[parseSP - 1] = ++topNode;
        PushParse(4);
        PushParse(topNode); /* serialise the arg */
        PushParse(0);
    }
}

static void SerialiseParse5() {
    word p, q, r;
    byte i;

    topNode = PopParse();
    r       = sStack[topNode].sNodeIdx;
    i       = sStack[topNode].op2 - 1;
    p = q = 0;
    if (i > 1) {
        p = CvtToRel(sStack[r + i - 1].sNodeIdx);
        q = CvtToRel(sStack[r + i].sNodeIdx);
    } else if (i > 0)
        p = CvtToRel(sStack[r + i].sNodeIdx);
    RedirectNode(topNode, WrTx2Item3Arg(T2_CALL, p, q, sStack[r].sNodeIdx));
}

static void SerialiseParse6() {
    byte i;
    word p;
    WrTx2Item(T2_BEGCALL);
    topNode = parseStack[parseSP];
    PushParse(7);
    if ((i = sStack[topNode].op2 - 1) != 0) {
        PushParse(i);
        p = sStack[topNode].sNodeIdx + 1;
        PushParse(p);
        PushParse(0); /* no arg info */
        PushParse(4);
        PushParse(p);
        PushParse(0);
    }
}

static void SerialiseParse7() {

    topNode         = PopParse();
    byte lastArgIdx = sStack[topNode].op2 - 1;
    word relArgs    = sStack[topNode].sNodeIdx;
    word relBC = 0, relDE = 0;
    if (lastArgIdx > 1) {
        relBC = CvtToRel(sStack[relArgs + lastArgIdx - 1].sNodeIdx);
        relDE = CvtToRel(sStack[relArgs + lastArgIdx].sNodeIdx);
    } else if (lastArgIdx > 0)
        relBC = CvtToRel(sStack[relArgs + lastArgIdx].sNodeIdx);
    word t2Cnt = WrTx2Item3Arg(T2_CALLVAR, relBC, relDE, CvtToRel(sStack[relArgs].sNodeIdx));
    RedirectNode(topNode, t2Cnt);
}

static void SerialiseParse8() {
    topNode    = PopParse();
    byte icode = sStack[topNode].icode;
    word sIdx  = sStack[topNode].sNodeIdx;
    word t2Cnt = WrTx2Item3Arg(icodeToTx2Map[icode], CvtToRel(sStack[sIdx].sNodeIdx),
                               CvtToRel(sStack[sIdx + 1].sNodeIdx), sStack[sIdx + 2].sNodeIdx);
    RedirectNode(topNode, t2Cnt);
}

static void SerialiseParse9() {
    topNode = parseStack[parseSP];
    PushParse(10); /* post serialise LHS */
    PushParse(sStack[topNode].sNodeIdx + sStack[topNode].op2 - 1);
    PushParse(0);
}

static void SerialiseParse10() {
    byte i;
    word p;

    topNode = parseStack[parseSP];
    PushParse(12);                /* mark LHS as used at end */
    i = sStack[topNode].op2 - 1;  /* num RHS */
    p = sStack[topNode].sNodeIdx; /* base RHS */
    PushParse(i);
    PushParse(p);
    PushParse(p + i); /* LHS */
    PushParse(11);    /* after serialised leaf */
    PushParse(p);     /* do the leaf */
    PushParse(0);
}

static void SerialiseParse11() { /* do one RHS assignment */

    word lhsIdx;

    byte lhsCnt = (byte)parseStack[parseSP - 2];
    word rhsIdx = parseStack[parseSP];
    if (sStack[lhsIdx = parseStack[parseSP - 1]].icode == I_OUTPUT) {
        word numberT2 = WrTx2Item1Arg(T2_NUMBER, sStack[lhsIdx].val);
        WrTx2Item2Arg(T2_OUTPUT, CvtToRel(numberT2), CvtToRel(sStack[rhsIdx].sNodeIdx));
    } else
        WrTx2Item2Arg(T2_COLONEQUALS, CvtToRel(sStack[lhsIdx].sNodeIdx),
                      CvtToRel(sStack[rhsIdx].sNodeIdx));

    if (--lhsCnt == 0) { /* all done */
        PopParse();
        PopParse();
        PopParse();
    } else { /* no so do another */
        parseStack[parseSP - 2] = lhsCnt;
        parseStack[parseSP - 1] = ++lhsIdx;
        PushParse(11);     /* state 11 after serialise */
        PushParse(lhsIdx); /* serialise leaf */
        PushParse(0);
    }
}

static void SerialiseParse12() { /* mark LHS as used */
    topNode = PopParse();
    word t2Cnt   = sStack[sStack[topNode].sNodeIdx + sStack[topNode].op2 - 1].sNodeIdx;
    RedirectNode(topNode, t2Cnt);
}

static void SerialiseParse13() { /* binary or unary op */

    topNode    = PopParse();
    word sIdx  = sStack[topNode].sNodeIdx;
    byte iCode = icodeToTx2Map[sStack[topNode].icode];
    word t2Cnt;
    if (sStack[topNode].op2 == 1)
        t2Cnt = WrTx2Item1Arg(iCode, CvtToRel(sStack[sIdx].sNodeIdx));
    else
        t2Cnt = WrTx2Item2Arg(iCode, CvtToRel(sStack[sIdx].sNodeIdx),
                              CvtToRel(sStack[sIdx + 1].sNodeIdx));
    RedirectNode(topNode, t2Cnt);
}

static void SerialiseParse14() {
    word p;
    p = sStack[topNode = parseStack[parseSP]].sNodeIdx;
    /* emit the count leaf */
    word t2Cnt = WrTx2Item1Arg(T2_BEGMOVE, CvtToRel(sStack[p].sNodeIdx));
    RedirectNode(p, t2Cnt);
    PushParse(15); /* Move() post serialise */
    PushParse(2);  /* serialise the address leaves */
    PushParse(p + 1);
    PushParse(1);
}

static void SerialiseParse15() { /* rest of Move() */

    topNode   = PopParse();
    word sIdx = sStack[topNode].sNodeIdx;
    word t2Cnt     = WrTx2Item3Arg(T2_MOVE, CvtToRel(sStack[sIdx + 1].sNodeIdx),
                              CvtToRel(sStack[sIdx + 2].sNodeIdx), CvtToRel(sStack[sIdx].sNodeIdx));
    RedirectNode(topNode, t2Cnt);
}

word SerialiseParse(word arg1w) {
    byte i;
    info_t *savInfo = info;

    parseSP         = 0;
    PushParse(arg1w);
    PushParse(0);
    while (parseSP != 0) {
        i = (byte)parseStack[parseSP--];
        switch (i) {
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
    infoIdx = ToIdx(info = savInfo);
    return sStack[arg1w].sNodeIdx;
}

static byte controlStk[20];
static bool returnsVal[20];
static info_t *infoStack[20];
static word hNodes[20];
static word eNodes[20];
static word sNodes[20];
static word byNodes[20];
static word controlSP;

static void PushScope(word arg1w) {
    if (scopeSP == 0x22)
        FatalError_ov1(ERR164); /* COMPILER Error: SCOPE STACK OVERFLOW */
    else
        scopeChains[++scopeSP] = arg1w;
}

static void PopScope() {
    if (scopeSP == 0)
        FatalError_ov1(ERR165); /* COMPILER Error: SCOPE STACK UNDERFLOW */
    else
        scopeSP--;
}

static void PushControl(byte controlType) {
    if (controlSP == 19)
        FatalError_ov1(ERR84); /*  LIMIT EXCEEDED: BLOCK NESTING */
    else {
        controlStk[++controlSP] = controlType;
        returnsVal[controlSP]   = false;
        infoStack[controlSP]    = NULL;
        hNodes[controlSP]       = 0;
        eNodes[controlSP]       = 0;
        sNodes[controlSP]       = 0;
    }
}

static void PopControl() {
    if (controlSP == 0)
        FatalError_ov1(ERR167); /* COMPILER Error: CONTROL STACK UNDERFLOW */
    else if (controlStk[controlSP--])
        returnsVal[controlSP] |= returnsVal[controlSP + 1];
}

static word NewLocalLabel() {
    return ++localLabelCnt;
}

static word WrTx2VarId(info_t *pInfo) {
    word varT2 = WrTx2Item1Arg(T2_IDENTIFIER, ToIdx(pInfo));
    if ((pInfo->flag & F_MEMBER)) {
        SetInfo(pInfo->parent);
        word memberNode = WrTx2Item1Arg(T2_IDENTIFIER, ToIdx(info));
        varT2           = WrTx2Item2Arg(T2_MEMBER, CvtToRel(memberNode), CvtToRel(varT2));
    } else
        infoIdx = ToIdx(info = pInfo);
    return varT2;
}

static word WrTx2Var(info_t *pInfo) {
    word varNode = WrTx2VarId(pInfo);
    if ((info->flag & F_BASED)) {
        word basedNode = WrTx2VarId(FromIdx(info->baseInfo));
        varNode        = WrTx2Item2Arg(T2_BASED, CvtToRel(basedNode), CvtToRel(varNode));
    }
    return varNode;
}

static void ChkEndOfStmt() {
    if (MatchTx2AuxFlag(128)) {
        WrTx2ExtError(ERR32);           /* INVALID SYNTAX, TEXT IGNORED UNTIL ';' */
        while ((tx1Aux2 & 0x80) != 0) { // skip Expression items
            GetTx1Item();
        }
        SetRegetTx1Item();
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
    word headLoop = hNodes[controlSP];
    word endLoop  = eNodes[controlSP];
    if ((stepLoop = sNodes[controlSP]) == 0) {
        if (info->type == BYTE_T)
            WrTx2Item1Arg(T2_LOCALLABEL, NewLocalLabel());
        word loopVar  = WrTx2Var(info);
        word loopVar1 = WrTx2Var(info);
        word loopStep = WrTx2Item1Arg(T2_NUMBER, 1);
        if (info->type == BYTE_T) {
            loopVar1 = WrTx2Item2Arg(T2_PLUSSIGN, CvtToRel(loopVar1), CvtToRel(loopStep));
            WrTx2Item2Arg(T2_COLONEQUALS, CvtToRel(loopVar), CvtToRel(loopVar1));
            WrTx2Item2Arg(T2_JNZ, headLoop, 3);
        } else {
            loopVar1 = WrTx2Item2Arg(T2_ADDW, CvtToRel(loopVar1), CvtToRel(loopStep));
            WrTx2Item2Arg(T2_COLONEQUALS, CvtToRel(loopVar), CvtToRel(loopVar1));
            WrTx2Item1Arg(T2_JNC, headLoop);
        }
    } else
        WrTx2Item1Arg(T2_JMP, stepLoop);
    WrTx2Item1Arg(T2_LOCALLABEL, endLoop);
}

static word SerialiseExpression() {
    Expression();
    return SerialiseParse(markedStSP);
}

static void SerialiseIF() {
    word exprNode = SerialiseExpression();
    ChkEndOfStmt();
    if (MatchTx1Item(L_JMPFALSE))
        WrTx2Item2Arg(T2_JMPFALSE, tx1Item.dataw[0], CvtToRel(exprNode));
    else
        FatalError_ov1(ERR168); /* COMPILER ERROR: BRANCH MISSING IN 'IF' STATEMENT */
}

static void ParseStmtcnt() {
    stmtT2Cnt = 0;
    MapLToT2();
    curStmtNum = tx1Item.dataw[0];
}

static void ParseIf() {
    SerialiseIF();
}

static void ParseProcedure() {
    MapLToT2();
    PushControl(DO_PROC);
    infoStack[controlSP] = curProc;
    curProc = info = FromIdx(infoIdx = tx1Item.dataw[0]);
    info->flag |= F_DECLARED;
}

/*
    Serialise WHILE
    do while (cond);

    a: T2_LOCALLABEL headLabel
    b: Serialise expression cond
    c: T2_JMPFALSE endlabel, rel(b)

*/
static void ParseWhile() {
    PushControl(DO_WHILE);
    WrTx2Item1Arg(T2_LOCALLABEL, hNodes[controlSP] = NewLocalLabel());
    word endLabel = eNodes[controlSP] = NewLocalLabel();
    WrTx2Item2Arg(T2_JMPFALSE, endLabel, CvtToRel(SerialiseExpression()));
    ChkEndOfStmt();
}

/*
    Serialise Case block
    a: serialise expression
       T2_CASEBLOCK headLabel rel(a)
*/
static void ParseCASE() {
    PushControl(DO_CASE);
    MapLToT2();
    word exprT2 = SerialiseExpression();
    WrTx2Item2Arg(T2_CASEBLOCK, hNodes[controlSP] = NewLocalLabel(), CvtToRel(exprT2));
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

    PushControl(DO_LOOP);
    if (NotMatchTx1Item(L_IDENTIFIER)) {
        WrTx2Error(ERR138); /* MISSING INDEX VARIABLE */
        return;
    }
    GetVariable();

    info_t *pInfo = infoStack[controlSP] = info;
    if ((info->type != BYTE_T && info->type != ADDRESS_T) || (info->flag & F_ARRAY)) {
        WrTx2ExtError(ERR139); /* INVALID INDEX VARIABLE TYPE, NOT BYTE OR ADDRESS */
        return;
    }
    if (NotMatchTx1Item(L_EQ)) {
        WrTx2ExtError(ERR140); /* MISSING '=' FOLLOWING INDEX VARIABLE */
        return;
    }
    controlStk[controlSP] = DO_ITERATIVE;
    word lhs              = WrTx2Var(pInfo);
    word rhs              = SerialiseExpression();
    WrTx2Item2Arg(T2_COLONEQUALS, CvtToRel(lhs), CvtToRel(rhs));
    word headLabel    = NewLocalLabel();
    hNodes[controlSP] = headLabel;
    WrTx2Item1Arg(T2_LOCALLABEL, headLabel);
    lhs = WrTx2Var(pInfo);

    if (MatchTx1Item(L_TO))
        rhs = SerialiseExpression();
    else {
        WrTx2ExtError(ERR141); /* MISSING 'to' CLAUSE */
        rhs = WrTx2Item1Arg(T2_NUMBER, 0);
    }

    lhs               = WrTx2Item2Arg(T2_LE, CvtToRel(lhs), CvtToRel(rhs));
    word endLabel     = NewLocalLabel();
    eNodes[controlSP] = endLabel;
    WrTx2Item2Arg(T2_JMPFALSE, endLabel, CvtToRel(lhs));

    if (NotMatchTx1Item(L_BY))
        return;
    word byLabel       = NewLocalLabel();
    byNodes[controlSP] = byLabel;
    WrTx2Item1Arg(T2_JMP, byLabel);
    word stepLabel    = NewLocalLabel();
    sNodes[controlSP] = stepLabel;
    WrTx2Item1Arg(T2_LOCALLABEL, stepLabel);
    lhs         = WrTx2Var(pInfo);
    rhs         = WrTx2Var(pInfo);
    word rhsExp = SerialiseExpression();
    rhs         = WrTx2Item2Arg(T2_ADDW, CvtToRel(rhs), CvtToRel(rhsExp));
    WrTx2Item2Arg(T2_COLONEQUALS, CvtToRel(lhs), CvtToRel(rhs));
    WrTx2Item1Arg(T2_JNC, headLabel);
    WrTx2Item1Arg(T2_JMP, endLabel);
    WrTx2Item1Arg(T2_LOCALLABEL, byLabel);
}

static void ParseEND() {
    switch (controlStk[controlSP]) {
    case DO_PROC:
        infoIdx = ToIdx(info = curProc);
        info->flag |= F_DEFINED;
        if (info->returnType && !returnsVal[controlSP])
            WrTx2Error(ERR156); /* MISSING RETURN STATEMENT IN TYPED PROCEDURE */
        WrTx2Item(T2_ENDPROC);
        curProc = infoStack[controlSP];
        break;
    case DO_LOOP:
        break;
    case DO_WHILE:
        WrTx2Item1Arg(T2_JMP, hNodes[controlSP]);
        WrTx2Item1Arg(T2_LOCALLABEL, eNodes[controlSP]);
        break;
    case DO_CASE:
        WrTx2Item1Arg(T2_LOCALLABEL, hNodes[controlSP]);
        WrTx2Item(T2_ENDCASE);
        break;
    case DO_ITERATIVE:
        EndIterDo();
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

static void ParseCALL() {
    if (Sub_59D4())
        SerialiseParse(markedStSP);
    ChkEndOfStmt();
}

static void ReturnStatement() // return already seen
{
    if (!curProc) {
        WrTx2Error(ERR155); /* INVALID RETURN IN MAIN PROGRAM */
        return;
    }
    infoIdx      = ToIdx(info = curProc);
    byte retType = info->returnType;
    if (MatchTx2AuxFlag(0x80)) { /* there is an Expression item */
        SetRegetTx1Item();
        if (retType == 0)       // untyped procedure
            WrTx2Error(ERR136); /* INVALID RETURN FOR UNTYPED PROCEDURE, VALUE ILLEGAL */
        WrTx2Item1Arg(retType == BYTE_T ? T2_RETURNBYTE : T2_RETURNWORD,
                      CvtToRel(SerialiseExpression()));
    } else {
        if (retType != 0)       // typed procedure
            WrTx2Error(ERR137); /* MISSING VALUE IN RETURN FOR TYPED PROCEDURE */
        WrTx2Item(T2_RETURN);
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
    if (NotMatchTx1Item(L_IDENTIFIER)) // no goto identifier
        WrTx2Error(ERR142);            /* MISSING IDENTIFIER FOLLOWING GOTO */
    else {
        ChkIdentifier(); // can only goto label
        if (info->type != LABEL_T)
            WrTx2ExtError(ERR143); /* INVALID REFERENCE FOLLOWING GOTO, NOT A LABEL */
        else {
            if (High(info->scope) == 1 &&
                High(scopeChains[scopeSP]) != 1) // goto is from local to a module level target
                info->flag |= F_MODGOTO;         // flag target as module level label
            if (High(info->scope) == 1 ||
                High(info->scope) == High(scopeChains[scopeSP])) { // its module level to module
                                                                   // level or local to local
                WrTx2Item1Arg(T2_GOTO, infoIdx); // write lex token with normalised infoOffset
                ChkEndOfStmt();
            } else
                WrTx2ExtError(ERR144); /* INVALID GOTO LABEL, NOT AT LOCAL OR MODULE LEVEL */
        }
    }
}

// <goto statement>
static void GoStatement() {
    if (NotMatchTx1Item(L_TO))
        WrTx2Error(ERR145); /* MISSING 'TO' FOLLOWING 'GO' */
    GotoStatement();        // handle as normal GOTO
}

// <locator> (AT already parsed)
static void Locator() {
    index_t varInfoIdx = tx1Item.dataw[0]; // info for variable

    GetTx1Item();
    RestrictedExpression(); // get the AT Expression
    WrAtByte(ATI_AHDR);
    WrAtWord(varInfoIdx);           // info index
    WrAtWord(curStmtNum);           // statement number
    WrAtWord(var.infoOffset);       // info offset for identifier in Expression
    WrAtWord(var.arrayIndex);       // any array index
    WrAtWord(var.nestedArrayIndex); // ditto for any structure element
    WrAtWord(var.val);              // the +/- delta
    ExpectRParen(ERR146);           /* MISSING ') ' AFTER 'AT' RESTRICTED Expression */
}

void Initialization() {
    index_t varInfoIdx = tx1Item.dataw[0];   // info of variable being initialised
    GetTx1Item();                            // get next item
    word cnt = InitialValueList(varInfoIdx); // parse the initialisation list
    SetInfo(varInfoIdx);
    if (infoIdx && (info->flag & F_STARDIM)) // update * dim
        info->dim = cnt;
}

void ParseLexItems() {
    controlSP = 0;
    GetTx1Item();
    while (tx1Item.type != L_EOF) {
        infoIdx = 0;
        info    = NULL;
        switch (tx1Item.type) {
        case L_STMTCNT:
            ParseStmtcnt();
            break;
        case L_SCOPE:
            PushScope(tx1Item.dataw[0]);
            break;
        case L_END:
            ParseEND();
            break;
        case L_IF:
            ParseIf();
            break;
        case L_DOLOOP:
            IterativeDoStatement();
            ChkEndOfStmt();
            break;
        case L_WHILE:
            ParseWhile();
            break;
        case L_DO:
            PushControl(DO_LOOP);
            break;
        case L_PROCEDURE:
            ParseProcedure();
            break;
        case L_CASE:
            ParseCASE();
            break;
        case L_STATEMENT:
            ParseStatement();
            break;
        case L_CALL:
            ParseCALL();
            break;
        case L_RETURN:
            ReturnStatement();
            break;
        case L_GOTO:
            GotoStatement();
            break;
        case L_GO:
            GoStatement();
            break;
        case L_SEMICOLON:
            break;
        case L_ENABLE:
        case L_DISABLE:
        case L_HALT:
            I8080DependentStatement();
            break;
        case L_AT:
            Locator();
            break;
        case L_DATA:
        case L_INITIAL:
            Initialization();
            break;
        default:
            break; /* simple items */
        case L_LABELDEF:
        case L_LOCALLABEL:
        case L_JMP:
        case L_JMPFALSE:
        case L_CASELABEL:
            MapLToT2();
            break;
        case L_EXTERNAL:
            SetInfo(tx1Item.dataw[0]);
            info->flag |= F_DECLARED | F_DEFINED; // flag proc as declared & defined
            break;
        }
        GetTx1Item();
    }
}
