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
        PushParseByte(11);
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
        PushParseByte(0); // <expression>
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
        PushParseWord(info->paramCnt);
        PushOperator(0);
        PushParseByte(16);
        if (MatchTx1Item(L_LPAREN)) {
            PushParseByte(15);
            PushParseByte(0);
        }
    } else if (info->type == BUILTIN_T) {
        if (info->returnType) {
            WrTx2ExtError(ERR129); /* ILLEGAL 'call' WITH TYPED procedure */
            return false;
        }
        PushParseWord(builtinsMap[info->builtinId]);
        PushParseWord(info->paramCnt);
        PushOperator(0);
        PushParseByte(17);
        if (MatchTx1Item(L_LPAREN)) {
            PushParseByte(15);
            PushParseByte(0);
        }
    } else if (info->type != ADDRESS_T || (info->flag & F_ARRAY)) {
        WrTx2ExtError(ERR118); /* INVALID INDIRECT CALL, IDENTIFIER NOT AN ADDRESS SCALAR */
        return false;
    } else {
        Sub_4D2C();
        PushOperator(0);
        PushParseByte(18);
        if (MatchTx1Item(L_LPAREN)) {
            PushParseByte(15);
            PushParseByte(0);
        }
    }
    ExpressionStateMachine();
    MoveExpr2Stmt();
    markedStSP = stSP;
    return true;
}

void Expression() {
    ResetStacks();
    PushParseByte(0);
    ExpressionStateMachine();
    MoveExpr2Stmt();
    markedStSP = stSP;
}

// lifted to file scope
static word topNode;
static word w99C1;

static void FlgVisited(word arg1w, word arg2w) {
    sStack[arg1w].op1  = 255;
    sStack[arg1w].info = arg2w;
}

static void Sub_5BF5(word arg1w) {
    byte i, j;
    offset_t p;

    p = sStack[arg1w].info;
    if ((i = sStack[arg1w].op1) == I_IDENTIFIER)
        w99C1 = WrTx2Item1Arg(T2_IDENTIFIER, p);
    else if (i == I_NUMBER) {
        if (High(p) == 0)
            j = T2_NUMBER;
        else
            j = T2_BIGNUMBER;
        w99C1 = WrTx2Item1Arg(j, p);
    } else if (i == I_INPUT)
        w99C1 = WrTx2Item1Arg(T2_INPUT, p);
    else if (i == I_LENGTH || i == I_LAST || i == I_SIZE)
        w99C1 = WrTx2Item1Arg(icodeToTx2Map[i], p);
    else
        w99C1 = WrTx2Item(icodeToTx2Map[i]);
    FlgVisited(arg1w, w99C1);
}

static void SerialiseParse0() {
    byte i;

    topNode = parseStack[parseSP];
    PopParseStack();
    if ((i = sStack[topNode].op1) == I_OUTPUT)
        return;
    if (sStack[topNode].op2 == 0)
        Sub_5BF5(topNode);
    else {
        PushParseWord(topNode);
        if (i == I_CALL)
            PushParseByte(3);
        else if (i == I_CALLVAR) {
            PushParseByte(6);
            PushParseWord(sStack[topNode].info);
            PushParseByte(0);
        } else if (i == I_COLONEQUALS)
            PushParseByte(9);
        else if (i == I_MOVE) {
            PushParseByte(14);
            PushParseWord(sStack[topNode].info);
            PushParseByte(0);
        } else if (i == I_BYTEINDEX || i == I_WORDINDEX) {
            PushParseByte(8);
            PushParseWord(2);
            PushParseWord(sStack[topNode].info);
            PushParseByte(1);
        } else {
            PushParseByte(13);
            PushParseWord(sStack[topNode].op2);  /* num args */
            PushParseWord(sStack[topNode].info); /* loc of args */
            PushParseByte(1);
        }
    }
}

static void SerialiseParse1() { /* serialise 1 leaf) check */
    topNode = parseStack[parseSP];
    PushParseByte(2);       /* flag to check for more leaves */
    PushParseWord(topNode); /* serialise this leaf */
    PushParseByte(0);
}

static void SerialiseParse2() { /* check for any more leaves */
    word p;

    p = parseSP - 1;
    parseStack[p]--;
    if (parseStack[p] == 0) {
        PopParseStack();
        PopParseStack();
    } else {
        parseStack[parseSP]++;
        PushParseByte(1);
    }
}

static void SerialiseParse3() { /* parse args */
    topNode = parseStack[parseSP];
    PushParseByte(5);                               /* final call wrap up */
    if (sStack[topNode].op2 > 1) {                  /* any args */
        PushParseWord(sStack[topNode].op2 - 1);     /* num args  */
        PushParseWord(sStack[topNode].info + 1);    /* index of arg info */
        SetInfo(sStack[sStack[topNode].info].info); /* from proc info adv to arginfo */
        AdvNxtInfo();
        PushParseWord(infoIdx);
        PushParseByte(4);
        PushParseWord(sStack[topNode].info + 1); /* index of arg info */
        PushParseByte(0);
    }
    w99C1 = WrTx2Item(T2_BEGCALL);
}

static void SerialiseParse4() {
    word p, q;
    byte i;

    q = (p = parseSP - 1) - 1;
    SetInfo(parseStack[parseSP]);
    topNode = parseStack[p];
    i       = (byte)parseStack[q];
    if (i > 2) { /* all bar first 2 args to stack */
        if (infoIdx == 0)
            w99C1 = WrTx2Item1Arg(T2_STKARG, Sub_42EF(sStack[topNode].info));
        else {
            w99C1 = WrTx2Item1Arg(info->type == BYTE_T ? T2_STKBARG : T2_STKWARG,
                                  Sub_42EF(sStack[topNode].info));
            AdvNxtInfo();
            parseStack[parseSP] = infoIdx;
        }
        FlgVisited(topNode, w99C1);
    }
    i--;
    if (i == 0) { /* all done, clear working data */
        PopParseStack();
        PopParseStack();
        PopParseStack();
    } else {
        parseStack[q] = i;
        parseStack[p] = ++topNode;
        PushParseByte(4);
        PushParseWord(topNode); /* serialise the arg */
        PushParseByte(0);
    }
}

static void SerialiseParse5() {
    word p, q, r;
    byte i;

    topNode = parseStack[parseSP];
    PopParseStack();
    r = sStack[topNode].info;
    i = sStack[topNode].op2 - 1;
    p = q = 0;
    if (i > 1) {
        p = Sub_42EF(sStack[r + i - 1].info);
        q = Sub_42EF(sStack[r + i].info);
    } else if (i > 0)
        p = Sub_42EF(sStack[r + i].info);
    w99C1 = WrTx2Item3Arg(T2_CALL, p, q, sStack[r].info);
    FlgVisited(topNode, w99C1);
}

static void SerialiseParse6() {
    byte i;
    word p;
    w99C1   = WrTx2Item(T2_BEGCALL);
    topNode = parseStack[parseSP];
    PushParseByte(7);
    if ((i = sStack[topNode].op2 - 1) != 0) {
        PushParseWord(i);
        p = sStack[topNode].info + 1;
        PushParseWord(p);
        PushParseWord(0); /* no arg info */
        PushParseByte(4);
        PushParseWord(p);
        PushParseByte(0);
    }
}

static void SerialiseParse7() {
    byte i;
    word p, q, r;

    topNode = parseStack[parseSP];
    PopParseStack();
    i = sStack[topNode].op2 - 1;
    p = sStack[topNode].info;
    q = r = 0;
    if (i > 1) {
        q = Sub_42EF(sStack[p + i - 1].info);
        r = Sub_42EF(sStack[p + i].info);
    } else if (i > 0)
        q = Sub_42EF(sStack[p + i].info);
    w99C1 = WrTx2Item3Arg(T2_CALLVAR, q, r, Sub_42EF(sStack[p].info));
    FlgVisited(topNode, w99C1);
}

static void SerialiseParse8() {
    byte i;
    word p;

    topNode = parseStack[parseSP];
    PopParseStack();
    i     = sStack[topNode].op1;
    p     = sStack[topNode].info;
    w99C1 = WrTx2Item3Arg(icodeToTx2Map[i], Sub_42EF(sStack[p].info), Sub_42EF(sStack[p + 1].info),
                          sStack[p + 2].info);
    FlgVisited(topNode, w99C1);
}

static void SerialiseParse9() {
    topNode = parseStack[parseSP];
    PushParseByte(10); /* post serialise LHS */
    PushParseWord(sStack[topNode].info + sStack[topNode].op2 - 1);
    PushParseByte(0);
}

static void SerialiseParse10() {
    byte i;
    word p;

    topNode = parseStack[parseSP];
    PushParseByte(12);           /* mark LHS as used at end */
    i = sStack[topNode].op2 - 1; /* num RHS */
    p = sStack[topNode].info;    /* base RHS */
    PushParseWord(i);
    PushParseWord(p);
    PushParseWord(p + i); /* LHS */
    PushParseByte(11);    /* after serialised leaf */
    PushParseWord(p);     /* do the leaf */
    PushParseByte(0);
}

static void SerialiseParse11() { /* do one RHS assignment */
    word p, q;
    byte i;
    word r, s;

    i = (byte)parseStack[q = (p = parseSP - 1) - 1];
    s = parseStack[parseSP];
    if (sStack[r = parseStack[p]].op1 == I_OUTPUT) {
        w99C1 = WrTx2Item1Arg(T2_NUMBER, sStack[r].info);
        w99C1 = WrTx2Item2Arg(T2_OUTPUT, Sub_42EF(w99C1), Sub_42EF(sStack[s].info));
    } else
        w99C1 = WrTx2Item2Arg(T2_COLONEQUALS, Sub_42EF(sStack[r].info), Sub_42EF(sStack[s].info));
    i--;
    if (i == 0) { /* all done */
        PopParseStack();
        PopParseStack();
        PopParseStack();
    } else { /* no so do another */
        parseStack[q] = i;
        r             = r + 1;
        parseStack[p] = r;
        PushParseByte(11); /* state 11 after serialise */
        PushParseWord(r);  /* serialise leaf */
        PushParseByte(0);
    }
}

static void SerialiseParse12() { /* mark LHS as used */
    topNode = parseStack[parseSP];
    PopParseStack();
    w99C1 = sStack[sStack[topNode].info + sStack[topNode].op2 - 1].info;
    FlgVisited(topNode, w99C1);
}

static void SerialiseParse13() { /* binary or unary op */
    word p;
    byte i;

    topNode = parseStack[parseSP];
    PopParseStack();
    p = sStack[topNode].info;
    i = icodeToTx2Map[sStack[topNode].op1];
    if (sStack[topNode].op2 == 1)
        w99C1 = WrTx2Item1Arg(i, Sub_42EF(sStack[p].info));
    else
        w99C1 = WrTx2Item2Arg(i, Sub_42EF(sStack[p].info), Sub_42EF(sStack[p + 1].info));
    FlgVisited(topNode, w99C1);
}

static void SerialiseParse14() {
    word p;
    p = sStack[topNode = parseStack[parseSP]].info;
    /* emit the count leaf */
    w99C1 = WrTx2Item1Arg(T2_BEGMOVE, Sub_42EF(sStack[p].info));
    FlgVisited(p, w99C1);
    PushParseByte(15); /* Move() post serialise */
    PushParseWord(2);  /* serialise the address leaves */
    PushParseWord(p + 1);
    PushParseByte(1);
}

static void SerialiseParse15() { /* rest of Move() */
    word p;

    topNode = parseStack[parseSP];
    PopParseStack();
    p     = sStack[topNode].info;
    w99C1 = WrTx2Item3Arg(T2_MOVE, Sub_42EF(sStack[p + 1].info), Sub_42EF(sStack[p + 2].info),
                          Sub_42EF(sStack[p].info));
    FlgVisited(topNode, w99C1);
}

word SerialiseParse(word arg1w) {
    byte i;
    word p;

    p       = infoIdx;
    parseSP = 0;
    PushParseWord(arg1w);
    PushParseByte(0);
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
    return sStack[arg1w].info;
}

static byte b99FF[20];
static bool b9A13[20];
static word procInfoStack[20];
static word hNodes[20];
static word eNodes[20];
static word w9A9F[20];
static word w9AC7[20];
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
        b99FF[++controlSP]       = arg1b;
        b9A13[controlSP]         = false;
        procInfoStack[controlSP] = 0;
        hNodes[controlSP]        = 0;
        eNodes[controlSP]        = 0;
        w9A9F[controlSP]         = 0;
    }
}

static void PopControl() {
    if (controlSP == 0)
        FatalError_ov1(ERR167); /* COMPILER Error: CONTROL STACK UNDERFLOW */
    else {
        if (b99FF[controlSP] != 0)
            b9A13[controlSP - 1] |= b9A13[controlSP];
        controlSP--;
    }
}

static word NewLocalLabel() {
    return ++localLabelCnt;
}

static word Sub_671D(offset_t arg1w) {
    word p, q;

    SetInfo(arg1w);
    q = WrTx2Item1Arg(T2_IDENTIFIER, arg1w);
    if ((info->flag & F_MEMBER)) {
        SetInfo(info->parent);
        p = WrTx2Item1Arg(T2_IDENTIFIER, infoIdx);
        q = WrTx2Item2Arg(T2_MEMBER, Sub_42EF(p), Sub_42EF(q));
    }
    return q;
}

static word Sub_677C(offset_t arg1w) {
    word p, q;
    q = Sub_671D(arg1w);
    if ((info->flag & F_BASED)) {
        p = Sub_671D(info->baseOff);
        q = WrTx2Item2Arg(T2_BASED, Sub_42EF(p), Sub_42EF(q));
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

static void Sub_67E3() {
    word p, q, r, s, u, v, w;

    p = procInfoStack[controlSP];
    u = hNodes[controlSP];
    v = eNodes[controlSP];
    if ((w = w9A9F[controlSP]) == 0) {
        SetInfo(p);
        if (info->type == BYTE_T)
            WrTx2Item1Arg(T2_LOCALLABEL, NewLocalLabel());
        q = Sub_677C(p);
        r = Sub_677C(p);
        s = WrTx2Item1Arg(T2_NUMBER, 1);
        if (info->type == BYTE_T) {
            r = WrTx2Item2Arg(T2_PLUSSIGN, Sub_42EF(r), Sub_42EF(s));
            q = WrTx2Item2Arg(T2_COLONEQUALS, Sub_42EF(q), Sub_42EF(r));
            q = WrTx2Item2Arg(T2_JNZ, u, 3);
        } else {
            r = WrTx2Item2Arg(T2_ADDW, Sub_42EF(r), Sub_42EF(s));
            q = WrTx2Item2Arg(T2_COLONEQUALS, Sub_42EF(q), Sub_42EF(r));
            q = WrTx2Item1Arg(T2_JNC, u);
        }
    } else
        q = WrTx2Item1Arg(T2_JMP, w);
    q = WrTx2Item1Arg(T2_LOCALLABEL, v);
}

static word Sub_6917() {
    Expression();
    return SerialiseParse(markedStSP);
}

static void Sub_6923() {
    word p;
    p = Sub_6917();
    ChkEndOfStmt();
    if (MatchTx1Item(L_JMPFALSE))
        p = WrTx2Item2Arg(T2_JMPFALSE, tx1Item.dataw[0], Sub_42EF(p));
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
    procInfoStack[controlSP] = procIdx;
    procIdx = infoIdx = tx1Item.dataw[0];
    info              = &infotab[infoIdx];
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
    r                 = Sub_6917();
    r                 = WrTx2Item2Arg(T2_JMPFALSE, q, Sub_42EF(r));
    ChkEndOfStmt();
}

static void ParseCASE() {
    word p, q;
    PushControl(DO_CASE);
    MapLToT2();
    q                 = Sub_6917();
    p                 = NewLocalLabel();
    q                 = WrTx2Item2Arg(T2_63, p, Sub_42EF(q));
    hNodes[controlSP] = p;
    ChkEndOfStmt();
}

static void IterativeDoStatement() {
    word p, q, r, s, t, u, v, w;

    PushControl(DO_LOOP);
    if (NotMatchTx1Item(L_IDENTIFIER)) {
        WrTx2Error(ERR138); /* MISSING INDEX VARIABLE */
        return;
    }
    GetVariable();
    w                        = infoIdx;
    procInfoStack[controlSP] = w;
    if ((info->type != BYTE_T && info->type != ADDRESS_T) || (info->flag & F_ARRAY)) {
        WrTx2ExtError(ERR139); /* INVALID INDEX VARIABLE TYPE, NOT BYTE OR ADDRESS */
        return;
    }
    if (NotMatchTx1Item(L_EQ)) {
        WrTx2ExtError(ERR140); /* MISSING '=' FOLLOWING INDEX VARIABLE */
        return;
    }
    b99FF[controlSP]  = 4;
    p                 = Sub_677C(w);
    q                 = Sub_6917();
    p                 = WrTx2Item2Arg(T2_COLONEQUALS, Sub_42EF(p), Sub_42EF(q));
    s                 = NewLocalLabel();
    hNodes[controlSP] = s;
    p                 = WrTx2Item1Arg(T2_LOCALLABEL, s);
    p                 = Sub_677C(w);

    if (MatchTx1Item(L_TO))
        q = Sub_6917();
    else {
        WrTx2ExtError(ERR141); /* MISSING 'to' CLAUSE */
        q = WrTx2Item1Arg(T2_NUMBER, 0);
    }

    p                 = WrTx2Item2Arg(T2_LE, Sub_42EF(p), Sub_42EF(q));
    t                 = NewLocalLabel();
    eNodes[controlSP] = t;
    p                 = WrTx2Item2Arg(T2_JMPFALSE, t, Sub_42EF(p));

    if (NotMatchTx1Item(L_BY))
        return;
    v                = NewLocalLabel();
    w9AC7[controlSP] = v;
    p                = WrTx2Item1Arg(T2_JMP, v);
    u                = NewLocalLabel();
    w9A9F[controlSP] = u;
    p                = WrTx2Item1Arg(T2_LOCALLABEL, u);
    p                = Sub_677C(w);
    q                = Sub_677C(w);
    r                = Sub_6917();
    q                = WrTx2Item2Arg(T2_ADDW, Sub_42EF(q), Sub_42EF(r));
    WrTx2Item2Arg(T2_COLONEQUALS, Sub_42EF(p), Sub_42EF(q));
    WrTx2Item1Arg(T2_JNC, s);
    WrTx2Item1Arg(T2_JMP, t);
    WrTx2Item1Arg(T2_LOCALLABEL, v);
}

static void ParseEND() {
    switch (b99FF[controlSP]) {
    case 0:
        SetInfo(procIdx);
        info->flag |= F_DEFINED;
        if (info->returnType && !b9A13[controlSP])
            WrTx2Error(ERR156); /* MISSING RETURN STATEMENT IN TYPED PROCEDURE */
        WrTx2Item(T2_ENDPROC);
        procIdx = procInfoStack[controlSP];
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
        Sub_67E3();
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

    if (procIdx == 0) {
        WrTx2Error(ERR155); /* INVALID RETURN IN MAIN PROGRAM */
        return;
    }
    SetInfo(procIdx);
    byte retType = info->returnType;
    if (MatchTx2AuxFlag(0x80)) { /* there is an Expression item */
        SetRegetTx1Item();
        if (retType == 0)             // untyped procedure
            WrTx2Error(ERR136); /* INVALID RETURN FOR UNTYPED PROCEDURE, VALUE ILLEGAL */
        p = Sub_6917();
        if (retType == BYTE_T)
            p = WrTx2Item1Arg(T2_RETURNBYTE, Sub_42EF(p));
        else
            p = WrTx2Item1Arg(T2_RETURNWORD, Sub_42EF(p));
    } else {
        if (retType != 0)             // typed procedure
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
