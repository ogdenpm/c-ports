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

byte Sub_5945() {
    byte i;

    ResetStacks();
    i = 0;
    while (1) {
        if (NotMatchTx1Item(L_IDENTIFIER)) {
            WrTx2ExtError(ERR128); /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
            return 0;
        }
        ChkIdentifier();
        PushParse(11);
        ExpressionStateMachine();
        if (Sub_512E(exSP)) {
            WrTx2ExtError(ERR128); /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
            ExprPop();
        } else
            i++;
        if (NotMatchTx1Item(L_COMMA))
            break;
    }

    if (MatchTx1Item(L_EQ)) {
        PushParse(0); // <expression>
        ExpressionStateMachine();
        if (i != 0) {
            ExprMakeNode(I_COLONEQUALS, i + 1);
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
static word t2Cnt;

static void FlgVisited(word node, word t2Cnt) {
    sStack[node].icode = 255;
    sStack[node].val   = t2Cnt;
}

static void Sub_5BF5(word node) {
    byte iCode = sStack[node].icode;

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
    FlgVisited(node, t2Cnt);
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
    t2Cnt = WrTx2Item(T2_BEGCALL);
}

static void SerialiseParse4() {
    SetInfo(parseStack[parseSP]);
    topNode     = parseStack[parseSP - 1];
    byte argCnt = (byte)parseStack[parseSP - 2];
    if (argCnt > 2) { /* all bar first 2 args to stack */
        if (!info)
            t2Cnt = WrTx2Item1Arg(T2_STKARG, RelCnt(sStack[topNode].sNodeIdx));
        else {
            t2Cnt = WrTx2Item1Arg(info->type == BYTE_T ? T2_STKBARG : T2_STKWARG,
                                  RelCnt(sStack[topNode].sNodeIdx));
            AdvNxtInfo();
            parseStack[parseSP] = infoIdx;
        }
        FlgVisited(topNode, t2Cnt);
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
        p = RelCnt(sStack[r + i - 1].sNodeIdx);
        q = RelCnt(sStack[r + i].sNodeIdx);
    } else if (i > 0)
        p = RelCnt(sStack[r + i].sNodeIdx);
    t2Cnt = WrTx2Item3Arg(T2_CALL, p, q, sStack[r].sNodeIdx);
    FlgVisited(topNode, t2Cnt);
}

static void SerialiseParse6() {
    byte i;
    word p;
    t2Cnt   = WrTx2Item(T2_BEGCALL);
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
    byte i;
    word p, q, r;

    topNode = PopParse();
    i       = sStack[topNode].op2 - 1;
    p       = sStack[topNode].sNodeIdx;
    q = r = 0;
    if (i > 1) {
        q = RelCnt(sStack[p + i - 1].sNodeIdx);
        r = RelCnt(sStack[p + i].sNodeIdx);
    } else if (i > 0)
        q = RelCnt(sStack[p + i].sNodeIdx);
    t2Cnt = WrTx2Item3Arg(T2_CALLVAR, q, r, RelCnt(sStack[p].sNodeIdx));
    FlgVisited(topNode, t2Cnt);
}

static void SerialiseParse8() {
    topNode    = PopParse();
    byte icode = sStack[topNode].icode;
    word sIdx  = sStack[topNode].sNodeIdx;
    t2Cnt      = WrTx2Item3Arg(icodeToTx2Map[icode], RelCnt(sStack[sIdx].sNodeIdx),
                               RelCnt(sStack[sIdx + 1].sNodeIdx), sStack[sIdx + 2].sNodeIdx);
    FlgVisited(topNode, t2Cnt);
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
        t2Cnt = WrTx2Item1Arg(T2_NUMBER, sStack[lhsIdx].val);
        t2Cnt = WrTx2Item2Arg(T2_OUTPUT, RelCnt(t2Cnt), RelCnt(sStack[rhsIdx].sNodeIdx));
    } else
        t2Cnt = WrTx2Item2Arg(T2_COLONEQUALS, RelCnt(sStack[lhsIdx].sNodeIdx),
                              RelCnt(sStack[rhsIdx].sNodeIdx));

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
    t2Cnt   = sStack[sStack[topNode].sNodeIdx + sStack[topNode].op2 - 1].sNodeIdx;
    FlgVisited(topNode, t2Cnt);
}

static void SerialiseParse13() { /* binary or unary op */

    topNode    = PopParse();
    word sIdx  = sStack[topNode].sNodeIdx;
    byte iCode = icodeToTx2Map[sStack[topNode].icode];
    if (sStack[topNode].op2 == 1)
        t2Cnt = WrTx2Item1Arg(iCode, RelCnt(sStack[sIdx].sNodeIdx));
    else
        t2Cnt =
            WrTx2Item2Arg(iCode, RelCnt(sStack[sIdx].sNodeIdx), RelCnt(sStack[sIdx + 1].sNodeIdx));
    FlgVisited(topNode, t2Cnt);
}

static void SerialiseParse14() {
    word p;
    p = sStack[topNode = parseStack[parseSP]].sNodeIdx;
    /* emit the count leaf */
    t2Cnt = WrTx2Item1Arg(T2_BEGMOVE, RelCnt(sStack[p].sNodeIdx));
    FlgVisited(p, t2Cnt);
    PushParse(15); /* Move() post serialise */
    PushParse(2);  /* serialise the address leaves */
    PushParse(p + 1);
    PushParse(1);
}

static void SerialiseParse15() { /* rest of Move() */

    topNode   = PopParse();
    word sIdx = sStack[topNode].sNodeIdx;
    t2Cnt     = WrTx2Item3Arg(T2_MOVE, RelCnt(sStack[sIdx + 1].sNodeIdx),
                              RelCnt(sStack[sIdx + 2].sNodeIdx), RelCnt(sStack[sIdx].sNodeIdx));
    FlgVisited(topNode, t2Cnt);
}

word SerialiseParse(word arg1w) {
    byte i;
    word p;

    p       = infoIdx;
    parseSP = 0;
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
    SetInfo(p);
    return sStack[arg1w].sNodeIdx;
}

static byte flowType[20];
static bool b9A13[20];
static info_t *procInfoStack[20];
static word hNodes[20];
static word eNodes[20];
static word sNodes[20];
static word byNodes[20];
static word controlSP;

static void PushScope(word arg1w) {
    if (blockDepth == 0x22)
        FatalError_ov1(ERR164); /* COMPILER Error: SCOPE STACK OVERFLOW */
    else
        procChains[++blockDepth] = arg1w;
}

static void PopScope() {
    if (blockDepth == 0)
        FatalError_ov1(ERR165); /* COMPILER Error: SCOPE STACK UNDERFLOW */
    else
        blockDepth--;
}

static void PushControl(byte arg1b) {
    if (controlSP == 0x13)
        FatalError_ov1(ERR84); /*  LIMIT EXCEEDED: BLOCK NESTING */
    else {
        flowType[++controlSP]    = arg1b;
        b9A13[controlSP]         = false;
        procInfoStack[controlSP] = NULL;
        hNodes[controlSP]        = 0;
        eNodes[controlSP]        = 0;
        sNodes[controlSP]        = 0;
    }
}

static void PopControl() {
    if (controlSP == 0)
        FatalError_ov1(ERR167); /* COMPILER Error: CONTROL STACK UNDERFLOW */
    else {
        if (flowType[controlSP])
            b9A13[controlSP - 1] |= b9A13[controlSP];
        controlSP--;
    }
}

static word NewLocalLabel() {
    return ++localLabelCnt;
}

static word Sub_671D(info_t *pInfo) {

    infoIdx = ToIdx(info = pInfo);
    word q  = WrTx2Item1Arg(T2_IDENTIFIER, ToIdx(info));
    if ((info->flag & F_MEMBER)) {
        SetInfo(info->parent);
        word p = WrTx2Item1Arg(T2_IDENTIFIER, ToIdx(info));
        q      = WrTx2Item2Arg(T2_MEMBER, RelCnt(p), RelCnt(q));
    }
    return q;
}

static word Sub_677C(info_t *pInfo) {
    word p, q;
    q = Sub_671D(pInfo);
    if ((info->flag & F_BASED)) {
        p = Sub_671D(FromIdx(info->baseInfo));
        q = WrTx2Item2Arg(T2_BASED, RelCnt(p), RelCnt(q));
    }
    return q;
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

static void EndFor() {
    word loopCnt, w;

    info         = procInfoStack[controlSP];
    word forLoop = hNodes[controlSP];
    word forEnd  = eNodes[controlSP];
    if ((w = sNodes[controlSP]) == 0) {
        if (info->type == BYTE_T)
            WrTx2Item1Arg(T2_LOCALLABEL, NewLocalLabel());
        loopCnt       = Sub_677C(info);
        word loopVar  = Sub_677C(info);
        word loopStep = WrTx2Item1Arg(T2_NUMBER, 1);
        if (info->type == BYTE_T) {
            loopVar = WrTx2Item2Arg(T2_PLUSSIGN, RelCnt(loopVar), RelCnt(loopStep));
            loopCnt = WrTx2Item2Arg(T2_COLONEQUALS, RelCnt(loopCnt), RelCnt(loopVar));
            loopCnt = WrTx2Item2Arg(T2_JNZ, forLoop, 3);
        } else {
            loopVar = WrTx2Item2Arg(T2_ADDW, RelCnt(loopVar), RelCnt(loopStep));
            loopCnt = WrTx2Item2Arg(T2_COLONEQUALS, RelCnt(loopCnt), RelCnt(loopVar));
            loopCnt = WrTx2Item1Arg(T2_JNC, forLoop);
        }
    } else
        loopCnt = WrTx2Item1Arg(T2_JMP, w);
    loopCnt = WrTx2Item1Arg(T2_LOCALLABEL, forEnd);
}

static word SerialiseExpression() {
    Expression();
    return SerialiseParse(markedStSP);
}

static void Sub_6923() {
    word p;
    p = SerialiseExpression();
    ChkEndOfStmt();
    if (MatchTx1Item(L_JMPFALSE))
        p = WrTx2Item2Arg(T2_JMPFALSE, tx1Item.dataw[0], RelCnt(p));
    else
        FatalError_ov1(ERR168); /* COMPILER ERROR: BRANCH MISSING IN 'IF' STATEMENT */
}

static void ParseStmtcnt() {
    t2CntForStmt = 0;
    MapLToT2();
    curStmtNum = tx1Item.dataw[0];
}

static void ParseIf() {
    Sub_6923();
}

static void ParseProcedure() {
    MapLToT2();
    PushControl(DO_PROC);
    procInfoStack[controlSP] = curProc;
    curProc = info = FromIdx(infoIdx = tx1Item.dataw[0]);
    info->flag |= F_DECLARED;
}

static void ParseWhile() {
    word p, q, r;
    PushControl(DO_WHILE);
    p                 = NewLocalLabel();
    q                 = NewLocalLabel();
    hNodes[controlSP] = p;
    eNodes[controlSP] = q;
    r                 = WrTx2Item1Arg(T2_LOCALLABEL, p);
    r                 = SerialiseExpression();
    r                 = WrTx2Item2Arg(T2_JMPFALSE, q, RelCnt(r));
    ChkEndOfStmt();
}

static void ParseCASE() {
    word p, q;
    PushControl(DO_CASE);
    MapLToT2();
    q                 = SerialiseExpression();
    p                 = NewLocalLabel();
    q                 = WrTx2Item2Arg(T2_63, p, RelCnt(q));
    hNodes[controlSP] = p;
    ChkEndOfStmt();
}

#if 0
static void IterativeDoStatement() {
    PushControl(DO_LOOP);
    if (NotMatchTx1Item(L_IDENTIFIER)) {
        WrTx2Error(ERR138); /* MISSING INDEX VARIABLE */
        return;
    }
    GetVariable();

    info_t *pInfo = procInfoStack[controlSP] = info;
    if ((info->type != BYTE_T && info->type != ADDRESS_T) || (info->flag & F_ARRAY)) {
        WrTx2ExtError(ERR139); /* INVALID INDEX VARIABLE TYPE, NOT BYTE OR ADDRESS */
        return;
    }
    if (NotMatchTx1Item(L_EQ)) {
        WrTx2ExtError(ERR140); /* MISSING '=' FOLLOWING INDEX VARIABLE */
        return;
    }
    flowType[controlSP] = 4;
    word codeStart      = Sub_677C(pInfo);       // var
    word codeCurrent    = SerialiseExpression(); // init expression

    WrTx2Item2Arg(T2_COLONEQUALS, RelCnt(codeStart), RelCnt(codeCurrent));
    WrTx2Item1Arg(T2_LOCALLABEL, hNodes[controlSP] = NewLocalLabel());
    codeStart = Sub_677C(pInfo);

    if (MatchTx1Item(L_TO))
        codeCurrent = SerialiseExpression();
    else {
        WrTx2ExtError(ERR141); /* MISSING 'to' CLAUSE */
        codeCurrent = WrTx2Item1Arg(T2_NUMBER, 0);
    }

    WrTx2Item2Arg(T2_LE, RelCnt(codeStart), RelCnt(codeCurrent));
    WrTx2Item2Arg(T2_JMPFALSE, eNodes[controlSP] = NewLocalLabel(), RelCnt(codeStart));

    if (NotMatchTx1Item(L_BY))
        return;
    WrTx2Item1Arg(T2_JMP, byNodes[controlSP] = NewLocalLabel());
    WrTx2Item1Arg(T2_LOCALLABEL, sNodes[controlSP] = NewLocalLabel());
    codeStart   = Sub_677C(pInfo);
    codeCurrent = Sub_677C(pInfo);
    codeCurrent = WrTx2Item2Arg(T2_ADDW, RelCnt(codeCurrent), RelCnt(SerialiseExpression()));
    WrTx2Item2Arg(T2_COLONEQUALS, RelCnt(codeStart), RelCnt(codeCurrent));
    WrTx2Item1Arg(T2_JNC, hNodes[controlSP]);
    WrTx2Item1Arg(T2_JMP, eNodes[controlSP]);
    WrTx2Item1Arg(T2_LOCALLABEL, byNodes[controlSP]);
}
#else
static void IterativeDoStatement() {
    word p, q, r, s, t, u, v;

    PushControl(DO_LOOP);
    if (NotMatchTx1Item(L_IDENTIFIER)) {
        WrTx2Error(ERR138); /* MISSING INDEX VARIABLE */
        return;
    }
    GetVariable();

    info_t *pInfo = procInfoStack[controlSP] = info;
    if ((info->type != BYTE_T && info->type != ADDRESS_T) || (info->flag & F_ARRAY)) {
        WrTx2ExtError(ERR139); /* INVALID INDEX VARIABLE TYPE, NOT BYTE OR ADDRESS */
        return;
    }
    if (NotMatchTx1Item(L_EQ)) {
        WrTx2ExtError(ERR140); /* MISSING '=' FOLLOWING INDEX VARIABLE */
        return;
    }
    flowType[controlSP]  = 4;
    p                 = Sub_677C(pInfo);
    q                 = SerialiseExpression();
    p                 = WrTx2Item2Arg(T2_COLONEQUALS, RelCnt(p), RelCnt(q));
    s                 = NewLocalLabel();
    hNodes[controlSP] = s;
    p                 = WrTx2Item1Arg(T2_LOCALLABEL, s);
    p                 = Sub_677C(pInfo);

    if (MatchTx1Item(L_TO))
        q = SerialiseExpression();
    else {
        WrTx2ExtError(ERR141); /* MISSING 'to' CLAUSE */
        q = WrTx2Item1Arg(T2_NUMBER, 0);
    }

    p                 = WrTx2Item2Arg(T2_LE, RelCnt(p), RelCnt(q));
    t                 = NewLocalLabel();
    eNodes[controlSP] = t;
    p                 = WrTx2Item2Arg(T2_JMPFALSE, t, RelCnt(p));

    if (NotMatchTx1Item(L_BY))
        return;
    v                = NewLocalLabel();
    byNodes[controlSP] = v;
    p                = WrTx2Item1Arg(T2_JMP, v);
    u                = NewLocalLabel();
    sNodes[controlSP] = u;
    p                = WrTx2Item1Arg(T2_LOCALLABEL, u);
    p                = Sub_677C(pInfo);
    q                = Sub_677C(pInfo);
    r                = SerialiseExpression();
    q                = WrTx2Item2Arg(T2_ADDW, RelCnt(q), RelCnt(r));
    WrTx2Item2Arg(T2_COLONEQUALS, RelCnt(p), RelCnt(q));
    WrTx2Item1Arg(T2_JNC, s);
    WrTx2Item1Arg(T2_JMP, t);
    WrTx2Item1Arg(T2_LOCALLABEL, v);
}
#endif


static void ParseEND() {
    switch (flowType[controlSP]) {
    case 0:
        infoIdx = ToIdx(info = curProc);
        info->flag |= F_DEFINED;
        if (info->returnType && !b9A13[controlSP])
            WrTx2Error(ERR156); /* MISSING RETURN STATEMENT IN TYPED PROCEDURE */
        WrTx2Item(T2_ENDPROC);
        curProc = procInfoStack[controlSP];
        break;
    case 1:
        break;
    case 2:
        WrTx2Item1Arg(T2_JMP, hNodes[controlSP]);
        WrTx2Item1Arg(T2_LOCALLABEL, eNodes[controlSP]);
        break;
    case 3:
        WrTx2Item1Arg(T2_LOCALLABEL, hNodes[controlSP]);
        WrTx2Item(T2_ENDCASE);
        break;
    case 4:
        EndFor();
        break;
    }
    PopControl();
    PopScope();
}

static void ParseStatement() {
    if (Sub_5945())
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
    word p;

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
        p = SerialiseExpression();
        if (retType == BYTE_T)
            p = WrTx2Item1Arg(T2_RETURNBYTE, RelCnt(p));
        else
            p = WrTx2Item1Arg(T2_RETURNWORD, RelCnt(p));
    } else {
        if (retType != 0)       // typed procedure
            WrTx2Error(ERR137); /* MISSING VALUE IN RETURN FOR TYPED PROCEDURE */
        p = WrTx2Item(T2_RETURN);
    }
    b9A13[controlSP] = true;
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
                High(procChains[blockDepth]) != 1) // goto is from local to a module level target
                info->flag |= F_MODGOTO;           // flag target as module level label
            if (High(info->scope) == 1 ||
                High(info->scope) == High(procChains[blockDepth])) { // its module level to module
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
    offset_t p;
    p = tx1Item.dataw[0]; // info for variable

    GetTx1Item();
    RestrictedExpression(); // get the AT Expression
    WrAtByte(ATI_AHDR);
    WrAtWord(p);                    // info index
    WrAtWord(curStmtNum);           // statement number
    WrAtWord(var.infoOffset);       // info offset for identifier in Expression
    WrAtWord(var.arrayIndex);       // any array index
    WrAtWord(var.nestedArrayIndex); // ditto for any structure element
    WrAtWord(var.val);              // the +/- delta
    ExpectRParen(ERR146);           /* MISSING ') ' AFTER 'AT' RESTRICTED Expression */
}

void Initialization() {
    word p, cnt;
    p = tx1Item.dataw[0];      // info of variable being initialised
    GetTx1Item();              // get next item
    cnt = InitialValueList(p); // parse the initialisation list
    SetInfo(p);
    if (infoIdx && (info->flag & F_STARDIM)) // update * dim
        info->dim = cnt;
}

void ParseLexItems() {
    controlSP = 0;
    GetTx1Item();
    while (tx1Item.type != L_EOF) {
        infoIdx = 0;
        info    = NULL;
        switch (lexHandlerIdxTable[tx1Item.type]) {
        case 0:
            ParseStmtcnt();
            break; /* L_STMTCNT */
        case 1:
            PushScope(tx1Item.dataw[0]);
            break; /* L_SCOPE */
        case 2:
            ParseEND();
            break; /* L_END */
        case 3:
            ParseIf();
            break; /* L_IF */
        case 4:
            IterativeDoStatement(); /* L_DOLOOP */
            ChkEndOfStmt();
            break;
        case 5:
            ParseWhile();
            break; /* L_WHILE */
        case 6:
            PushControl(1);
            break; /* L_DO */
        case 7:
            ParseProcedure();
            break; /* L_PROCEDURE */
        case 8:
            ParseCASE();
            break; /* L_CASE */
        case 9:
            ParseStatement();
            break; /* L_STATEMENT */
        case 10:
            ParseCALL();
            break; /* L_CALL */
        case 11:
            ReturnStatement();
            break; /* L_RETURN */
        case 12:
            GotoStatement();
            break; /* L_GOTO <goto statement> */
        case 13:
            GoStatement();
            break; /* L_GO <goto statement> */
        case 14:
            break; /* L_SEMICOLON */
        case 15:
            I8080DependentStatement();
            break; /* L_ENABLE, L_DISABLE, L_HALT <8080 dependent statement> */
        case 16:
            Locator();
            break; /* L_AT */
        case 17:
            Initialization();
            break; /* L_DATA, L_INITIAL */
        case 18:
            break; /* simple items */
        case 19:
            MapLToT2();
            break; /* L_LABELDEF, L_LOCALLABEL, L_JMP, L_JMPFALSE, L_CASELABEL */
        case 20:
            SetInfo(tx1Item.dataw[0]);            /* L_EXTERNAL */
            info->flag |= F_DECLARED | F_DEFINED; // flag proc as declared & defined
            break;
        }
        GetTx1Item();
    }
}
