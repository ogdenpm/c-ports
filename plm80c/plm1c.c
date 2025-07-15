/****************************************************************************
 *  plm1c.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

bool use8bit;

// <restricted primary>
static word RestrictedPrimary() {
    word val;
    if (MatchTx1Item(T1_NUMBER))
        val = tx1Item.dataw[0];
    else {
        if (MatchTx1Item(T1_IDENTIFIER))
            ChkIdentifier();
        Wr2TokError(151); /* INVALID OPERAND IN RESTRICTED Expression */
        return 0;
    }
    if (val > 0xff)
        use8bit = false;
    return val;
}

// <restricted secondary>
static word RestrictedSecondary() {
    if (MatchTx1Item(T1_MINUSSIGN)) {
        word val = -RestrictedPrimary();
        return use8bit ? Low(val) : val;
    } else
        return RestrictedPrimary();
}

//  <restricted sum>
static word RestrictedSum() {
    word sum = RestrictedSecondary(); // get number
    while (1) {
        if (MatchTx1Item(T1_PLUSSIGN)) { // + number
            sum += RestrictedSecondary();
            if (use8bit)
                sum = Low(sum);
        } else if (MatchTx1Item(T1_MINUSSIGN)) { // - number
            sum -= RestrictedSecondary();
            if (use8bit)
                sum = Low(sum);
        } else
            break;
    }
    return sum;
}

static word GetRestrictedArrayIndex() {
    use8bit = true;
    if (!((info->flag & F_ARRAY)))
        Wr2TokError(149); /* INVALID SUBSCRIPTING IN RESTRICTED REFERENCE */
    word sum = RestrictedSum();
    ExpectRParen(150); /* MISSING ') ' AT END OF RESTRICTED SUBSCRIPT */
    return sum;
}

static void GetRestrictedVar() {
    if (NotMatchTx1Item(T1_IDENTIFIER)) {
        Wr2Error(ERR147); /* MISSING IDENTIFIER FOLLOWING DOT OPERATOR */
        RecoverRPOrEndExpr();
        return;
    }
    ChkIdentifier();
    var.infoIdx = ToIdx(info);     // record the var info
    if (info->type == BUILTIN_T) { // can't take the address of a builtin!!
        Wr2Error(ERR123);          /* INVALID DOT OPERAND, BUILT-IN PROCEDURE ILLEGAL */
        RecoverRPOrEndExpr();
        return;
    }
    if (MatchTx1Item(T1_LPAREN)) // is indexed
        var.subscript = GetRestrictedArrayIndex();

    if (MatchTx1Item(T1_PERIOD)) // a struct ref
    {
        info = FromIdx(var.infoIdx);  // recover the base  info
        if (info->type != STRUCT_T) { // not on a stuct
            Wr2TokError(ERR148);      /* INVALID QUALIFICATION IN RESTRICTED REFERENCE */
            RecoverRPOrEndExpr();
            return;
        }
        if (NotMatchTx1Item(T1_IDENTIFIER)) { // no identifier after the .
            Wr2Error(ERR147);                 /* MISSING IDENTIFIER FOLLOWING DOT OPERATOR */
            return;
        }
        ChkStructureMember();
        var.infoIdx = ToIdx(info);
        if (MatchTx1Item(T1_LPAREN)) // its a member array element reference
            var.memberSubscript = GetRestrictedArrayIndex();
    }
}

// <restricted reference> := .<identifier> [<restricted subscript>] [.<identifier> [<restricted
// subscript>]]
void RestrictedExpression() {
    var.infoIdx = var.subscript = var.memberSubscript = var.val = 0;
    if (MatchTx1Item(T1_PERIOD)) {
        GetRestrictedVar();
        if (MatchTx1Item(T1_PLUSSIGN)) // [<restricted adding operator>]
            ;
        else if (MatchTx1Item(T1_MINUSSIGN)) // let RestrictedSum handle -
            UngetTx1Item();
        else
            return;      // no so all done
        use8bit = false; // operations will be 16 bit
        var.val = RestrictedSum();
    } else if (MatchTx1Item(T1_RPAREN)) // closing ')'
        UngetTx1Item(); // unget for later processing
    else {
        use8bit = true; // assume 8 bit
        var.val = RestrictedSum();
    }
}

word InitialValueList(offset_t infoOffset) {
    word listLen = 0;
    WrAtByte(ATI_DHDR);
    WrAtWord(infoOffset);
    WrAtWord(curStmtNum);
    while (1) {
        if (MatchTx1Item(T1_STRING)) { // string initialisation
            WrAtByte(ATI_STRING);
            WrAtWord(tx1Item.len);             // write length
            WrAtBuf(tx1Item.str, tx1Item.len); // and the string
            info = FromIdx(infoOffset);
            listLen += info->type == ADDRESS_T ? (tx1Item.len + 1) / 2 : tx1Item.len;
        } else {
            RestrictedExpression(); // get the restricted Expression
            WrAtByte(ATI_DATA);
            WrAtBuf((pointer)&var, sizeof(var_t)); // and write the info
            listLen++;
        }
        if (NotMatchTx1Item(T1_COMMA)) // keep going if comma
            break;
        else if (MatchTx1Item(T1_RPAREN)) { // ) with no preceeding data
            Wr2Error(151);                  /* INVALID OPERAND IN RESTRICTED Expression */
            UngetTx1Item();
            break;
        }
    }
    WrAtByte(ATI_END);
    ExpectRParen(152); /* MISSING ') ' AFTER CONSTANT LIST */
    info = FromIdx(infoOffset);

    return listLen; // items read i.e. dimension
}

void ResetStacks() {
    parseSP = operandSP = operatorSP = stSP = 0;
}

void PushParse(word arg1w) {
    if (parseSP == 99)
        fatal_ov1(ERR119); /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
    parseStack[++parseSP] = arg1w;
}

word PopParse() {
    if (parseSP == 0)
        fatal_ov1(159); /* COMPILER ERROR: PARSE STACK UNDERFLOW */
    return parseStack[parseSP--];
}

static void PushOperand(operand_t *operand) {
    if (operandSP == EXPRSTACKSIZE - 1)
        fatal_ov1(ERR121); /* LIMIT EXCEEDED: Expression TOO COMPLEX */
    operandStack[++operandSP] = *operand;
}

void PopOperand() {
    if (operandSP == 0)
        fatal_ov1(160); /* COMPILER Error: OPERAND STACK UNDERFLOW */
    operandSP--;
}

static void SwapOperands() {
    if (operandSP < 2)
        fatal_ov1(161); /* COMPILER ERROR: ILLEGAL OPERAND STACK EXCHANGE */
    operand_t tmp               = operandStack[operandSP];
    operandStack[operandSP]     = operandStack[operandSP - 1];
    operandStack[operandSP - 1] = tmp;
}

void PushSimpleOperand(byte icode, word val) {
    operand_t operand = { icode, 0, { val } };
    PushOperand(&operand);
}

static void PushStmt(operand_t *operand) {
    if (stSP == 299)
        fatal_ov1(122); /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX   */
    tree[++stSP] = *operand;
}

void MoveOperandToTree() {
    PushStmt(&operandStack[operandSP]);
    PopOperand();
}

void PushOperator(byte arg1b) {
    if (operatorSP == 49)
        fatal_ov1(120); /* LIMIT EXCEEDED: Expression TOO COMPLEX */
    operatorStack[++operatorSP] = arg1b;
}

word PopOperator() {
    if (operatorSP == 0)
        fatal_ov1(162); /* COMPILER ERROR: OPERATOR STACK UNDERFLOW */
    return operatorStack[operatorSP--];
}

void PushComplexOperand(byte icode, byte childCnt) {
    // new operand has given instruction, number of children and where children are
    // are saved on the sStack
    operand_t operand = { icode, childCnt, { childCnt ? stSP + 1 : 0 } };

    if (operandSP < childCnt)   // not enough operands to use
        fatal_ov1(ERR163); /* COMPILER ERROR: GENERATION FAILURE  */
    // move children to sStack
    operandSP -= childCnt;
    for (int j = operandSP; childCnt; childCnt--) // move the arg information to the sStack
        PushStmt(&operandStack[++j]);

    PushOperand(&operand); // replace with new operand item
}

void ApplyOperator() {
    byte op = (byte)operatorStack[operatorSP];
    PushComplexOperand(op, op == I_NOT || op == I_UNARYMINUS ? 1 : 2);
    PopOperator();
}

static void PushIdentifier() {

    if ((info->flag & F_MEMBER)) {                            // if member create I_MEMBER node
        PushSimpleOperand(I_IDENTIFIER, ToIdx(info->parent)); // struct info
        PushSimpleOperand(I_IDENTIFIER, ToIdx(info));         // member info
        PushComplexOperand(I_MEMBER, 2);                      // new node
        info = info->parent;                                  // now point to struct
    } else
        PushSimpleOperand(I_IDENTIFIER, ToIdx(info)); // simple identifier node
}

void FixupBased(info_t *varInfo) {
    info = varInfo;               // var info
    if ((info->flag & F_BASED)) { // is it based?
        info = info->baseInfo;    // get the info of the base
        PushIdentifier();         // save it
        info = varInfo;
        SwapOperands();                 // swap var and base on stack
        PushComplexOperand(I_BASED, 2); // create a based node
    }
}

void PushVariable() {
    PushIdentifier();
    FixupBased(info);
}

void ChkTypedProcedure() {
    if (!info->returnType)
        Wr2TokError(131); /* ILLEGAL REFERENCE to UNTYPED PROCEDURE */
}

// on entry parseStack[top] -> number of expected args
//          operatorStack[top] -> actual number of args
// makes sure number of  arguments is correct
// removing / adding as needed
byte GetCallArgCnt() {
    word nArgExpected = PopParse();
    word nArgActual   = PopOperator();

    if (nArgExpected != nArgActual) {
        if (nArgExpected < nArgActual) {
            Wr2TokError(ERR153); /* INVALID NUMBER OF ARGUMENTS IN CALL, TOO MANY */
            while (nArgActual-- != nArgExpected)
                PopOperand();
        } else {
            Wr2TokError(ERR154); /* INVALID NUMBER OF ARGUMENTS IN CALL, TOO FEW */
            while (nArgExpected != nArgActual++)
                PushSimpleOperand(I_NUMBER, 0);
        }
    }
    return (byte)nArgExpected;
}

void ParseSizing(byte iCode) {
    if (NotMatchTx1Item(T1_LPAREN)) {
        Wr2TokError(ERR124); /* MISSING ARGUMENTS FOR BUILT-IN PROCEDURE */
        PushSimpleOperand(I_NUMBER, 0);
    } else {
        if (NotMatchTx1Item(T1_IDENTIFIER)) {
            Wr2Error(ERR125); /* ILLEGAL ARGUMENT FOR BUILT-IN PROCEDURE */
            PushSimpleOperand(I_NUMBER, 0);
        } else {
            ChkIdentifier();
            if (MatchTx1Item(T1_LPAREN)) {
                if ((info->flag & F_ARRAY)) {
                    ResyncRParen();                                 // skip subscript
                    if (MatchTx1Item(T1_RPAREN)) {                  // end of builtin ?
                        if (iCode == I_LENGTH || iCode == I_LAST) { // subscript only for SIZE
                            Wr2TokError(ERR125); /*  ILLEGAL ARGUMENT FOR BUILT-IN PROCEDURE */
                            PushSimpleOperand(I_NUMBER, 0);
                        } else
                            PushSimpleOperand(iCode, ToIdx(info)); // I_SIZE info - one item
                        return;
                    }
                } else
                    Wr2TokError(127); /* INVALID SUBSCRIPT ON NON-ARRAY */
            }
            if (MatchTx1Item(T1_PERIOD)) {
                if (info->type != STRUCT_T)
                    Wr2TokError(110); /* INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE */
                else if (NotMatchTx1Item(T1_IDENTIFIER))
                    Wr2TokError(111); /* INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER */
                else
                    ChkStructureMember();
            }
            if (MatchTx1Item(T1_LPAREN)) {
                if ((info->flag & F_ARRAY)) {
                    ResyncRParen(); // skip subscript
                    if (iCode == I_LENGTH || iCode == I_LAST) {
                        Wr2TokError(125); /* ILLEGAL ARGUMENT FOR BUILT-IN procedure */
                        PushSimpleOperand(I_NUMBER, 0);
                    } else
                        PushSimpleOperand(iCode, ToIdx(info));
                } else {
                    Wr2TokError(127); /* INVALID SUBSCRIPT ON NON-ARRAY */
                    PushSimpleOperand(I_NUMBER, 0);
                }
            } else if (iCode == I_LENGTH || iCode == I_LAST) {
                if ((info->flag & F_ARRAY))
                    PushSimpleOperand(iCode, ToIdx(info));
                else {
                    Wr2TokError(157);
                    /* INVALID ARGUMENT, ARRAY REQUIRED FOR LENGTH() or LAST() */
                    PushSimpleOperand(I_NUMBER, 0);
                }
            } else {
                PushSimpleOperand(iCode, ToIdx(info));    // I_SIZE info - one item
                PushSimpleOperand(I_LENGTH, ToIdx(info)); // number of items
                PushComplexOperand(I_STAR, 2);            // size is size one item * number of items
            }
        }
        ExpectRParen(126); /* MISSING ') ' AFTER BUILT-IN PROCEDURE ARGUMENT LIST */
    }
}
/*
    On entry
    eStack[exSP - 1]    -> var info
    eStack[exSP]        -> index expression
*/
void MkIndexNode() {
    info = FromIdx(operandStack[operandSP - 1].infoIdx); /* get var */
    if (operandStack[operandSP].icode == I_PLUSSIGN) {   /* see if (index is of form expr + ?? */
        word p = operandStack[operandSP].nodeIdx + 1;
        if (tree[p].icode == I_NUMBER) { /* expr + number */
            operandStack[operandSP] = tree[p - 1];
            PushSimpleOperand(I_NUMBER, tree[p].val); /* and get the number as an offset */
        } else
            PushSimpleOperand(I_NUMBER, 0); /* no simple 0 offset */
    } else
        PushSimpleOperand(I_NUMBER, 0); /* 0 offset */

    byte icode = info->type == ADDRESS_T ? I_WORDINDEX : I_BYTEINDEX;

    if (info->type == STRUCT_T) {      /* scale structure index */
        word r = operandSP - 1;        /* the index expr (ex offset) */
        PushOperand(&operandStack[r]); /* calc dimension */
        PushSimpleOperand(I_SIZE, ToIdx(info));
        PushComplexOperand(I_STAR, 2);
        operandStack[r] = operandStack[operandSP]; /* replace index expr */
        PopOperand();                              /* waste intermediate */
    }
    PushComplexOperand(icode, 3);
}

void ParsePortNum(byte inOutId) {
    word portNum = 0;
    if (MatchTx1Item(T1_LPAREN)) {
        if (MatchTx1Item(T1_NUMBER)) {
            if (tx1Item.dataw[0] <= 255)
                portNum = tx1Item.dataw[0];
            else
                Wr2TokError(106); /* INVALID Input/OUTPUT PORT NUMBER */
        } else
            Wr2TokError(107); /* ILLEGAL Input/OUTPUT PORT NUMBER, not NUMERIC CONSTANT */
        ExpectRParen(108);    /* MISSING ') ' AFTER Input/OUTPUT PORT NUMBER */
    } else
        Wr2TokError(109); /* MISSING Input/OUTPUT PORT NUMBER */

    PushSimpleOperand(inOutId, portNum);
}

void ChkIllegalCall() {
    if ((info->flag & F_REENTRANT) && curProc && (curProc->flag & F_REENTRANT))
        return;
    if (!(info->flag & F_DECLARED))
        Wr2TokError(169); /* ILLEGAL FORWARD call */
    else if (!(info->flag & F_DEFINED))
        Wr2TokError(170); /* ILLEGAL RECURSIVE call */
}

// check if top item is an lvalue
byte IsLValue(word sp) {
    byte c = operandStack[sp].icode;
    index_t infoIdx;

    if (c == I_OUTPUT || c == I_STACKPTR || c == I_BASED)
        return true;
    if (c == I_IDENTIFIER)
        infoIdx = operandStack[sp].infoIdx;
    else if (c == I_BYTEINDEX || c == I_WORDINDEX)
        infoIdx = tree[operandStack[sp].nodeIdx].infoIdx;
    else if (c == I_MEMBER) {
        if (tree[operandStack[sp].nodeIdx].icode == I_IDENTIFIER)
            infoIdx = tree[operandStack[sp].nodeIdx].infoIdx;
        else
            infoIdx = tree[tree[operandStack[sp].nodeIdx].nodeIdx].infoIdx;
    } else
        return false;
    info = FromIdx(infoIdx);
    if (info->flag & F_DATA)
        /* INVALID LEFT SIDE OF ASSIGNMENT: VARIABLE DECLARED WITH DATA ATTRIBUTE */
        Wr2TokError(173);
    return true;
}

void ConstantList() {
    CreateInfo(0x100, BYTE_T, 0);                 // create an info block to hold the list
    info->flag |= F_DATA;                         // mark as data
    PushSimpleOperand(I_IDENTIFIER, ToIdx(info)); // push the identifier
    info->flag |= F_ARRAY | F_STARDIM;            // set as a byte array with * dim
    info->dim = InitialValueList(ToIdx(info));    // get the list and record number of bytes
}
