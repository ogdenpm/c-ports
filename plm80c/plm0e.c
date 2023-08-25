/****************************************************************************
 *  plm0e.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

static byte ENDorSEMICOLON[2] = { T_END, T_SEMICOLON };
static byte tokenTypeTable[]  = {
    T_NUMBER, T_NUMBER, T_NUMBER, T_IDENTIFIER, T_IDENTIFIER, T_PLUSSIGN, T_MINUSSIGN,
    T_STAR,   T_SLASH,  T_LPAREN, T_RPAREN,     T_COMMA,      T_COLON,    T_SEMICOLON,
    T_STRING, T_PERIOD, T_EQ,     T_LT,         T_GT,         0,          0,
    0,        0,        0,        T_IDENTIFIER
};

byte binValidate[4] = { 0, 1, 1, 1 };
byte octValidate[4] = { 0, 0, 1, 1 };
byte decValidate[4] = { 0, 0, 0, 1 };
byte hexValidate[4] = { 0, 0, 0, 2 };

static byte Toupper(byte c) {
    if ('a' <= c && c <= 'z')
        c = c & 0x5F;
    return c;
}

// lifted to file scope because of nested procedure
byte tokenLen, ch;

static void Str2num(byte radix, pointer validateP) {
    word trial;
    byte valId;
    bool errored;
    byte curoff, ct;

    tokenVal = trial = 0;
    errored          = false;
    for (curoff = 1; curoff <= tokenLen; curoff++) {
        if ((ct = cClass[ch = tokenStr[curoff]]) > CC_HEXCHAR)
            valId = 1;
        else
            valId = validateP[ct];
        switch (valId) {
        case 0:
            ch -= '0';
            break;
        case 1:
            if (!errored) {
                Wr1TokenErrorAt(ERR5); /* INVALID CHARACTER IN NUMERIC CONSTANT */
                errored = true;
            }
            ch = 0;
            break;
        case 2:
            ch = ch - 'A' + 10;
            break;
        }

        if ((trial = tokenVal * radix + ch) < tokenVal) {
            Wr1TokenErrorAt(ERR94); /* ILLEGAL CONSTANT, VALUE > 65535 */
            return;
        }
        tokenVal = trial;
    }
}

static void Token2Num() {
    if (cClass[ch = tokenStr[tokenLen = tokenStr[0]]] <= CC_DECDIGIT)
        Str2num(10, decValidate);
    else {
        tokenLen--;
        if (ch == 'H')
            Str2num(16, hexValidate);
        else if (ch == 'Q' || ch == 'O')
            Str2num(8, octValidate);
        else if (ch == 'B')
            Str2num(2, binValidate);
        else if (ch == 'D')
            Str2num(10, decValidate);
        else {
            Wr1TokenErrorAt(ERR4); /* ILLEGAL NUMERIC CONSTANT TYPE */
            tokenVal = 0;
        }
    }
} /* Token2Num() */

static void NestMacro() {
    Wr1XrefUse();
    if (macroDepth == 10)
        Wr1TokenErrorAt(ERR7); /* LIMIT EXCEEDED: MACROS NESTED TOO DEEPLY */
    else {
        info->type = MACRO_T; // mark the type as  MACRO_T to spot recursive expansion
        macroPtrs[++macroDepth].text   = inChrP;   // push the current location
        macroPtrs[macroDepth].macroIdx = macroIdx; // and infoP
        inChrP   = (byte *)(info->lit->str) - 1;   // adjust for initial increment in GNxtCh()
        macroIdx = infoIdx;                        // set up the new infoP
    }
} /* NestMacro() */

static bool expandLit() {
    Lookup((pstr_t *)tokenStr);
    markedSymbolP = curSym;
    if (symtab[curSym].infoIdx >= 0xFF00) /* simple key word */
        tokenType = Low(symtab[curSym].infoIdx);
    else {
        index_t tmp = infoIdx; // if lit expand then restore infoIdx
        if (FindInfo()) {
            if (info->type == LIT_T) {
                NestMacro();
                SetInfo(tmp);
                return true;
            } else if (info->type == MACRO_T) {
                Wr1TokenErrorAt(ERR6); /* ILLEGAL MACRO REFERENCE, RECURSIVE EXPANSION */
                SetInfo(tmp);
                return true;
            }
        }
    }
    return false;
}

// updated version of GetName
// now allows _ in names, in numbers _ is ignored like $ is
static void GetName(word maxlen) {
    byte ct;
    word curOff = 1;

    ct          = cClass[nextCh];
    bool isnum  = ct <= CC_DECDIGIT;
    while (ct <= CC_ALPHA || nextCh == '$' || ct == CC_UNDERBAR) {
        if (nextCh == '$' || (isnum && ct == CC_UNDERBAR))
            GNxtCh();
        else if (curOff > maxlen) {
            Wr1TokenErrorAt(ERR3); /* IDENTIFIER, STRING, or NUMBER TOO LONG, TRUNCATED */
            while (ct <= CC_ALPHA || nextCh == '$' ||
                   ct == CC_UNDERBAR) { // junk rest of identifier
                GNxtCh();
                ct = cClass[nextCh];
            }
            curOff = maxlen + 1; // length will exceed limit
        } else {
            tokenStr[curOff++] = Toupper(nextCh); // save name in upper case
            GNxtCh();
        }
        ct = cClass[nextCh];
    }

    tokenStr[0] = (byte)(curOff - 1); // save pstr length
}

static void ParseString() {
    bool tooLong = false;
    word curOff  = 1;

    // code simplified in port to C
    while (1) {
        GNxtCh();
        if (nextCh == '\r' || nextCh == '\n') // ignore cr lf
            continue;
        if (nextCh == QUOTE) { // double quote passes through as single quote
            GNxtCh();
            if (nextCh != QUOTE) // single quote finishes string
                break;
        }
        if (curOff != MAXSTRING)
            tokenStr[curOff++] = nextCh;
        else {
            tooLong = true;
            if (nextCh == ';') {
                Wr1TokenErrorAt(ERR85); /* LONG STRING ASSUMED CLOSED AT NEXT SEMICOLON OR QUOTE */
                break;
            }
        }
    }
    wTokenLen = (word)(curOff - 1); // record length long string
    tokenStr[0] = wTokenLen < 256 ? (byte)wTokenLen : 255;
    if (wTokenLen == 0)
        Wr1TokenErrorAt(ERR189); /* NULL STRING NOT ALLOWED */
    if (tooLong)
        Wr1TokenErrorAt(ERR3); /* IDENTIFIER, STRING, or NUMBER TOO LONG, TRUNCATED */
}

static void LocYylex() {
    while (1) {
        byte chrClass = cClass[nextCh]; // the lookahead char
        tokenType     = tokenTypeTable[chrClass];
        switch (chrClass) {
        default: /* white space */
            do {
                GNxtCh();
            } while (cClass[nextCh] == CC_WSPACE);
            break;
        case CC_BINDIGIT:
        case CC_OCTDIGIT:
        case CC_DECDIGIT: // number
            GetName(31);
            Token2Num();
            return;
        case CC_HEXCHAR:
        case CC_ALPHA:
        case CC_UNDERBAR: // token or identifier
            GetName(255);
            if (!expandLit())
                return;
            GNxtCh(); // carry on processing
            break;
        case CC_PLUS:
        case CC_MINUS:
        case CC_STAR:
        case CC_LPAREN:
        case CC_RPAREN:
        case CC_COMMA:
        case CC_SEMICOLON:
        case CC_PERIOD:
        case CC_EQUALS:
            GNxtCh();
            return;
        case CC_SLASH:
            GNxtCh();
            if (nextCh != '*') // not start of comment
                return;
            GNxtCh();
            do {
                while (nextCh != '*')
                    GNxtCh();
                GNxtCh();
            } while (nextCh != '/'); // not end of comment
            GNxtCh();
            break;
        case CC_COLON:
            GNxtCh();
            if (nextCh == '=') {
                tokenType = T_COLON_EQUALS;
                GNxtCh();
            }
            return;
        case CC_QUOTE:
            ParseString();
            return;
        case CC_LESS:
            GNxtCh();
            if (nextCh == '>') {
                tokenType = T_NE;
                GNxtCh();
            } else if (nextCh == '=') {
                tokenType = T_LE;
                GNxtCh();
            }
            return;
        case CC_GREATER:
            GNxtCh();
            if (nextCh == '=') {
                tokenType = T_GE;
                GNxtCh();
            }
            return;
        case CC_DOLLAR:
        case CC_INVALID:
            Wr1TokenErrorAt(ERR1); /* INVALID PL/M-80 CHARACTER */
            GNxtCh();
            break;
        case CC_NONPRINT:
            Wr1TokenErrorAt(ERR2); /* UNPRINTABLE ASCII CHARACTER */
            GNxtCh();
            break;
        }
    }
} /* LocYylex() */

void Yylex() {
    static bool endToggle = false; // relies on C's false = 0, true = 1

    if (yyAgain)
        yyAgain = false;
    else if (afterEOF) // return alternating END and ; to allow recovery
        tokenType = ENDorSEMICOLON[endToggle = !endToggle];
    else {
        LocYylex();
        if (tokenType == T_EOF) {
            afterEOF  = true;
            tokenType = T_END;
        }
    }
}

void SetYyAgain() {
    yyAgain = true;
}

/*
    look for matching token
*/
bool YylexMatch(byte token) {
    Yylex(); /* get the token to check */
    if (tokenType == token)
        return true;
    else {
        SetYyAgain(); /* not matching but push back */
        return false;
    }
}

bool YylexNotMatch(byte token) {
    return !YylexMatch(token);
}

static void WrNestedExpression() {
    word nesting;

    nesting = 1;
    Wr1LexToken();
    Yylex();

    while (1) {
        if (tokenType == T_LPAREN)
            nesting++;
        else if (tokenType == T_RPAREN) {
            if ((--nesting) == 0) {
                Wr1LexToken();
                return;
            }
        } else if (tokenType == T_SEMICOLON || (tokenType >= T_CALL && tokenType <= T_RETURN)) {
            Wr1TokenErrorAt(ERR82); /* INVALID SYNTAX, MISMATCHED '(' */
            while (nesting != 0) {
                Wr1Byte(L_RPAREN);
                nesting--;
            }
            SetYyAgain();
            return;
        }
        Wr1LexToken();
        Yylex();
    }
}

// convert and write Expression into lex tokens
// stop when endTok, semicolon or new start symbol for <declaration> or <unit> seen
void ParseExpresion(byte endTok) {
    Yylex();
    while (tokenType != endTok && tokenType != T_SEMICOLON) {
        if (T_CALL <= tokenType && tokenType <= T_RETURN)
            break;
        Wr1LexToken();
        Yylex();
    }
    SetYyAgain();
}

/*
    Error() recovery to) or end of statement
    skip to ; or
        ) unless inside nested ()
*/
static void RecoverToSemiOrRP() {
    word nesting;

    nesting = 0;
    while (1) {
        if (tokenType == T_SEMICOLON)
            break;
        if (tokenType == T_LPAREN)
            nesting++;
        else if (tokenType == T_RPAREN && nesting-- == 0)
            break;
        Yylex(); /* get next token */
    }
    SetYyAgain(); /* push back token */
}

/*
    Error() recovery to next element in parameter list
    skip to ; or
        ) or , unless inside nested ()
*/
static void RecoverMissingParam() {
    word nesting;
    nesting = 0;
    while (1) {
        if (tokenType == T_SEMICOLON)
            break;
        if (nesting == 0 && tokenType == T_COMMA)
            break;
        if (tokenType == T_LPAREN)
            nesting++;
        else if (tokenType == T_RPAREN && nesting-- == 0)
            break;
        Yylex(); /* get next token */
    }
    SetYyAgain(); /* push back token */
}

static offset_t declNames[33];
static word declBasedNames[33];
static word declNameCnt;
static offset_t declaringName;

static word basedInfo;
static uint32_t dclFlags;
static byte dclType;
static pstr_t const *lastLit;
static word arrayDim;
static struct {
    index_t sym;
    word dim;
    byte type;
} member[33];

static word memberCnt;
static byte factoredParamCnt;
static bool isNewVar;

static void DeclarationError(word errcode) {
    Wr1TokenError((byte)errcode, declaringName);
}

static void ChkModuleLevel() {
    if (curScope != 0x100)
        Wr1TokenErrorAt(ERR73); /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
}

static void ChkNotArray() {
    if (dclFlags & F_ARRAY) {
        Wr1TokenErrorAt(ERR63);             /* INVALID DIMENSION WITH THIS ATTRIBUTE */
        dclFlags &= ~(F_ARRAY | F_STARDIM); // can't be any form of array
        arrayDim = 0;                       // dim not applicable
    }
}

static void ChkNotStarDim() {
    if (dclFlags & F_STARDIM) {
        Wr1TokenErrorAt(ERR62); /* INVALID STAR DIMENSION WITH 'STRUCTURE' OR 'EXTERNAL' */
        dclFlags &= ~F_STARDIM; // reset to single instance
        arrayDim = 1;
    }
}

static void CreateStructMemberInfo() {

    if (memberCnt == 0)
        return;
    index_t parent = infoIdx;
    for (word i = 1; i <= memberCnt; i++) {
        curSym = member[i].sym;
        CreateInfo(curScope, member[i].type, member[i].sym);
        Wr1XrefDef();
        info->flag |= F_MEMBER | F_LABEL;
        if (member[i].dim != 0) {
            info->flag |= F_ARRAY;
            info->dim = member[i].dim;
        } else
            info->dim = 1;
        info->parent = parent; // this is the infoP of the structure
    }
}

static void ChkAndSetAttributes(word factoredIdx) {
    word declaringBase = declBasedNames[factoredIdx];
    curSym = declaringName = declNames[factoredIdx];
    if (FindScopedInfo(curScope)) { // identifier already in scope  - only valid for parameter
        Wr1XrefUse();
        if ((info->flag & (F_PARAMETER | F_LABEL)) == F_PARAMETER) {
            if (dclFlags)
                DeclarationError(ERR76); /* CONFLICTING ATTRIBUTE WITH PARAMETER */
            if (dclType != BYTE_T && dclType != ADDRESS_T)
                DeclarationError(ERR79); /* ILLEGAL PARAMETER TYPE, not byte or address */
            else
                info->type = dclType;
            if (declaringBase) {
                DeclarationError(ERR77); /* INVALID PARAMETER DECLARATION, BASE ILLEGAL */
                declaringBase = 0;
            }
        } else {
            DeclarationError(ERR78); /* DUPLICATE DECLARATION */
            return;
        }
    } else {
        CreateInfo(curScope, dclType, curSym);
        Wr1XrefDef();
        info->flag = dclFlags;
    }
    if (dclType == LIT_T) {
        if (declaringBase != 0)      // lits cannot be based
            DeclarationError(ERR81); /* CONFLICTING ATTRIBUTE WITH 'BASE' */
        info->lit = lastLit;
        return;
    } else if (dclType == LABEL_T) {
        if (declaringBase)
            DeclarationError(ERR80); /* INVALID DECLARATION, LABEL MAY NOT BE BASED */
        if ((info->flag & F_EXTERNAL))
            info->flag |= F_LABEL;
        return;
    } else {
        if (declaringBase) {
            if ((info->flag & (F_PUBLIC | F_EXTERNAL | F_AT | F_INITIAL | F_DATA))) {
                DeclarationError(ERR81); /* CONFLICTING ATTRIBUTE WITH 'BASE' */
                declaringBase = 0;
            } else
                info->flag |= F_BASED;
        }
        info->dim     = arrayDim;
        info->baseOff = declaringBase;
        if ((infotab[procIdx].flag & F_REENTRANT)) {
            if (!(info->flag & (F_DATA | F_BASED | F_AT)))
                info->flag |= F_AUTOMATIC;
        }
    }
    if ((info->flag & F_PARAMETER))
        factoredParamCnt++;
    else if (factoredIdx - factoredParamCnt != 1) // isNewVar first non parameter var as packed
        info->flag |= F_PACKED;
    info->flag |= F_LABEL;
    if (dclType == STRUCT_T)
        CreateStructMemberInfo();
}

static void SetFactoredAttributes() {
    factoredParamCnt = 0;
    for (word i = 1; i <= declNameCnt; i++)
        ChkAndSetAttributes(i);
}

/*
    parse at, data or initial argument
*/
static void AttributeExpression(byte lexItem, uint32_t locflag) {
    if (dclFlags & F_EXTERNAL)
        Wr1TokenErrorAt(ERR41); /* CONFLICTING ATTRIBUTE */
    if (YylexMatch(T_LPAREN)) {
        Wr1Byte(lexItem);
        if (isNewVar)
            Wr1InfoOffset(infoCnt);
        else
            Wr1InfoOffset(0);
        dclFlags |= locflag;
        WrNestedExpression();
    } else
        Wr1TokenErrorAt(ERR75); /* MISSING ARGUMENT OF 'AT' , 'DATA' , OR 'INITIAL' */
}

static void ParseDclInitial() {
    if (YylexMatch(T_INITIAL)) {
        ChkModuleLevel();
        AttributeExpression(L_INITIAL, F_INITIAL);
    } else if (YylexMatch(T_DATA))
        AttributeExpression(L_DATA, F_DATA);
    else if (dclFlags & F_STARDIM) {
        Wr1TokenErrorAt(ERR74); /* INVALID STAR DIMENSION, not WITH 'data' or 'initial' */
        dclFlags &= ~F_STARDIM;
        arrayDim = 1;
    }
}

static void ParseDclAt() {
    if (YylexMatch(T_AT))
        AttributeExpression(L_AT, F_AT);
}

static void ParseDclScope() {
    if (YylexMatch(T_PUBLIC)) {
        ChkModuleLevel();
        dclFlags |= F_PUBLIC;
    } else if (YylexMatch(T_EXTERNAL)) {
        ChkNotStarDim();
        ChkModuleLevel();
        dclFlags |= F_EXTERNAL;
    }
}

static void ParseMemberType() {
    word type;

    if (YylexMatch(T_BYTE))
        type = BYTE_T;
    else if (YylexMatch(T_ADDRESS))
        type = ADDRESS_T;
    else {
        type = BYTE_T;
        if (YylexMatch(T_STRUCTURE)) {
            Wr1TokenErrorAt(ERR70); /* INVALID MEMBER TYPE, 'STRUCTURE' ILLEGAL */
            if (YylexMatch(T_LPAREN)) {
                RecoverToSemiOrRP();
                Yylex();
            }
        } else if (YylexMatch(T_LABEL))
            Wr1TokenErrorAt(ERR71); /* INVALID MEMBER TYPE, 'LABEL' ILLEGAL */
        else
            Wr1TokenErrorAt(ERR72); /* MISSING TYPE FOR STRUCTURE MEMBER */
    }
    member[memberCnt].type = (byte)type;
}

static void ParseMemberDim() {
    word dim;

    if (YylexMatch(T_LPAREN)) {
        if (YylexMatch(T_NUMBER))
            dim = tokenVal;
        else if (YylexMatch(T_STAR)) {
            dim = 1;
            Wr1TokenErrorAt(ERR69); /* INVALID STAR DIMENSION WITH STRUCTURE MEMBER */
        } else {
            dim = 1;
            Wr1TokenErrorAt(ERR59); /* ILLEGAL DIMENSION ATTRIBUTE */
        }
        if (dim == 0) {
            dim = 1;
            Wr1TokenErrorAt(ERR57); /* ILLEGAL DIMENSION ATTRIBUTE */
        }
        if (YylexNotMatch(T_RPAREN)) {
            Wr1TokenErrorAt(ERR60); /* MISSING ') ' AT END OF DIMENSION */
            RecoverToSemiOrRP();
            Yylex();
        }
        member[memberCnt].dim = dim;
    }
}

static void ParseMember() {

    if (YylexNotMatch(T_IDENTIFIER))
        Wr1TokenErrorAt(ERR66); /* INVALID STRUCTURE MEMBER, NOT AN IDENTIFIER */
    else {
        for (byte mcnt = 1; mcnt <= memberCnt; mcnt++) {
            if (curSym == member[mcnt].sym)
                Wr1TokenErrorAt(ERR67); /* DUPLICATE STRUCTURE MEMBER NAME */
        }
        if (memberCnt == 32)
            Wr1TokenErrorAt(ERR68); /* LIMIT EXCEEDED: NUMBER OF STRUCTURE MEMBERS */
        else
            memberCnt++;
        member[memberCnt].sym  = curSym;
        member[memberCnt].type = 0;
        member[memberCnt].dim  = 0;
        ParseMemberDim();
        ParseMemberType();
    }
}

static void ParseStructMem() {
    memberCnt = 0;
    if (YylexNotMatch(T_LPAREN))
        Wr1TokenErrorAt(ERR64); /* MISSING STRUCTURE MEMBERS */
    else {
        while (1) {
            ParseMember();
            if (YylexNotMatch(T_COMMA))
                break;
        }
        if (YylexNotMatch(T_RPAREN)) {
            Wr1TokenErrorAt(ERR65); /* MISSING ') ' AT END OF STRUCTURE MEMBER LIST */
            RecoverToSemiOrRP();
            Yylex();
        }
    }
}

static void ParseDclDataType() {
    if (YylexMatch(T_BYTE))
        dclType = BYTE_T;
    else if (YylexMatch(T_ADDRESS))
        dclType = ADDRESS_T;
    else if (YylexMatch(T_STRUCTURE)) {
        dclType = STRUCT_T;
        ChkNotStarDim(); // illegal for structure to have * dim
        ParseStructMem();
    } else if (YylexMatch(T_LABEL)) {
        dclType = LABEL_T;
        ChkNotArray();
    } else {
        Wr1TokenErrorAt(ERR61); /* MISSING TYPE */
        dclType = BYTE_T;       // assume byte
    }
}

static void ParseArraySize() {
    if (YylexMatch(T_LPAREN)) {
        dclFlags |= F_ARRAY;
        if (YylexMatch(T_NUMBER)) {
            if (tokenVal == 0) {
                Wr1TokenErrorAt(ERR57); /* INVALID DIMENSION, ZERO ILLEGAL */
                arrayDim = 1;
            } else
                arrayDim = tokenVal;
        } else if (YylexMatch(T_STAR)) {
            if (declNameCnt > 1) {
                Wr1TokenErrorAt(ERR58); /* INVALID STAR DIMENSION IN FACTORED DECLARATION */
                arrayDim = 1;
            } else
                dclFlags |= F_STARDIM;
        } else {
            Wr1TokenErrorAt(ERR59); /* ILLEGAL DIMENSION ATTRIBUTE */
            arrayDim = 1;
        }
        if (YylexNotMatch(T_RPAREN)) {
            Wr1TokenErrorAt(ERR60); /* MISSING ') ' AT END OF DIMENSION */
            RecoverToSemiOrRP();
            Yylex();
        }
    }
}

static void ParseDeclType() {
    ParseArraySize();
    ParseDclDataType();
    ParseDclScope();
    ParseDclAt();
    ParseDclInitial();
}

static void ParseLiterally() {
    if (YylexNotMatch(T_STRING)) {
        Wr1TokenErrorAt(ERR56); /* INVALID MACRO TEXT, NOT A STRING CONSTANT */
        wTokenLen   = 1;        // give default of a single space
        tokenStr[1] = ' ';
    }
    lastLit = CreateLit(wTokenLen, tokenStr + 1);
    dclType = LIT_T;
}

static void ParseLitOrType() {
    arrayDim = 1;
    dclFlags = 0;
    dclType  = LIT_T;
    if (YylexMatch(T_LITERALLY))
        ParseLiterally();
    else
        ParseDeclType();
}

static void ParseBaseSpecifier() {
    offset_t base1Name, base2Name;

    basedInfo = 0;
    if (YylexNotMatch(T_IDENTIFIER))
        Wr1TokenErrorAt(ERR52); /* INVALID BASE, MEMBER OF BASED STRUCTURE OR ARRAY OF STRUCTURES */
    else {
        base1Name = curSym;
        if (YylexMatch(T_PERIOD)) {
            if (YylexMatch(T_IDENTIFIER))
                base2Name = curSym;
            else {
                Wr1TokenErrorAt(ERR53); /* INVALID STRUCTURE MEMBER IN BASE */
                return;
            }
        } else
            base2Name = 0;
        curSym = base1Name;
        if (!FindInfo()) {
            Wr1TokenErrorAt(ERR54); /* UNDECLARED BASE */
            return;
        }
        Wr1XrefUse();
        if (base2Name == 0)
            basedInfo = infoIdx;
        else {
            curSym = base2Name;
            FindMemberInfo();
            if (infoIdx == 0) {
                Wr1TokenErrorAt(ERR55); /* UNDECLARED STRUTURE MEMBER IN BASE */
                return;
            }
            Wr1XrefUse();
            basedInfo = infoIdx;
        }
    }
} /* ParseBaseSpecifier() */

// parse <variable name specifier>
static void ParseVariableNameSpecifier() {
    if (YylexNotMatch(T_IDENTIFIER))
        Wr1TokenErrorAt(ERR48); /* ILLEGAL DECLARATION STATEMENT SYNTAX */
    else {
        if (declNameCnt == 32)
            Wr1TokenErrorAt(ERR49); /* LIMIT EXCEEDED: NUMBER OF ITEMS IN FACTORED DECLARE */
        else
            declNameCnt++;

        declNames[declNameCnt]      = curSym;
        declBasedNames[declNameCnt] = 0;
        if (!isNewVar && !FindScopedInfo(curScope)) // new var definition
            isNewVar = true;
        if (YylexMatch(T_BASED)) { // check for BASED variant
            ParseBaseSpecifier();
            if (basedInfo != 0) {
                SetInfo(basedInfo);
                // base var has to be basic address not a based var or array var
                if ((info->flag & (F_BASED | F_ARRAY)) || info->type != ADDRESS_T) {
                    Wr1TokenErrorAt(ERR50); /* INVALID ATTRIBUTES FOR BASE */
                    basedInfo = 0;
                } else if ((info->flag & F_MEMBER)) { // for structure var base, structure cannot be
                                                      // array or based
                    SetInfo(info->parent);
                    if ((info->flag & (F_ARRAY | F_BASED))) {
                        Wr1TokenErrorAt(ERR52); /* INVALID BASE, MEMBER OF BASED STRUCTURE OR ARRAY
                                                   OF STRUCTURES */
                        basedInfo = 0;
                    }
                }
                declBasedNames[declNameCnt] = basedInfo;
            }
        }
    }
}

void ParseDeclareNames() {
    declNameCnt = 0;
    isNewVar    = false;
    if (YylexMatch(T_LPAREN)) { // factored names list?
        while (1) {             // collect the variable names
            ParseVariableNameSpecifier();
            if (YylexNotMatch(T_COMMA))
                break;
        }
        if (YylexNotMatch(T_RPAREN)) {
            Wr1TokenErrorAt(ERR47); /* MISSING ') ' AT END OF FACTORED DECLARATION */
            RecoverToSemiOrRP();
            Yylex();
        }
    } else
        ParseVariableNameSpecifier();
}

// parse <declare element>
void ParseDeclareElementList() {
    if (stmtLabelCnt != 0)     // declare cannot have label prefix
        Wr1SyntaxError(ERR46); /* ILLEGAL USE OF LABEL */
    while (1) {
        ParseDeclareNames();
        if (declNameCnt != 0) {
            ParseLitOrType();
            SetFactoredAttributes();
        }
        if (YylexNotMatch(T_COMMA))
            return;
    }
}

static void SetPublic() {
    if (info->scope != 0x100)
        Wr1TokenErrorAt(ERR39); /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
    else if ((info->flag & F_PUBLIC))
        Wr1TokenErrorAt(ERR40); /* DUPLICATE ATTRIBUTE */
    else if ((info->flag & F_EXTERNAL))
        Wr1TokenErrorAt(ERR41); /* CONFLICTING ATTRIBUTE */
    else
        info->flag |= F_PUBLIC;
}

static void SetExternal() {
    if (info->scope != 0x100)
        Wr1TokenErrorAt(ERR39); /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
    else if ((info->flag & F_EXTERNAL))
        Wr1TokenErrorAt(ERR40); /* DUPLICATE ATTRIBUTE */
    else if ((info->flag & (F_REENTRANT | F_PUBLIC)))
        Wr1TokenErrorAt(ERR41); /* CONFLICTING ATTRIBUTE */
    else
        info->flag |= F_EXTERNAL;
}

static void SetReentrant() {
    if (info->scope != 0x100)
        Wr1TokenErrorAt(ERR39); /* INVALID ATTRIBUTE or INITIALIZATION, not at MODULE LEVEL */
    else if ((info->flag & F_REENTRANT))
        Wr1TokenErrorAt(ERR40); /* DUPLICATE ATTRIBUTE */
    else if ((info->flag & F_EXTERNAL))
        Wr1TokenErrorAt(ERR41); /* CONFLICTING ATTRIBUTE */
    else
        info->flag |= F_REENTRANT;
}

static void SetInterruptNo() {
    if (YylexMatch(T_NUMBER)) {
        if (tokenVal > 255) {
            Wr1TokenErrorAt(ERR42); /* INVALID interrupt VALUE */
            tokenVal = 0;
        }
    } else {
        Wr1TokenErrorAt(ERR43); /* MISSING interrupt VALUE */
        tokenVal = 0;
    }
    if (info->scope != 0x100)
        Wr1TokenErrorAt(ERR39); /* INVALID ATTRIBUTE or INITIALIZATION, not at MODULE LEVEL */
    else if ((info->flag & F_INTERRUPT))
        Wr1TokenErrorAt(ERR40); /* DUPLICATE ATTRIBUTE */
    else if (info->paramCnt)
        Wr1TokenErrorAt(ERR44); /* ILLEGAL ATTRIBUTE, 'interrupt' WITH PARAMETERS */
    else if (info->returnType)
        Wr1TokenErrorAt(ERR45); /* ILLEGAL ATTRIBUTE, 'interrupt' WITH TYPED procedure */
    else if ((info->flag & F_EXTERNAL))
        Wr1TokenErrorAt(ERR41); /* CONFLICTING ATTRIBUTE */
    else {
        info->flag |= F_INTERRUPT;
        info->intno = (byte)tokenVal;
    }
}

static void ParseProcAttrib() {
    SetInfo(procIdx);

    while (1) {
        if (YylexMatch(T_PUBLIC))
            SetPublic();
        else if (YylexMatch(T_EXTERNAL))
            SetExternal();
        else if (YylexMatch(T_REENTRANT))
            SetReentrant();
        else if (YylexMatch(T_INTERRUPT))
            SetInterruptNo();
        else
            return;
    }
}

static void ParseRetType() {
    SetInfo(procIdx);
    if (YylexMatch(T_BYTE))
        info->returnType = BYTE_T;
    else if (YylexMatch(T_ADDRESS))
        info->returnType = ADDRESS_T;
}

static void AddParam() {
    if (FindScopedInfo(curScope))
        Wr1TokenErrorAt(ERR38);           /* DUPLICATE PARAMETER NAME */
    CreateInfo(curScope, BYTE_T, curSym); // for now assume byte
    Wr1XrefDef();
    info->flag |= F_PARAMETER;
}

static void ParseParams() {
    byte paramCnt = 0;
    if (YylexMatch(T_LPAREN)) {

        while (1) {
            if (YylexMatch(T_IDENTIFIER)) {
                AddParam();
                paramCnt++;
            } else {
                Wr1TokenErrorAt(ERR36); /* MISSING PARAMETER */
                RecoverMissingParam();
            }
            if (YylexNotMatch(T_COMMA))
                break;
        }
        if (YylexNotMatch(T_RPAREN)) {
            Wr1TokenErrorAt(ERR37); /* MISSING ') ' AT END OF PARAMETER LIST */
            RecoverToSemiOrRP();
            Yylex();
        }
        SetInfo(procIdx); // note number of params. info's follow procInfo
    }
    info->paramCnt = paramCnt;
    ;
}

// parse <procedure statement> (label already parsed)

void ProcProcStmt() {
    word parentProcIdx;

    parentProcIdx = procIdx;
    curSym        = stmtLabels[1];
    if (FindScopedInfo(curScope))         // error if procedure already exists
        Wr1SyntaxError(ERR34);            /* DUPLICATE procedure DECLARATION */
    CreateInfo(curScope, PROC_T, curSym); // create a new procedure info block with current scope
    info->flag |= F_LABEL;
    Wr1XrefDef();
    if (procCnt == 254)            // oops too many procedures
        LexFatalError(ERR35);      /* LIMIT EXCEEDED: NUMBER OF PROCEDURES */
    procInfo[++procCnt] = infoIdx; // save procedure info
    curScope = procCnt << 8;       // set scope procedure id reset do block count for this procedure
    procIdx  = infoIdx;
    doBlkCnt = 0;
    PushBlock(curScope); // push current scope
    ParseParams();
    ParseRetType();
    ParseProcAttrib();
    /* write info to tx1 stream */
    SetInfo(procIdx);                 // accessors use curInfoP
    if (!(info->flag & F_EXTERNAL)) { // not external
        Wr1Byte(L_PROCEDURE);
        Wr1InfoOffset(infoIdx);
        Wr1Byte(L_SCOPE);
        Wr1Word(curScope);
    } else {
        Wr1Byte(L_EXTERNAL);
        Wr1InfoOffset(infoIdx);
    }
    info->procId = High(curScope);
    if (parentProcIdx && (infotab[parentProcIdx].flag & F_REENTRANT)) // were we in re-entrant proc?
        Wr1SyntaxError(ERR88); /* INVALID procedure NESTING, ILLEGAL IN reentrant procedure */
}
