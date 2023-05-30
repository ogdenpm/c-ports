/****************************************************************************
 *  asm2m.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// vim:ts=4:expandtab:shiftwidth=4:
//
#include "asm80.h"

/*
    0    -> ?
    1    -> start single word expression
    5    -> single byte opcode no operand
    7    -> collect reg / imm8 operand via acc1
    0Fh    -> binary topOp
    0Dh    -> unary topOp
    17h    -> collect IMM8 operand via acc2
    37h    -> collect imm16 operand via acc2
    40h    -> list
    47h    -> 2 operand topOp
    4Dh    -> start word list
    80h    -> end expression
    81h    -> DoRept operand
    0C0h    -> DoLocal operand

    -------x    -> getnum to acc1 & copy to acc2
    ------x-    -> getnum to acc1
    -----x--    -> collect low(acc1)
    ----x---    -> collect high(acc1)
    ---x----    -> collect low(acc2)
    --x-----    -> collect high(acc2)
    -x------    -> list
*/

byte opFlags[] = {
    /* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
       0, 0x80,   0,   0, 0xF, 0xF,0x80, 0xF, 0xD, 0xF, 0xD, 0xF, 0xF, 0xF, 0xF, 0xF,
     0xF,  0xD, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xD, 0xD,0x40,0x4D,   1,   1,   1,   1,
     0x80,   1,   0,   0,0x47,   7,   7,   7,0x17,0x47,   7,0x47,0x37,   5,   7,   0,
       0,    0,0x40,0x40,   0,   1,0x80,0x40,0x80,   0,0x40,0x80,0x80,0x40,0x81,0xC0,
     0x80, 0xD
};

static byte noRegOperand[] = { 0x41, 0, 0, 0, 0x19, 0x40, 0, 0x1C, 0, 0 };
/* bit vector 66 -> 0 x 24 00011001 01000000 00000000 00011100 00000000 00 */
static byte validRelocExprOp[] = { 0x1A, 5, 0x80, 0, 0xC0 };
/* bit vector 27 -> 00000101 10000000 00000000 110 */
/* +, -, unary+, HIGH, LOW*/
static byte opIncompat[] = { 0x57, 0x71, 0xF4, 0x57, 0x76, 0x66, 0x66, 0x67, 0x77, 0x77, 0x77, 0x55 };
/* bit vector 88 -> 01110001 11110100 01010111 01110110
                    01100110 01100110 01100111 01110111
                    01110111 01110111 01010101 */
static byte propagateFlags[] = { 0x57, 6, 2, 0x20, 0, 0, 0, 0, 0, 0, 0, 0x22 };
/* bit vector 88 -> 00000110 00000010 00100000 00000000
                00000000 00000000 00000000 00000000
                00000000 00000000 00100010 */
static byte typeHasTokSym[] = { 0x3A, 0xFF, 0x80, 0, 0, 0xF, 0xFE, 0, 0x20 };
/* bit vector 59 -> 11111111 10000000 00000000 00000000
                    00001111 11111110 00000000 001 */
                    /* BEGIN, EOLCH, LPAREN, RPAREN/O_LABEL, STAR, PLUS/K_SPECIAL, COMMA, */
                    /* MINUS/K_REGNAME, UPLUS/K_SP */
                    /* LXI, REG16, LDSTAX, ARITH, IMM8, MVI, INRDCR. MOV, IMM16, SINGLE */
                    /* RST */

/* precedence table */
/*
    10 - NULL
    9 - HIGH, LOW
    8 - *, /, MOD, SHL, SHR
    7 - +, -, UPLUS, UMINUS
    6 - =, <, <=, >, >=, !=
    5 - NOT
    4 - AND
    3 - OR, XOR,
    2 - ! used
    1 - COMMA, DB - STKLEN, O_MACROPARAM, ENDM, EXITM, O_3D, REPT, LOCAL
    0 - BEGIN,EOLCH,LPAREN,RPAREN,MACRO,MACRONAME,IRP,IRPC
*/
byte precedence[] = {
    /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
       0, 0, 0, 0, 8, 7, 1, 7, 7, 8, 7, 6, 6, 6, 6, 6,
       6, 5, 4, 3, 3, 8, 8, 8, 9, 9, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1,
       0, 10
};


bool TestBit(byte bitIdx, pointer bitVector) {
    // ch based bitVector byte;

    return bitIdx <= bitVector[0] && bitVector[bitIdx / 8 + 1] & (1 << (7 - (bitIdx % 8)));
}

bool IsReg(byte type) {
    return type == K_REGNAME || type == K_SP;
}

void ChkInvalidRegOperand(void) {
    if (TestBit(topOp, noRegOperand))
        if (IsReg(acc1ValType))
            OperandError();
}

void ResultType(void) {
    if (IsReg(acc1ValType))         // registers cannot be in full expressions
        OperandError();
    if (!(opFlags[topOp] & 2))      // only single operand
        acc2RelocFlags = 0;              // clear out flags
    else if (IsReg(acc2ValType))    // registers cannot be in full expressions
        OperandError();

    acc1ValType = O_NUMBER;

    if ((acc1RelocFlags & UF_SEGMASK) && (acc2RelocFlags & UF_SEGMASK))
        if ((acc1RelocFlags ^ acc2RelocFlags) & (UF_SEGMASK | UF_RBOTH))  /* must have same seg and relocation info */
            ExpressionError();

    bool isReloc1  = acc1RelocFlags & UF_RBOTH;
    bool isReloc2  = acc2RelocFlags & UF_RBOTH;
    bool isExtern1 = acc1RelocFlags & UF_EXTRN;
    bool isExtern2 = acc2RelocFlags & UF_EXTRN;
    if (isExtern1 || isExtern2) { /* either extern ?*/
        if (topOp == PLUS && !(isExtern1 || isReloc1)) { // ok for abs + extern/reloc
            acc1RelocVal = acc2RelocVal;            /* add relocation information from acc2*/
            acc1RelocFlags = acc2RelocFlags;
        } else if (isExtern2 || isReloc2 || !TestBit(topOp, validRelocExprOp)) {
            ExpressionError();
            acc1RelocFlags = 0;
        }
        return;
    }
    byte bitIdx = ((topOp - 4) << 2) | (isReloc1 ? 2 : 0) | (isReloc2 ? 1: 0);
    if (TestBit(bitIdx, opIncompat)) {              // check operation is compatible with operands
        ExpressionError();
        acc1RelocFlags = 0;
    } else if (TestBit(bitIdx, propagateFlags)) {
        if (!isReloc1)                          // only copy flags if not relocatable already
            acc1RelocFlags = acc2RelocFlags;
    } else
        acc1RelocFlags = 0;     // is absolute value
}


void SwapAccBytes(void) {
    accum1 = (word)((accum1 >> 8) + (accum1 << 8));
}



void SetExpectOperands(void) {
    expectOperand = true;
    expectOpcode = false;
}


void LogError(byte ch) {
    if (tokenStk[tokenIdx].type != NULVAL) {  /* ignore error if processing an optional value */
        SourceError(ch);
        return;
    }
    if (token.size == 0)                  /* make into a NUL */
        tokenStk[tokenIdx].type = NUL;
}

word GetNumVal(void) { // load numeric value from top of stack

    acc1RelocFlags = 0;         // initialise to absolute zero value
    accum1 = 0;
    acc1ValType = O_NAME;       // with NAME type
    if (token.type == NULVAL)
        PushToken(O_PARAM);
    if (tokenIdx == 0 || (token.type == O_DATA && !b6B36))
        LogError('Q');		// questionable syntax - possible missing opcode
    else {
        if (token.type == O_NAME || token.type == COMMA)      // can't handle undefined name or missing name
            LogError('U');	// undefined symbol - if here in pass2 then genuine error
        else {
            acc1ValType = token.type;                             // update the value type
            if (TestBit(acc1ValType, typeHasTokSym)) {
                acc1RelocFlags = token.symbol->flags & ~UF_PUBLIC; /* remove public attribute */
                *(wpointer)tokenStart = acc1RelocVal   = token.symbol->value;
                token.size = 2;        /* word value */
            } else if (token.size == 0)
                LogError('V');		// value illegal
            else {
                if (token.size > 2)
                    LogError('V');
                acc1RelocFlags = token.attr & ~UF_PUBLIC;    /* remove public attribute */
                acc1RelocVal = token.symId;        /* use the symbol Id() */
            }

            /* modified to avoid assumption of little endian */
            if (token.size > 1) { /* and high byte if ! a register */
                accum1 = *(wpointer)tokenStart;
                if (token.type == K_REGNAME)
                    accum1 &= 0xff;
            } else if (token.size > 0) /* get low byte */
                accum1 = *tokenStart;
        }

        // For 16 bit string operand swap bytes
        if (has16bitOperand && token.size == 2 && token.type == O_STRING)
            SwapAccBytes();

        if ((acc1RelocFlags & UF_EXTRN) && token.type < 9)
            accum1 = 0;

        PopToken();
    }

    b6B36 = false;
    return accum1;
}


byte GetPrec(byte topOp) {
    return precedence[topOp];
}

/*
   control
   xxxxxxx1    acc2 -> 16 bit reg
   xxxxxx1x    acc1 = acc1 | acc2
   xxxxx1xx    acc2 = rol(acc2, 3)
   xxxx1xxx    acc2 -> 8 bit value
   nnnnxxxx    topOp = LXI + nnnn

*/
void MkCode(byte control) {
    if ((control & 3)) {   /* lxi, ldax, stax, regarith, mvi, mov, rst */
        if (accum2 > 7    /* reg or rst num <= 7 */
            || (control & accum2 & 1)    /* only B D H SP if lxi, ldax or stax */
            || ((control & 3) == 3 && Low(accum2) > 2)    /* B or D if ldax or stax */
            || (!IsReg(acc2ValType) && topOp != RST))    /* reg unless rst */
            OperandError();
        else if (IsReg(acc2ValType) && topOp == RST)         /* cannot be reg for rst */
            OperandError();
        if (control & 4)
            accum2 = MkWord(High(accum2), Low((accum2 << 3) | ((accum2 >> 5) & 7)));
        accum1 |= Low(accum2);
    } else if (topOp != SINGLE)        /* single byte topOp */
        if (IsReg(acc2ValType))
            OperandError();

    if (control & 8) {
        if ((acc2RelocFlags & UF_RBOTH) == UF_RBOTH) {       // can't support 16bit relocatable as 8 bit value
            ValueError();
            acc2RelocFlags = (acc2RelocFlags & ~UF_RBOTH) | UF_RLOW;  // assume low 8 bits
        }
        if (accum2 >= 0x100 && accum2 < 0xFF00)    /* Error() if ! FF or 00 */
            ValueError();
    }
    if (topOp == IMM8 || topOp == IMM16) {   /* Imm8() or imm16 */
        acc1RelocFlags = acc2RelocFlags;                  // copy over relocation info for 8 or 16 bit imm
        acc1RelocVal = acc2RelocVal;
    } else
        acc1RelocFlags = 0;                          // else make abs non relocatable

    if (topOp != SINGLE)             /* single byte topOp */
        if (Low(accum1) == 0x76)         /* mov m,m is actually Halt() */
            OperandError();
    if ((topOp = (control >> 4) + LXI) == LXI)
        nextTokType = O_DATA;
}

byte NxtTokI(void) {
    if (tokI >= tokenIdx)
        return 0;
    return ++tokI;
}



bool ShowLine(void) {
    return (((!isControlLine) && controls.list) || (ctlListChanged && isControlLine))
        && (expandingMacro <= 1 || controls.gen)
        && (!(condAsmSeen || skipIf[0]) || controls.cond);
}

/*
    xrefMode= 0 -> defined
        = 1 -> used
        = 2 -> finalise
*/
void EmitXref(byte xrefMode, char const *name) {

    if ((!IsPhase1() || !controls.xref || IsSkipping()) && !xRefPending)
        return;
    InsertXref(xrefMode == XREF_DEF, name, srcLineCnt);
}
