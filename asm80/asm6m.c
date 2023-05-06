/****************************************************************************
 *  asm6m.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"

void SyntaxError(void)
{
    SourceError('Q');
}

void DoubleDefError(void)
{
    SourceError('D');
}

void ExpressionError(void)
{
    SourceError('E');
}

void CommandError(void)
{
    SourceError('C');
}

void OpcodeOperandError(void)
{
    SourceError('O');
}

void NameError(void)
{
    SourceError('R');
}

void MultipleDefError(void)
{
    SourceError('M');
}

void ValueError(void)
{
    SourceError('V');
}

void NestingError(void)
{
    SourceError('N');
}

void PhaseError(void)
{
    SourceError('P');
}

void StackError(void)
{
    RuntimeError(RTE_STACK);
}

void FileError(void)
{
    RuntimeError(RTE_FILE);
}

void IllegalCharError(void)
{
    SourceError('I');
}

void BalanceError(void)
{
    SourceError('B');
}

void UndefinedSymbolError(void)
{
    SourceError('U');
}

void LocationError(void)
{
    SourceError('L');
}

void OperandError(void)
{
    SourceError('X');
}

byte HaveTokens(void)
{
    return tokenIdx != 0;
}


void PopToken(void) {
#ifdef TRACE
    DumpTokenStack(true);
#endif
    tokStart[0] = tokStart[tokenIdx];
    tokenSym.stack[0] = tokenSym.stack[tokenIdx];
    tokenType[0] = tokenType[tokenIdx];
    tokenAttr[0] = tokenAttr[tokenIdx];
    tokenSize[0] = tokenSize[tokenIdx];
    tokenSymId[0] = tokenSymId[tokenIdx];
    if (HaveTokens())
        tokenIdx--;
}



/* nest - sw == 1 -> nest macro sw == 2 -> nest if */
void Nest(byte sw)
{
    macroCondStk[++macroCondSP] = macroCondStk[0];
    /* record whether current nest is macro of if */
    if ((macroCondStk[0] = sw) == 1) {
        if (++macroDepth > 9) {
            StackError();
            macroDepth = 0;
        } else {
            macro.stack[macroDepth] = macro.stack[0];
            //memcpy(&macro.stack[macroDepth], &macro.stack[0], sizeof(macro_t));
            macro.top.condSP = macroCondSP;
            macro.top.ifDepth = ifDepth;
            nestMacro = true;
        }
    } else {
        if (++ifDepth > 8) {
            StackError();
            ifDepth = 0;
        } else {
            skipIf[ifDepth] = skipIf[0];
            inElse[ifDepth] = inElse[0];
        }
    }
}


void UnNest(byte sw)
{
    if (sw != macroCondStk[0]) {   /* check for unbalanced unnest */
        NestingError();
        if (sw == 2)          /* ! macro unnest */
            return;
        macroCondSP = macro.top.condSP;
        ifDepth = macro.top.ifDepth;
    }

    macroCondStk[0] = macroCondStk[macroCondSP];    /* restore macro stack */
    macroCondSP--;
    if (sw == 1) {         /* is unnest macro */
        macro.stack[0] = macro.stack[macroDepth];
        memcpy(&macro.stack[0], &macro.stack[macroDepth], sizeof(macro_t));
        ReadM(macro.top.blk);
        savedMtype = macro.top.mtype;
        if (--macroDepth == 0) { /* end of macro nest */
            expandingMacro = 0;     /* not expanding */
            baseMacroTbl = Physmem() + 0xBF;
        }
    } else {
        skipIf[0] = skipIf[ifDepth];    /* pop skipIf and inElse status */
        inElse[0] = inElse[ifDepth];
        ifDepth--;
    }
}

void PushToken(byte type)
{
    if (tokenIdx >= 8)
        StackError();
    else {
        tokenIdx = tokenIdx + 1;
        tokStart[tokenIdx] = tokStart[0];
        tokenSym.stack[tokenIdx] = tokenSym.stack[0];
        tokenType[tokenIdx] = tokenType[0];
        tokenAttr[tokenIdx] = tokenAttr[0];
        tokenSize[tokenIdx] = tokenSize[0];
        tokenSymId[tokenIdx] = tokenSymId[0];    
        tokStart[0] += tokenSize[0];    /* advance for next token */
        tokenType[0] = type;
        tokenAttr[0] = tokenSize[0] = bZERO;
        tokenSym.stack[0] = NULL; // (tokensym_t *)wZERO; to work around gcc complaint
        tokenSymId[0] = wZERO;
    }
#ifdef TRACE
    DumpTokenStack(false);
#endif
}

void CollectByte(byte c)
{
    pointer s;

    if ((s = tokPtr + tokenSize[0]) < endLineBuf) {   /* check for lineBuf overrun */
        *s = c;
        tokenSize[0]++;
    }
    else
        StackError();
}

void GetId(byte type)
{
    PushToken(type);    /* save any previous token and initialise this one */
    reget = 1;        /* force re get of first character */

    while ((type = GetChClass()) == CC_DIG || type == CC_LET) {    /* digit || letter */
        if (curChar > 0x60)    /* make sure upper case */
            curChar = curChar & 0xDF;
        CollectByte(curChar);
    }
    reget = 1;        /* force re get of Exit() char */
}


void GetNum(void) {
    word accum;
    byte radix, digit, i;
//    byte chrs based tokPtr [1];

    GetId(O_NUMBER);
    radix = tokPtr[--tokenSize[0]];
    if (radix == 'H')
        radix = 16;

    if (radix == 'D')
        radix = 10;

    if (radix == 'O' || radix == 'Q')
        radix = 8;

    if (radix == 'B')
        radix = 2;

    if (radix > 16)
        radix = 10;
    else
        tokenSize[0]--;

    accum = 0;
    for (i = 0; i <= tokenSize[0]; i++) {
        if (tokPtr[i] == '?' || tokPtr[i] == '@') {
            IllegalCharError();
            digit = 0;
        } else {
            if ((digit = tokPtr[i] - '0') > 9)
                digit = digit - 7;
            if (digit >= radix)
                if (! (tokenType[2] == O_NULVAL)) { /* bug? risk that may be random - tokenIdx may be < 2 */
                    IllegalCharError();
                    digit = 0;
                }
        }

        accum = accum * radix + digit;
    }
    /* replace with packed number */
    tokenSize[0] = 0;
    CollectByte((accum) & 0xff);
    CollectByte((accum) >> 8);
}

void GetStr(void) {
    PushToken(O_STRING);

    while (GetCh() != CR) {
        if (curChar == '\'')
            if (GetCh() != '\'')	// if not '' then all done
                goto L6268;
        CollectByte(curChar);	// collect char - '' becomes '
    }

    BalanceError();				// EOL seen before closing '

L6268:
    reget = 1;					// push back CR or char after '
}