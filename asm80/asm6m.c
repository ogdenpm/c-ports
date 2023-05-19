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
#include <ctype.h>

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

_Noreturn void FileError(void)
{
    RuntimeError(RTE_FILE);
    exit(1);
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
    token[0]          = token[tokenIdx];
    if (tokenIdx)
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
            macro[macroDepth] = curMacro;
            curMacro.condSP = macroCondSP;
            curMacro.ifDepth = ifDepth;
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
        macroCondSP = curMacro.condSP;
        ifDepth = curMacro.ifDepth;
    }

    macroCondStk[0] = macroCondStk[macroCondSP];    /* restore macro stack */
    macroCondSP--;
    if (sw == 1) {         /* is unnest macro */
       curMacro = macro[macroDepth];
        ReadM(curMacro.blk);
        savedMtype = curMacro.mtype;
        if (--macroDepth == 0) { /* end of macro nest */
            expandingMacro = 0;     /* not expanding */
            baseMacroTbl = &macroParams[MAXMACROPARAM - 1];
        }
    } else {
        skipIf[0] = skipIf[ifDepth];    /* pop skipIf and inElse status */
        inElse[0] = inElse[ifDepth];
        ifDepth--;
    }
}

void PushToken(byte type) {
    if (tokenIdx >= 8)
        StackError();
    else {
        token[++tokenIdx] = token[0];
        token[0].start += token[0].size; /* advance for next token */
        token[0].type = type;
        token[0].attr = token[0].size = 0;
        token[0].symbol               = NULL;
        token[0].symId                = 0;
    }
#ifdef TRACE
    DumpTokenStack(false);
#endif
}

void CollectByte(byte c)
{
    pointer s;

    if ((s = tokPtr + token[0].size) < endLineBuf) {   /* check for lineBuf overrun */
        *s = c;
        token[0].size++;
    }
    else
        StackError();
}

void GetId(byte type)
{
    PushToken(type);    /* save any previous token and initialise this one */
    reget = 1;        /* force re get of first character */

    while ((type = GetChClass()) == CC_DIG || type == CC_LET || type == CC_DOLLAR) {    /* digit || letter */
        if (type != CC_DOLLAR)
            CollectByte(toupper(curChar));
    }
    reget = 1;        /* force re get of Exit() char */
}


void GetNum(void) {
    word num;
    byte radix, digit, i;
//    byte chrs based tokPtr [1];

    GetId(O_NUMBER);
    radix = tokPtr[--token[0].size];
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
        token[0].size--;

    num = 0;
    for (i = 0; i <= token[0].size; i++) {
        if (tokPtr[i] == '?' || tokPtr[i] == '@') {
            IllegalCharError();
            digit = 0;
        } else {
            if ((digit = tokPtr[i] - '0') > 9)
                digit = digit - 7;
            if (digit >= radix)
                if (! (token[2].type == O_NULVAL)) { /* bug? risk that may be random - tokenIdx may be < 2 */
                    IllegalCharError();
                    digit = 0;
                }
        }

        num = num * radix + digit;
    }
    /* replace with packed number */
    token[0].size = 0;
    CollectByte((num) & 0xff);
    CollectByte((num) >> 8);
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