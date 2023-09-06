/****************************************************************************
 *  plm1d.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

// <expression>
static void ExprParse0() {
    PushOperator(0);                  // put end marker
    if (MatchTx1Item(L_IDENTIFIER)) { // startes with identifier
        PushParse(E_1);
        ChkIdentifier();
        PushParse(E_11);
    } else { // should NOT or '-'
        PushParse(E_3);
        PushParse(E_4);
    }
}

static void ExprParse1() {
    if (MatchTx1Item(L_COLONEQUALS)) {
        if (Sub_512E(exSP)) {
            WrTx2ExtError(ERR128); /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
            ExprPop();
        } else
            PushParse(E_COLONEQUALS);
        PushParse(E_3);
        PushParse(4);
    } else {
        if (eStack[exSP].icode == I_OUTPUT) {
            WrTx2Error(ERR130); /*  ILLEGAL REFERENCE to OUTPUT FUNCTION */
            ExprPop();
            ExprPush2(I_NUMBER, 0);
        }
        PushParse(E_3);
    }
}

static void ExprParse2() {
    ExprMakeNode(I_COLONEQUALS, 2);
}

static void ExprParse3() {
    if (MatchTx2AuxFlag(0x40)) {
        while (b4172[tx1Aux1] <= b4172[operatorStack[operatorSP]]) {
            if (b4172[tx1Aux1] == 50 && b4172[operatorStack[operatorSP]] == 50)
                WrTx2ExtError(ERR218); /* ILLEGAL SUCCESSIVE USES OF RELATIONAL OPERATORS */
            AcceptOpAndArgs();
        }
        PushOperator(tx1Aux1);
        PushParse(E_3);
        PushParse(4);
    } else {
        while (operatorStack[operatorSP] != 0)
            AcceptOpAndArgs();
        PopOperator();
    }
}

static void ExprParse4() {
    if (MatchTx1Item(L_MINUSSIGN)) // starts with - so convert to unary minus
        PushOperator(I_UNARYMINUS);
    else if (MatchTx1Item(L_NOT)) // starts with NOT
        PushOperator(I_NOT);
    PushParse(E_PRIMARY); // <primary>
}

// <primary>
static void ExprParse5() {
    word p;

    if (MatchTx1Item(L_IDENTIFIER)) {
        ChkIdentifier();
        if (info->type == BUILTIN_T && info->builtinId == 9) { /* 9 -> OUTPUT */
            WrTx2ExtError(ERR130); /* ILLEGAL REFERENCE TO OUTPUT FUNCTION */
            if (MatchTx1Item(L_LPAREN))
                ResyncRParen();
            ExprPush2(I_NUMBER, 0); // error so assume 0
        } else
            PushParse(E_11);
    } else if (MatchTx1Item(L_NUMBER))
        ExprPush2(I_NUMBER, tx1Item.dataw[0]); // push I_NUMBER & value
    else if (MatchTx1Item(L_STRING)) {
        if (tx1Item.dataw[0] == 1)
            p = Low(tx1Item.dataw[1]);                             // single char
        else if (tx1Item.dataw[0] == 2)                            // double char
            p = (tx1Item.dataw[1] >> 8) + (tx1Item.dataw[1] << 8); // swap bytes;
        else {
            WrTx2ExtError(ERR100); /* INVALID STRING CONSTANT IN EXPRESSION  */
            p = 0;                 // error so use 0
        }
        ExprPush2(I_NUMBER, p); // push I_NUMBER & string constant value
    } else if (MatchTx1Item(L_LPAREN)) {
        PushParse(E_RPAREN);                 // closing ')' of subexpression
        PushParse(E_EXPRESSION);             // <expression>
    } else if (MatchTx1Item(L_PERIOD)) {     // addressof
        if (MatchTx1Item(L_IDENTIFIER)) {    // <location reference> := .<variable reference>
            PushParse(E_ADDRESSOF);          // <location reference> make addressof node
            PushParse(E_VARREF);             // <variable reference>
        } else if (MatchTx1Item(L_LPAREN)) { // <location reference> := .<constant list>
            ConstantList();                  // collect the constant list
            PushParse(E_ADDRESSOF);          // <location reference> make addressof node
        } else {
            WrTx2ExtError(ERR101); /* INVALID ITEM FOLLOWS DOT OPERATOR */
            PushParse(E_PRIMARY);  // <primary>
        }
    } else {
        WrTx2ExtError(ERR102);  /* MISSING PRIMARY OPERAND */
        ExprPush2(I_NUMBER, 0); // assume a 0
    }
}

// closing ')' of <subexpression>
static void ExprParse6() {
    ExpectRParen(ERR103); /* MISSING ') ' AT END OF SUBEXPRESSION */
}

// <location reference> make address of node
static void ExprParse7() {
    ExprMakeNode(I_ADDRESSOF, 1);
}

// <variable reference>
static void ExprParse8() {
    ChkIdentifier();
    if (info->type == BUILTIN_T) { // can't be a bulitin
        PopParse();
        WrTx2ExtError(ERR123); /* INVALID DOT OPERAND, BUILT-IN PROCEDURE ILLEGAL */
        if (MatchTx1Item(L_LPAREN))
            ResyncRParen();
        ExprPush2(I_NUMBER, 0); // assume a 0
    } else {
        ExprPush2(I_IDENTIFIER, infoIdx); // record the variable
        if (info->type == PROC_T) {       // if proc then can't have (...) invocation
            if (MatchTx1Item(L_LPAREN)) {
                WrTx2ExtError(ERR104); /* ILLEGAL PROCEDURE INVOCATION WITH DOT OPERATOR */
                ResyncRParen();        // skip invocation
            }
        } else { // labels are also bad
            if (info->type == LABEL_T)
                WrTx2ExtError(ERR158); /* INVALID DOT OPERAND, LABEL ILLEGAL */
            PushParse(E_DATAREF);      // <data reference>
        }
    }
}

// <data reference>
static void ExprParse9() {
    PushParse(infoIdx);           // save name info
    PushParse(E_MEMBERSPEC);      // <member specifier>
    if (MatchTx1Item(L_LPAREN)) { // start of <subscript>
        if (!(info->flag & F_ARRAY))
            WrTx2ExtError(ERR127); /* INVALID SUBSCRIPT ON NON-ARRAY */
        PushParse(E_SUBSCRIPT);    // <subscript>
    }
}

// <member specifier>
static void ExprParse10() {
    offset_t nameInfo = PopParse();

    if (MatchTx1Item(L_PERIOD)) { // start of <member specifier>
        SetInfo(nameInfo);
        if (info->type != STRUCT_T)
            WrTx2ExtError(ERR110); /* INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE */
        else if (NotMatchTx1Item(L_IDENTIFIER))
            WrTx2ExtError(ERR111); /* INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER */
        else {
            ChkStructureMember();
            ExprPush2(I_IDENTIFIER, infoIdx);
            PushParse(nameInfo); // member info
            PushParse(E_MEMBER); // make member node
            if (MatchTx1Item(L_LPAREN))
                PushParse(E_SUBSCRIPT); // <subscript>
        }
    } else
        FixupBased(nameInfo);
}

static void ExprParse11() {
    word p;
    if (info->type == BUILTIN_T) {
        p = builtinsMap[info->builtinId];
        if (p == I_INPUT || p == I_OUTPUT)
            ParsePortNum((byte)p);
        else if (p == I_LENGTH || p == I_LAST || p == I_SIZE)
            Sub_4DCF((byte)p);
        else {
            PushParse(p);
            PushParse(info->paramCnt);
            PushOperator(0);
            PushParse(E_17);
            ChkTypedProcedure();
            if (MatchTx1Item(L_LPAREN)) {
                PushParse(E_ARGLIST);
                PushParse(E_EXPRESSION); // <expression>
            }
        }
    } else if (info->type == PROC_T) {
        Sub_50D5();
        ExprPush2(I_IDENTIFIER, infoIdx);
        ChkTypedProcedure();
        PushParse(info->paramCnt);
        PushOperator(0);
        PushParse(E_FUNCTION);
        if (MatchTx1Item(L_LPAREN)) {
            PushParse(E_ARGLIST);
            PushParse(E_EXPRESSION); // <expression>
        }
    } else
        PushParse(E_12);
}

static void ExprParse12() {
    ExprPush2(I_IDENTIFIER, infoIdx);
    PushParse(infoIdx);
    if (info->type == LABEL_T)
        WrTx2ExtError(ERR132); /* ILLEGAL USE OF label */
    PushParse(E_PERIOD);
    if ((info->flag & F_ARRAY)) {
        if (MatchTx1Item(L_LPAREN))
            PushParse(E_SUBSCRIPT);
        else
            WrTx2ExtError(ERR133); /* ILLEGAL REFERENCE to UNSUBSCRIPTED ARRAY */
    } else if (MatchTx1Item(L_LPAREN)) {
        WrTx2ExtError(ERR127); /* INVALID SUBSCRIPT ON NON-ARRAY */
        PushParse(E_SUBSCRIPT);
    }
}

static void ExprParse13() {
    index_t memberInfo = PopParse();

    SetInfo(memberInfo);

    if (MatchTx1Item(L_PERIOD)) {
        if (info->type != STRUCT_T)
            WrTx2ExtError(ERR110); /* INVALID LEFT OPERAND OF QUALIFICATION, not A structure */
        else if (NotMatchTx1Item(L_IDENTIFIER))
            WrTx2ExtError(ERR111); /* INVALID RIGHT OPERAND OF QUALIFICATION, not IDENTIFIER */
        else {
            ChkStructureMember();
            ExprPush2(I_IDENTIFIER, infoIdx);
            PushParse(memberInfo);
            PushParse(E_MEMBER);
            if ((info->flag & F_ARRAY)) {
                if (MatchTx1Item(L_LPAREN))
                    PushParse(E_SUBSCRIPT);
                else
                    WrTx2ExtError(ERR134); /* ILLEGAL REFERENCE to UNSUBSCRIPTED MEMBER ARRAY */
            } else if (MatchTx1Item(L_LPAREN))
                WrTx2ExtError(ERR127); /* INVALID SUBSCRIPT ON NON-ARRAY */
        }
    } else {
        if (info->type == STRUCT_T)
            WrTx2ExtError(ERR135); /* ILLEGAL REFERENCE to AN UNQUALIFIED structure */
        FixupBased(memberInfo);
    }
}

// make member node
static void ExprParse14() {
    ExprMakeNode(I_MEMBER, 2); // make member node
    FixupBased(PopParse());    // member info
}

static void ExprParse15() {
    operatorStack[operatorSP]++;
    if (MatchTx1Item(L_COMMA)) {
        PushParse(E_ARGLIST);
        PushParse(E_EXPRESSION); // <expression>
    } else
        ExpectRParen(ERR113); /* MISSING ') ' at end OF ARGUMENT LIST */
}

static void ExprParse16() {
    ExprMakeNode(I_CALL, GetCallArgCnt() + 1);
}

static void ExprParse17() {
    byte argCnt = GetCallArgCnt(); // force ordering of functions
    ExprMakeNode((byte)PopParse(), argCnt);
}

static void ExprParse18() {
    ExprMakeNode(I_CALLVAR, PopOperator() + 1);
}

// <subscript>
static void ExprParse19() {
    PushParse(E_ENDSUBSCRIPT); // end of <subscript>
    PushParse(E_EXPRESSION);   // <expression>
}

// end of <subscript>
static void ExprParse20() {
    if (MatchTx1Item(L_COMMA)) {
        WrTx2ExtError(ERR114); /* INVALID SUBSCRIPT, MULTIPLE SUBSCRIPTS ILLEGAL */
        RecoverRPOrEndExpr();
    }
    ExpectRParen(ERR115); /* MISSING ') ' at end OF SUBSCRIPT */
    MkIndexNode();
}

void ExpressionStateMachine() {
    while (parseSP) {
        switch (parseStack[parseSP--]) {
        case 0:
            ExprParse0();
            break; // <expression>
        case 1:
            ExprParse1();
            break;
        case 2:
            ExprParse2();
            break;
        case 3:
            ExprParse3();
            break;
        case 4:
            ExprParse4();
            break;
        case 5:
            ExprParse5();
            break; // <primary>
        case 6:
            ExprParse6();
            break; // closing ')' of <subexpression>
        case 7:
            ExprParse7();
            break; // <location reference> make addressof node
        case 8:
            ExprParse8();
            break; // <variable reference>
        case 9:
            ExprParse9();
            break; // <data reference>
        case 10:
            ExprParse10();
            break; // <member specifier>
        case 11:
            ExprParse11();
            break;
        case 12:
            ExprParse12();
            break;
        case 13:
            ExprParse13();
            break;
        case 14:
            ExprParse14();
            break; // make member node
        case 15:
            ExprParse15();
            break;
        case 16:
            ExprParse16();
            break;
        case 17:
            ExprParse17();
            break;
        case 18:
            ExprParse18();
            break;
        case 19:
            ExprParse19();
            break; // <subscript>
        case 20:
            ExprParse20();
            break; // end of <subscript>
        }
    }
}
