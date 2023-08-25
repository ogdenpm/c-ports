/****************************************************************************
 *  plm1c.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

word rValue;
bool use8bit;

// <restricted primary>
static void RestrictedPrimary() {
    if (MatchTx1Item(L_NUMBER))
        rValue = tx1Item.dataw[0];
    else {
        if (MatchTx1Item(L_IDENTIFIER))
            ChkIdentifier();
        WrTx2ExtError(151); /* INVALID OPERAND IN RESTRICTED Expression */
        rValue = 0;
    }
    use8bit = use8bit && (High(rValue) == 0); // if > 255 then switch off any 8 bit mode
}

// <restricted secondary>
static void RestrictedSecondary() {
    if (MatchTx1Item(L_MINUSSIGN)) {
        RestrictedPrimary();
        if (use8bit)
            rValue = -Low(rValue);
        else
            rValue = -rValue;
    } else
        RestrictedPrimary();
}

//  <restricted sum>
static void RestrictedSum() {
    word p;

    RestrictedSecondary(); // get number
    p = rValue;
    while (1) {
        if (MatchTx1Item(L_PLUSSIGN)) { // + number
            RestrictedSecondary();
            if (use8bit)
                p = (p + rValue) & 0xff;
            else
                p += rValue;
        } else if (MatchTx1Item(L_MINUSSIGN)) { // - number
            RestrictedSecondary();
            if (use8bit)
                p = (p - rValue) & 0xff;
            else
                p -= rValue;
        } else
            break;
    }
    rValue = p;
}

static void GetRestrictedArrayIndex() {
    use8bit = true;
    if (!((info->flag & F_ARRAY)))
        WrTx2ExtError(149); /* INVALID SUBSCRIPTING IN RESTRICTED REFERENCE */
    RestrictedSum();
    ExpectRParen(150); /* MISSING ') ' AT END OF RESTRICTED SUBSCRIPT */
}

static void GetRestrictedVar() {
    if (NotMatchTx1Item(L_IDENTIFIER)) {
        WrTx2Error(ERR147); /* MISSING IDENTIFIER FOLLOWING DOT OPERATOR */
        RecoverRPOrEndExpr();
        return;
    }
    ChkIdentifier();
    var.infoOffset = infoIdx;      // record the var info
    if (info->type == BUILTIN_T) { // can't take the address of a builtin!!
        WrTx2Error(ERR123);        /* INVALID DOT OPERAND, BUILT-IN PROCEDURE ILLEGAL */
        RecoverRPOrEndExpr();
        return;
    }
    if (MatchTx1Item(L_LPAREN)) { // is indexed
        GetRestrictedArrayIndex();
        var.arrayIndex = rValue;
    }
    if (MatchTx1Item(L_PERIOD)) // a struct ref
    {
        SetInfo(var.infoOffset);      // recover the base  info
        if (info->type != STRUCT_T) { // not on a stuct
            WrTx2ExtError(ERR148);    /* INVALID QUALIFICATION IN RESTRICTED REFERENCE */
            RecoverRPOrEndExpr();
            return;
        }
        if (NotMatchTx1Item(L_IDENTIFIER)) { // no identifier after the .
            WrTx2Error(ERR147);              /* MISSING IDENTIFIER FOLLOWING DOT OPERATOR */
            return;
        }
        ChkStructureMember();
        var.infoOffset = infoIdx;
        if (MatchTx1Item(L_LPAREN)) { // its a member array element reference
            GetRestrictedArrayIndex();
            var.nestedArrayIndex = rValue; // value of member array index
        }
    }
}

void RestrictedExpression() {
    var.infoOffset = var.arrayIndex = var.nestedArrayIndex = var.val = 0;
    if (MatchTx1Item(L_PERIOD)) { // <restricted reference> := .<identifier> [<restricted
                                  // subscript>] [.<identifier> [<restricted subscript>]]
        GetRestrictedVar();
        if (MatchTx1Item(L_PLUSSIGN)) // [<restricted adding operator>]
            ;
        else if (MatchTx1Item(L_MINUSSIGN)) // let RestrictedSum handle -
            SetRegetTx1Item();
        else
            return;      // no so all done
        use8bit = false; // operations will be 16 bit
        RestrictedSum(); // get sum
        var.val = rValue;
    } else if (MatchTx1Item(L_RPAREN)) // <restricted reference> := <restricted sum>
        SetRegetTx1Item();
    else {
        use8bit = true;  // assume 8 bit
        RestrictedSum(); // get sum
        var.val = rValue;
    }
}

word InitialValueList(offset_t infoOffset) {
    word listLen;

    listLen = 0;
    WrAtByte(ATI_DHDR);
    WrAtWord(infoOffset);
    WrAtWord(curStmtNum);
    while (1) {
        if (MatchTx1Item(L_STRING)) { // string initialisation
            WrAtByte(ATI_STRING);
            WrAtWord(tx1Item.len);             // write length
            WrAtBuf(tx1Item.str, tx1Item.len); // and the string
            SetInfo(infoOffset);
            listLen += info->type == ADDRESS_T ? (tx1Item.len + 1) / 2 : tx1Item.len;
        } else {
            RestrictedExpression(); // get the restricted Expression
            WrAtByte(ATI_DATA);
            WrAtBuf((pointer)&var, sizeof(var_t)); // and write the info
            listLen++;
        }
        if (NotMatchTx1Item(L_COMMA)) // keep going if comma
            break;
        else if (MatchTx1Item(L_RPAREN)) { // ) with no preceeding data
            WrTx2Error(151);               /* INVALID OPERAND IN RESTRICTED Expression */
            SetRegetTx1Item();
            break;
        }
    }
    WrAtByte(ATI_END);
    ExpectRParen(152); /* MISSING ') ' AFTER CONSTANT LIST */
    SetInfo(infoOffset);

    return listLen; // items read i.e. dimension
}

void ResetStacks() {
    parseSP = exSP = operatorSP = stSP = 0;
}

void PushParseWord(word arg1w) {
    if (parseSP == 99)
        FatalError_ov1(ERR119); /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
    parseStack[++parseSP] = arg1w;
}

void PopParseStack() {
    if (parseSP == 0)
        FatalError_ov1(159); /* COMPILER ERROR: PARSE STACK UNDERFLOW */
    parseSP--;
}

void PushParseByte(byte arg1b) {
    PushParseWord(arg1b);
}

static void ExprPush3(eStack_t *expr) {
    if (exSP == EXPRSTACKSIZE - 1)
        FatalError_ov1(121); /* LIMIT EXCEEDED: Expression TOO COMPLEX */
    eStack[++exSP] = *expr;
}

void ExprPop() {
    if (exSP == 0)
        FatalError_ov1(160); /* COMPILER Error: OPERAND STACK UNDERFLOW */
    exSP--;
}

static void SwapOperandStack() {
    if (exSP < 2)
        FatalError_ov1(161); /* COMPILER ERROR: ILLEGAL OPERAND STACK EXCHANGE */
    eStack_t tmp     = eStack[exSP];
    eStack[exSP]     = eStack[exSP - 1];
    eStack[exSP - 1] = tmp;
}

void ExprPush2(byte icode, word val) {
    eStack_t expr = { icode, 0, val };
    ExprPush3(&expr);
}

static void StmtPush3(eStack_t *expr) {
    if (stSP == 299)
        FatalError_ov1(122); /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX   */
    sStack[++stSP] = *expr;
}

void MoveExpr2Stmt() {
    StmtPush3(&eStack[exSP]);
    ExprPop();
}

void PushOperator(byte arg1b) {
    if (operatorSP == 0x31)
        FatalError_ov1(120); /* LIMIT EXCEEDED: Expression TOO COMPLEX */
    operatorStack[++operatorSP] = arg1b;
}

void PopOperatorStack() {
    if (operatorSP == 0)
        FatalError_ov1(162); /* COMPILER Error: OPERATOR STACK UNDERFLOW */
    operatorSP--;
}

void ExprMakeNode(byte icode, byte argCnt) {
    // info set to where args will be on stmt stack
    eStack_t expr = { icode, argCnt, argCnt ? stSP + 1 : 0 };

    if (exSP < argCnt)
        FatalError_ov1(ERR163); /* COMPILER ERROR: GENERATION FAILURE  */
    exSP -= argCnt;
    for (int j = exSP; argCnt; argCnt--) // push the arg information to the stmt statck
        StmtPush3(&eStack[++j]);

    ExprPush3(&expr); // replace with new expr item
}

void AcceptOpAndArgs() {
    byte op = (byte)operatorStack[operatorSP];
    ExprMakeNode(op, op == I_NOT || op == I_UNARYMINUS ? 1 : 2);
    PopOperatorStack();
}

static void PushIdentifier() {

    if ((info->flag & F_MEMBER)) { // if member create I_MEMBER node
        offset_t memberIdx = infoIdx;
        SetInfo(info->parent);
        ExprPush2(I_IDENTIFIER, infoIdx);   // struct info
        ExprPush2(I_IDENTIFIER, memberIdx); // member info
        ExprMakeNode(I_MEMBER, 2);          // new node
    } else
        ExprPush2(I_IDENTIFIER, infoIdx); // simple identifier node
}

void FixupBased(offset_t arg1w) {
    SetInfo(arg1w);               // var info
    if ((info->flag & F_BASED)) { // is it based?
        SetInfo(info->baseOff);   // get the info of the base
        PushIdentifier();         // save it
        SetInfo(arg1w);
        SwapOperandStack();       // swap var and base on stack
        ExprMakeNode(I_BASED, 2); // create a based node
    }
}

void Sub_4D2C() {
    PushIdentifier();
    FixupBased(infoIdx);
}

void ChkTypedProcedure() {
    if (!info->returnType)
        WrTx2ExtError(131); /* ILLEGAL REFERENCE to UNTYPED PROCEDURE */
}

byte GetCallArgCnt() {
    byte i, j, k;
    i = (byte)parseStack[parseSP];
    j = (byte)operatorStack[operatorSP];
    PopParseStack();
    PopOperatorStack();
    if (i == j)
        return i;
    if (i < j) {
        WrTx2ExtError(153); /* INVALID NUMBER OF ARGUMENTS IN CALL, TOO MANY */
        k = j - i;
        while (k != 0) {
            ExprPop();
            k--;
        }
    } else {
        WrTx2ExtError(154); /* INVALID NUMBER OF ARGUMENTS IN CALL, TOO FEW */
        k = i - j;
        while (k != 0) {
            ExprPush2(I_NUMBER, 0);
            k--;
        }
    }
    return i;
}

void Sub_4DCF(byte arg1b) {
    if (NotMatchTx1Item(L_LPAREN)) {
        WrTx2ExtError(ERR124); /* MISSING ARGUMENTS FOR BUILT-IN PROCEDURE */
        ExprPush2(I_NUMBER, 0);
    } else {
        if (NotMatchTx1Item(L_IDENTIFIER)) {
            WrTx2Error(ERR125); /* ILLEGAL ARGUMENT FOR BUILT-IN PROCEDURE */
            ExprPush2(I_NUMBER, 0);
        } else {
            ChkIdentifier();
            if (MatchTx1Item(L_LPAREN)) {
                if ((info->flag & F_ARRAY)) {
                    ResyncRParen();
                    if (MatchTx1Item(L_RPAREN)) {
                        if (arg1b == I_LENGTH || arg1b == I_LAST) {
                            WrTx2ExtError(ERR125); /*  ILLEGAL ARGUMENT FOR BUILT-IN PROCEDURE */
                            ExprPush2(I_NUMBER, 0);
                        } else
                            ExprPush2(arg1b, infoIdx);
                        return;
                    }
                } else
                    WrTx2ExtError(127); /* INVALID SUBSCRIPT ON NON-ARRAY */
            }
            if (MatchTx1Item(L_PERIOD)) {
                if (info->type != STRUCT_T)
                    WrTx2ExtError(110); /* INVALID LEFT OPERAND OF QUALIFICATION, not A structure */
                else if (NotMatchTx1Item(L_IDENTIFIER))
                    WrTx2ExtError(111); /* INVALID RIGHT OPERAND OF QUALIFICATION, not IDENTIFIER */
                else
                    ChkStructureMember();
            }
            if (MatchTx1Item(L_LPAREN)) {
                if ((info->flag & F_ARRAY)) {
                    ResyncRParen();
                    if (arg1b == I_LENGTH || arg1b == I_LAST) {
                        WrTx2ExtError(125); /* ILLEGAL ARGUMENT FOR BUILT-IN procedure */
                        ExprPush2(I_NUMBER, 0);
                    } else
                        ExprPush2(arg1b, infoIdx);
                } else {
                    WrTx2ExtError(127); /* INVALID SUBSCRIPT ON NON-ARRAY */
                    ExprPush2(I_NUMBER, 0);
                }
            } else if (arg1b == I_LENGTH || arg1b == I_LAST) {
                if ((info->flag & F_ARRAY))
                    ExprPush2(arg1b, infoIdx);
                else {
                    WrTx2ExtError(157);
                    /* INVALID ARGUMENT, ARRAY REQUIRED FOR length() or last() */
                    ExprPush2(I_NUMBER, 0);
                }
            } else {
                ExprPush2(arg1b, infoIdx);
                ExprPush2(I_LENGTH, infoIdx);
                ExprMakeNode(I_STAR, 2);
            }
        }
        ExpectRParen(126); /* MISSING ') ' AFTER BUILT-IN PROCEDURE ARGUMENT LIST */
    }
}

void MkIndexNode() {
    word p, r;
    byte i;

    SetInfo(eStack[exSP - 1].info);       /* get var */
    if (eStack[exSP].op1 == I_PLUSSIGN) { /* see if (index is of form expr + ?? */
        p = eStack[exSP].info + 1;
        if (sStack[p].op1 == I_NUMBER) { /* expr + number */
            eStack[exSP] = sStack[p - 1];
            ExprPush2(I_NUMBER, sStack[p].info); /* and get the number as an offset */
        } else
            ExprPush2(I_NUMBER, 0); /* no simple 0 offset */
    } else
        ExprPush2(I_NUMBER, 0); /* 0 offset */

    if (info->type == ADDRESS_T) /* simple word array */
        i = I_WORDINDEX;
    else {
        i = I_BYTEINDEX;
        if (info->type == STRUCT_T) { /* scale structure index */
            r = exSP - 1;             /* the index expr (ex offset) */
            ExprPush3(&eStack[r]);    /* calc dimension */
            ExprPush2(I_SIZE, infoIdx);
            ExprMakeNode(I_STAR, 2);
            eStack[r] = eStack[exSP]; /* replace index expr */
            ExprPop();                /* waste intermediate */
        }
    }
    ExprMakeNode(i, 3);
}

void ParsePortNum(byte arg1b) {
    word portNum = 0;
    if (MatchTx1Item(L_LPAREN)) {
        if (MatchTx1Item(L_NUMBER)) {
            if (tx1Item.dataw[0] <= 255)
                portNum = tx1Item.dataw[0];
            else
                WrTx2ExtError(106); /* INVALID Input/OUTPUT PORT NUMBER */
        } else
            WrTx2ExtError(107); /* ILLEGAL Input/OUTPUT PORT NUMBER, not NUMERIC CONSTANT */
        ExpectRParen(108);      /* MISSING ') ' AFTER Input/OUTPUT PORT NUMBER */
    } else
        WrTx2ExtError(109); /* MISSING Input/OUTPUT PORT NUMBER */

    ExprPush2(arg1b, portNum);
}

void Sub_50D5() {
    if ((info->flag & F_REENTRANT) && procIdx && (infotab[procIdx].flag & F_REENTRANT))
        return;
    if (!(info->flag & F_DECLARED))
        WrTx2ExtError(169); /* ILLEGAL FORWARD call */
    else if (!(info->flag & F_DEFINED))
        WrTx2ExtError(170); /* ILLEGAL RECURSIVE call */
}

byte Sub_512E(word arg1w) {
    byte c;

    if ((c = eStack[arg1w].op1) == I_OUTPUT || c == I_STACKPTR || c == I_BASED)
        return false;
    if (c == I_IDENTIFIER)
        infoIdx = eStack[arg1w].info;
    else if (c == I_BYTEINDEX || c == I_WORDINDEX)
        infoIdx = sStack[eStack[arg1w].info].info;
    else if (c == I_MEMBER) {
        if (sStack[eStack[arg1w].info].op1 == I_IDENTIFIER)
            infoIdx = sStack[eStack[arg1w].info].info;
        else
            infoIdx = sStack[sStack[eStack[arg1w].info].info].info;
    } else
        return true;
    info = &infotab[infoIdx];
    if ((info->flag & F_DATA))
        WrTx2ExtError(173);
    /*  INVALID LEFT SIDE OF ASSIGNMENT: VARIABLE DECLARED WITH data ATTRIBUTE */
    return false;
}

void ConstantList() {
    CreateInfo(256, BYTE_T, 0);            // create an info block to hold the list
    info->flag |= F_DATA;                  // mark as data
    ExprPush2(I_IDENTIFIER, infoIdx);      // push the identifier
    info->flag |= F_ARRAY | F_STARDIM;     // set as a byte array with * dim
    info->dim = InitialValueList(infoIdx); // get the list and record number of bytes
}
