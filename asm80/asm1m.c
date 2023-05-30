/****************************************************************************
 *  asm1m.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

 // vim:ts=4:expandtab:shiftwidth=4:
#include "asm80.h"
#include <ctype.h>
byte tokReq[] = {
    /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
       0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
       0, 0 };

static byte definingLHS[] = { 0x36, 0, 0, 0, 6, 0, 0, 2 };
/* bit vector 55 -> 0 x 24 00000110 0 x 16 0000001 */
/* 29, 30, 55 */
bool absValueReq[] = {
    /* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
       false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
       false, false, false, false, false, false, false, false, false, false, false, false, true,  false, false, true,
       false, true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false,
       false, false, false, false, false, false, false, false, false, false, true,  true,  true, false,  true,  false,
       false, false };
/* true for DS, ORG, IF, 3A?, IRP, IRPC DoRept */

static byte b3F88[] = { 0x41, 0x90, 0, 0, 0, 0, 0, 0, 0, 0x40 };
/* bit vector 66 -> 10010000 0 x 56 01 */

void SkipNextWhite(void) {
    while (GetCh() == ' ' || IsTab())
        ;
}


byte NonHiddenSymbol(void) {
    const char *s = token.symbol->name;
    return s[0] != '?' || s[1] != '?' || !isdigit(s[2]) || !isdigit(s[3]) || !isdigit(s[4]) ||
           !isdigit(s[5]) || s[6];
}


// flush the macro buffer to disk in full 128 byte blocks
// residual are moved to start of macro buffer
void FlushM(bool fin) {

    if ((mSpoolMode & 1)) { /* spool macros to disk in 128 byte blocks */
        if (macroInIdx >= 128) {
            WriteM(macroText, macroInIdx / 128);
            if (macroInIdx & 0x7f)
                memcpy(macroText, macroText + (macroInIdx & ~0x7f), macroInIdx & 0x7f);
            macroInIdx &= 0x7f;
        }
    }
    if (fin)
        WriteM(macroText, 1);
}


void SkipWhite(void) {
    while (IsWhite())
        curChar = GetCh();
}



void Skip2NextLine(void) {
    Skip2EOL();
}


void gotValue(void) {
    if (curOp == VALUE)		// if previous was value then two consecutive values so error
        ExpressionError();
    inExpression = 0;
    curOp = VALUE;			// record we just saw a value
}

void Tokenise(void) {
    while (true) {
        if (atStartLine) {
            ParseControlLines();
            atStartLine = false;
        }

        switch (GetChClass()) {
        case CC_ESC:	// moved to allow fall through to error case rather than use goto
            if (expandingMacro) {
                skipIf[0] = false;
                yyType = NULVAL;
                return;
            }
        case CC_BAD:
            IllegalCharError(); break;
        case CC_WS: break;
        case CC_SEMI:
            if (!inQuotes) {
                inComment = true;
                if (GetChClass() == CC_SEMI && (mSpoolMode & 1)) {
                    excludeCommentInExpansion = true;
                    macroInIdx -= 2;		// remove the ;;
                }
                Skip2NextLine();			// process the rest of the line
                yyType = EOL;
                return;
            }
            break;
        case CC_COLON:
            if (!haveLabel) {				// ok if only label on line
                if (skipIf[0] || (mSpoolMode & 1))	// skipping or spooling so junk tokens
                    PopToken();
                else {
                    labelUse = L_TARGET;			// note label usage and update symbol table with location
                    UpdateSymbolEntry(segLocation[activeSeg], O_TARGET);
                }
                expectOperand = false;			// should see an opcode first
                haveLabel = expectOpcode = true;
            }
            else {
                SyntaxError();				// two labels is an error
                PopToken();
            }
            EmitXref(XREF_DEF, name);		// this is a x-ref definition
            haveUserSymbol = false;			// not a user symbol but a label
            curOp = O_LABEL;
            break;
        case CC_EOL:
            yyType = EOL;
            inQuotes = false;		// auto close string for spooling
            return;
        case CC_PUN:
            if (curChar == '+' || curChar == '-')
                if (!TestBit(curOp, b3F88)) /* not BEGIN, RPAREN or NUL */
                    curChar += (UPLUS - PLUS);    /* make unary versions */
            yyType = curChar - '(' + LPAREN;
            return;
        case CC_DOLLAR:
            PushToken(O_NUMBER);        // $ is treated as a number
            CollectByte(segLocation[activeSeg] & 0xff); /* its value is the current seg's size*/
            CollectByte(segLocation[activeSeg] >> 8);
            if (activeSeg != SEG_ABS)   // if not abs set seg and relocatable flags
                token.attr |= activeSeg | UF_RBOTH;
            gotValue();
            break;
        case CC_QUOTE:
            if (yyType == MACROARG) {		// quote not allowed in macro parameter (unless escaped)
                IllegalCharError();
                return;
            }
            if (mSpoolMode & 1)					// spooling passes through string, record whether in / out of string
                inQuotes = !inQuotes;
            else {
                GetStr();						// normal processing so collect string
                if (expectOpcode)			// can't be an opcode
                    SetExpectOperands();
                gotValue();
            }
            break;
        case CC_DIG:
            GetNum();
            if (expectOpcode)				// can't be an opcode
                SetExpectOperands();
            gotValue();
            break;
        case CC_LET:
            startMacroTokenIdx = macroInIdx - 1;
            GetTokenText(O_NAME);    /* assume it's a name */
            if (token.size > MAXSYMSIZE)  /* cap length */
                token.size = MAXSYMSIZE;
            tokenStart[token.size] = '\0';

            if (controls.xref) {
                strcpy(savName, name);
            }
            /* copy the token to name */
            strcpy(name, (char *)tokenStart);
            nameLen = token.size;

            if (haveUserSymbol) {			// user symbol not followed by a colon
                haveNonLabelSymbol = true;
                haveUserSymbol = false;
            }


            if (Lookup(TID_MACRO) != O_NAME && (mSpoolMode & 1)) {
                bool isType0 = !token.type; // assignment pulled out to allow short circuit tests
                if (!inQuotes || (isType0 && (curChar == '&' || macroText[startMacroTokenIdx -1] == '&'))) {
                    macroInIdx = startMacroTokenIdx;
                    InsertByteInMacroTbl(isType0 ? 0x80 : 0x81);
                    InsertByteInMacroTbl((byte)GetNumVal());
                    InsertByteInMacroTbl(curChar);
                    yyType = O_NAME;			// reuse of yyType?
                }
            }
            else if (yyType != MACROARG && mSpoolMode != 2) {		// skip if capturing macro parameter or local names
                if (Lookup(TID_KEYWORD) == O_NAME) {       /* not a key word */
                    token.type = Lookup(TID_SYMBOL);    /* look up in symbol space */
                    haveUserSymbol = true;        /* note we have a user symbol */
                }

                yyType = token.type;
                needsAbsValue = absValueReq[token.type]; /* DS, ORG, IF, MACRONAME, IRP, IRPC DoRept */
                if (!tokReq[token.type]) /* i.e. not instruction, reg or MACRONAME or punctuation */
                    PopToken();

                // for name seen to lhs of op emit xref, for SET/EQU/MACRO PARAM then this is defining
                // else it is reference
                if (haveNonLabelSymbol) {               /* EQU, SET or O_MACROPARAM */
                    EmitXref(!TestBit(yyType, definingLHS), savName);   // maps to XREF_DEF or XREF_REF
                    haveNonLabelSymbol = false;
                }
            }
            if (mSpoolMode == 1) {					// start of macro spooling
                if (yyType == LOCAL) {			// move to capture locals
                    mSpoolMode = 2;
                    if (spooledControl)				// error if there are any controls before local in macro
                        SyntaxError();
                    spooledControl = false;
                } else {
                    spooledControl = false;			// clear spooled control flag
                    mSpoolMode = 0xff;				// capture body
                }
            }

            if (yyType == NUL)
                PushToken(NULVAL);
            if (yyType < 10 /* | yyType == 9 | 80h*/) { /* !! only first term contributes */
                gotValue();
                if (expectOpcode)
                    SetExpectOperands();
            }
            else {
                expectOpcode = false;
                return;
            }
            break;
        case CC_MAC:
            nestedMacroSeen = false;
            GetMacroToken();
            if (nestedMacroSeen)
                return;
            break;
        }
    }
}
