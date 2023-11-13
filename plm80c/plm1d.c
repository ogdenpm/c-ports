/****************************************************************************
 *  plm1d.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

// <expression>
static void ExprParse0() {
    PushOperator(I_MARKER);            // put end marker
    if (MatchTx1Item(T1_IDENTIFIER)) { // starts with identifier
        PushParse(E_EMBED);
        ChkIdentifier();
        PushParse(E_VARREF);
    } else {                   // should be NOT or '-'
        PushParse(E_OPERATOR); // shift / reduce operator
        PushParse(E_UNARY);    // <unaryOp>? <primary>
    }
}

// check embedded :=
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
        if (operandStack[operandSP].icode == I_OUTPUT) {
            Wr2Error(ERR130); /*  ILLEGAL REFERENCE to OUTPUT FUNCTION */
            PopOperand();
            PushSimpleOperand(I_NUMBER, 0);
        }
        PushParse(E_OPERATOR);
    }
}

static void ExprParse2() { // := ...
    PushComplexOperand(I_COLONEQUALS, 2);
}

// check to see if shift / reduce operator
static void ExprParse3() {
    if (MatchTx2AuxFlag(F_BINOP)) { // binary operator
        while (precedence[tx1ICode] <= precedence[operatorStack[operatorSP]]) {
            if (precedence[tx1ICode] == 50 && precedence[operatorStack[operatorSP]] == 50)
                Wr2TokError(ERR218); /* ILLEGAL SUCCESSIVE USES OF RELATIONAL OPERATORS */
            ApplyOperator();         // reduce previous operators
        }
        PushOperator(tx1ICode); // add this new operator
        PushParse(E_OPERATOR);
        PushParse(E_UNARY); // parse <unaryOp>? <primary>
    } else {
        while (operatorStack[operatorSP] != I_MARKER)
            ApplyOperator(); // reduce remaining operators
        PopOperator();       // pop marker
    }
}

// parse <unaryOp>? <primary>
static void ExprParse4() {
    if (MatchTx1Item(T1_MINUSSIGN)) // starts with - so convert to unary minus
        PushOperator(I_UNARYMINUS);
    else if (MatchTx1Item(T1_NOT)) // starts with NOT
        PushOperator(I_NOT);
    PushParse(E_PRIMARY); // <primary>
}

// <primary>
static void ExprParse5() {
    if (MatchTx1Item(T1_IDENTIFIER)) {
        ChkIdentifier();
        if (info->type == BUILTIN_T && info->builtinId == 9) { /* 9 -> OUTPUT */
            Wr2TokError(ERR130);         /* ILLEGAL REFERENCE TO OUTPUT FUNCTION */
            if (MatchTx1Item(T1_LPAREN)) // skip any (...)
                ResyncRParen();
            PushSimpleOperand(I_NUMBER, 0); // error so assume 0
        } else
            PushParse(E_VARREF);
    } else if (MatchTx1Item(T1_NUMBER))
        PushSimpleOperand(I_NUMBER, tx1Item.dataw[0]); // push I_NUMBER & value
    else if (MatchTx1Item(T1_STRING)) {
        word val;
        if (tx1Item.dataw[0] == 1)
            val = Low(tx1Item.dataw[1]);                             // single char
        else if (tx1Item.dataw[0] == 2)                              // double char
            val = (tx1Item.dataw[1] >> 8) + (tx1Item.dataw[1] << 8); // swap bytes;
        else {
            Wr2TokError(ERR100); /* INVALID STRING CONSTANT IN EXPRESSION  */
            val = 0;             // error so use 0
        }
        PushSimpleOperand(I_NUMBER, val); // push I_NUMBER & string constant value
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
        PushSimpleOperand(I_NUMBER, 0); // assume a 0
    }
}

// closing ')' of <subexpression>
static void ExprParse6() {
    ExpectRParen(ERR103); /* MISSING ') ' AT END OF SUBEXPRESSION */
}

// <location reference> make address of node
static void ExprParse7() {
    PushComplexOperand(I_ADDRESSOF, 1);
}

// <location reference>
static void ExprParse8() {
    ChkIdentifier();
    if (info->type == BUILTIN_T) { // can't be a bulitin
        PopParse();
        Wr2TokError(ERR123); /* INVALID DOT OPERAND, BUILT-IN PROCEDURE ILLEGAL */
        if (MatchTx1Item(T1_LPAREN))
            ResyncRParen();
        PushSimpleOperand(I_NUMBER, 0); // assume a 0
    } else {
        PushSimpleOperand(I_IDENTIFIER, ToIdx(info)); // record the variable
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

// <. variable reference>
static void ExprParse9() {
    PushParse(ToIdx(info));        // save name info
    PushParse(E_MEMBERSPEC);       // <member specifier>
    if (MatchTx1Item(T1_LPAREN)) { // start of <subscript>
        if (!(info->flag & F_ARRAY))
            Wr2TokError(ERR127); /* INVALID SUBSCRIPT ON NON-ARRAY */
        PushParse(E_SUBSCRIPT);  // <subscript>
    }
}

// <member specifier>
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
            PushSimpleOperand(I_IDENTIFIER, ToIdx(info));
            PushParse(ToIdx(nameInfo)); // member info
            PushParse(E_MEMBER);        // make member node
            if (MatchTx1Item(T1_LPAREN))
                PushParse(E_SUBSCRIPT); // <subscript>
        }
    } else
        FixupBased(nameInfo);
}
// parse <variable reference> -> <builtin> | <proc> | <variable>
static void ExprParse11() {
    if (info->type == BUILTIN_T) {
        word icode = builtinsMap[info->builtinId];
        if (icode == I_INPUT || icode == I_OUTPUT)
            ParsePortNum((byte)icode);
        else if (icode == I_LENGTH || icode == I_LAST || icode == I_SIZE)
            ParseSizing((byte)icode);
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
        PushSimpleOperand(I_IDENTIFIER, ToIdx(info));
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

// parse <data reference>
static void ExprParse12() {
    PushSimpleOperand(I_IDENTIFIER, ToIdx(info));
    PushParse(ToIdx(info)); // for member access and based fixup
    if (info->type == LABEL_T)
        Wr2TokError(ERR132);      /* ILLEGAL USE OF label */
    PushParse(E_DOT);          // member check
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

// parse member reference
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
            PushSimpleOperand(I_IDENTIFIER, ToIdx(info)); // member
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

// make member node
static void ExprParse14() {
    PushComplexOperand(I_MEMBER, 2); // make member node
    FixupBased(FromIdx(PopParse())); // parent (might be based)
}

// parse arglist
static void ExprParse15() {
    operatorStack[operatorSP]++; // actual argCnt
    if (MatchTx1Item(T1_COMMA)) {
        PushParse(E_ARGLIST);    //  get another arg
        PushParse(E_EXPRESSION); // <expression>
    } else
        ExpectRParen(ERR113); /* MISSING ') ' at end OF ARGUMENT LIST */
}

// reduce <function>
static void ExprParse16() {
    PushComplexOperand(I_CALL, GetCallArgCnt() + 1);
}

// reduce <builtin>
// on entry parseStack[top] -> number of expected args
//          parseStack[top - 1] = builtin icode
//          operatorStack[top] -> actual number of args
static void ExprParse17() {
    byte argCnt = GetCallArgCnt(); // separate out to avoid sequence point issues. Uses top args
    PushComplexOperand((byte)PopParse(), argCnt);
}

// reduce <reduce call var>
// not actually used as var(args) looks like an array index and will be processes as such
static void ExprParse18() {
    PushComplexOperand(I_CALLVAR, PopOperator() + 1);
}

// <subscript expression>
static void ExprParse19() {
    PushParse(E_ENDSUBSCRIPT); // end of <subscript>
    PushParse(E_EXPRESSION);   // <expression>
}

// <subscript end>
static void ExprParse20() {
    if (MatchTx1Item(T1_COMMA)) {
        Wr2TokError(ERR114); /* INVALID SUBSCRIPT, MULTIPLE SUBSCRIPTS ILLEGAL */
        RecoverRPOrEndExpr();
    }
    ExpectRParen(ERR115); /* MISSING ') ' at end OF SUBSCRIPT */
    MkIndexNode();
}

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
            ExprParse8();  // <location reference>
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
        case E_CALLVAR: // not used
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
