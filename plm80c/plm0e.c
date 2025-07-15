/****************************************************************************
 *  plm0e.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"
#include <ctype.h>

static byte ENDorSEMICOLON[2] = { L_END, L_SEMICOLON };
static byte tokenTypeTable[]  = {
    L_NUMBER, L_NUMBER, L_NUMBER, L_IDENTIFIER, L_IDENTIFIER, L_PLUSSIGN, L_MINUSSIGN,
    L_STAR,   L_SLASH,  L_LPAREN, L_RPAREN,     L_COMMA,      L_COLON,    L_SEMICOLON,
    L_STRING, L_PERIOD, L_EQ,     L_LT,         L_GT,         0,          0,
    0,        0,        0,        L_IDENTIFIER
};

static void Token2Num() {
    byte ch;
    byte radix    = 10;

    byte tokenLen = tokenStr[0];
    if (!isdigit(ch = tokenStr[tokenLen])) {
        tokenLen--;
        if (ch == 'H')
            radix = 16;
        else if (ch == 'Q' || ch == 'O')
            radix = 8;
        else if (ch == 'B')
            radix = 2;
        else if (ch != 'D') {
            Wr1TokenErrorAt(ERR4); /* ILLEGAL NUMERIC CONSTANT TYPE */
            tokenVal = 0;
            return;
        }
    }
    int trial    = 0;
    bool errored = false;
    ;
    for (int i = 1; i <= tokenLen; i++) {
        ch = tokenStr[i];
        if (!isxdigit(ch) || (ch = isdigit(ch) ? ch - '0' : ch - 'A' + 10) >= radix) {
            if (!errored) {
                Wr1TokenErrorAt(ERR5); /* INVALID CHARACTER IN NUMERIC CONSTANT */
                errored = true;
            }
            ch = 0;
        }
        if ((trial = trial * radix + ch) > 65535) {
            Wr1TokenErrorAt(ERR94); /* ILLEGAL CONSTANT, VALUE > 65535 */
            trial = 0;
            break;
        }
    }
    tokenVal = (word)trial;
} /* Token2Num() */

static void NestMacro() {
    Wr1XrefUse();
    if (macroDepth == 10)
        Wr1TokenErrorAt(ERR7); /* LIMIT EXCEEDED: MACROS NESTED TOO DEEPLY */
    else {
        info->type = MACRO_T; // mark the type as  MACRO_T to spot recursive expansion
        macroPtrs[++macroDepth].text    = inChrP;    // push the current location
        macroPtrs[macroDepth].macroInfo = macroInfo; // and infoP
        inChrP    = (byte *)(info->lit->str) - 1;    // adjust for initial increment in GNxtCh()
        macroInfo = info;                            // set up the new infoP
    }
} /* NestMacro() */

static bool expandLit() {
    Lookup((pstr_t *)tokenStr);
    markedSym = curSym;
    if (curSym->infoChain >= infotab + MAXINFO) /* simple key word */
        tokenType = ToIdx(curSym->infoChain) - MAXINFO;
    else {
        info_t *savInfo = info; // if lit expand then restore info
        if (FindInfo()) {
            if (info->type == LIT_T) {
                NestMacro();
                info = savInfo;
                return true;
            } else if (info->type == MACRO_T) {
                Wr1TokenErrorAt(ERR6); /* ILLEGAL MACRO REFERENCE, RECURSIVE EXPANSION */
                info = savInfo;
                return true;
            }
        }
    }
    return false;
}

// updated version of GetName
// now allows _ in names, in numbers _ is ignored like $ is
static void GetName(word maxlen) {
    word curOff = 0;

    bool isNum  = isdigit(nextCh);
    while (isalnum(nextCh) || nextCh == '$' || nextCh == '_') {
        if (nextCh != '$' && (!isNum || nextCh != '_') && curOff++ < maxlen)
            tokenStr[curOff] = toupper(nextCh); // save name in upper case
        GNxtCh();
    }
    if (curOff > maxlen) {
        Wr1TokenErrorAt(ERR3); /* IDENTIFIER, STRING, or NUMBER TOO LONG, TRUNCATED */
        curOff = maxlen;
    }
    tokenStr[0] = (byte)curOff;
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
    wTokenLen   = (word)(curOff - 1); // record length long string
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
        case CC_DIGIT: // number
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
                tokenType = L_COLON_EQUALS;
                GNxtCh();
            }
            return;
        case CC_QUOTE:
            ParseString();
            return;
        case CC_LESS:
            GNxtCh();
            if (nextCh == '>') {
                tokenType = L_NE;
                GNxtCh();
            } else if (nextCh == '=') {
                tokenType = L_LE;
                GNxtCh();
            }
            return;
        case CC_GREATER:
            GNxtCh();
            if (nextCh == '=') {
                tokenType = L_GE;
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

void Yylex(void) {
    static bool endToggle = false; // relies on C's false = 0, true = 1

    if (yyAgain)
        yyAgain = false;
    else if (afterEOF) // return alternating END and ; to allow recovery
        tokenType = ENDorSEMICOLON[endToggle = !endToggle];
    else {
        LocYylex();
        if (tokenType == L_EOF) {
            afterEOF  = true;
            tokenType = L_END;
        }
    }
}

void SetYyAgain(void) {
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

// spool the nested expression
// note cannot contain CALL, DECLARE, DISABLE, DO, ENABLE, END, GO(TO), HALT, IF
// PROCEDURE or RETURN
static void WrNestedExpression() {
    word nesting = 1;
    Wr1LexToken();
    Yylex();

    while (1) {
        if (tokenType == L_LPAREN)
            nesting++;
        else if (tokenType == L_RPAREN) {
            if ((--nesting) == 0) {
                Wr1LexToken();  // write final token
                return;
            }
        } else if (tokenType == L_SEMICOLON || (L_CALL <= tokenType && tokenType <= L_RETURN)) {
            Wr1TokenErrorAt(ERR82); /* INVALID SYNTAX, MISMATCHED '(' */
            while (nesting--)       // fix missing ')'
                Wr1Byte(T1_RPAREN);
            SetYyAgain(); // rescan token
            return;
        }
        Wr1LexToken();  // spool the token
        Yylex();    // get another
    }
}

// convert and write Expression into lex tokens
// stop when endTok, semicolon or new start symbol for <declaration> or <unit> seen
void ParseExpresion(byte endTok) {
    Yylex();
    while (tokenType != endTok && tokenType != L_SEMICOLON) {
        if (L_CALL <= tokenType && tokenType <= L_RETURN)
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
        if (tokenType == L_SEMICOLON)
            break;
        if (tokenType == L_LPAREN)
            nesting++;
        else if (tokenType == L_RPAREN && nesting-- == 0)
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
    word nesting = 0;
    while (tokenType != L_RPAREN || nesting--) {
        if (tokenType == L_SEMICOLON || (tokenType == L_COMMA && !nesting))
            break;
        if (tokenType == L_LPAREN)
            nesting++;
        Yylex(); /* get next token */
    }
    SetYyAgain(); /* push back token */
}

static sym_t *declNames[MAXFACTORED];
static info_t *declBasedNames[MAXFACTORED];
static word declNameCnt;

static info_t *basedInfo;
static uint32_t dclAttr;
static byte dclType;
static pstr_t const *lastLit;
static word arrayDim;
static struct {
    sym_t *sym;
    word dim;
    byte type;
} member[MAXMEMBER];

static word memberCnt;
static bool isNewVar;

static void DeclarationError(word errcode) {
    Wr1TokenError((byte)errcode, curSym);
}

static void ChkModuleLevel() {
    if (curScope != 0x100)
        Wr1TokenErrorAt(ERR73); /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
}

static void ChkNotArray() {
    if (dclAttr & F_ARRAY) {
        Wr1TokenErrorAt(ERR63);            /* INVALID DIMENSION WITH THIS ATTRIBUTE */
        dclAttr &= ~(F_ARRAY | F_STARDIM); // can't be any form of array
        arrayDim = 0;                      // dim not applicable
    }
}

static void ChkNotStarDim() {
    if (dclAttr & F_STARDIM) {
        Wr1TokenErrorAt(ERR62); /* INVALID STAR DIMENSION WITH 'STRUCTURE' OR 'EXTERNAL' */
        dclAttr &= ~F_STARDIM;  // reset to single instance
        arrayDim = 1;
    }
}

static void CreateStructMemberInfo() {

    if (memberCnt == 0)
        return;
    info_t *structInfo = info;
    for (int i = 0; i < memberCnt; i++) {
        CreateInfo(curScope, member[i].type, member[i].sym);
        Wr1XrefDef();
        info->flag |= F_MEMBER | F_LABEL;
        if (member[i].dim != 0) {
            info->flag |= F_ARRAY;
            info->dim = member[i].dim;
        } else
            info->dim = 1;
        info->parent = structInfo;
    }
}

static void SetFactoredAttributes() {
    byte packed = 0;

    for (int i = 0; i < declNameCnt; i++) {
        info_t *declaringBase = declBasedNames[i];
        curSym                = declNames[i];

        if (FindScopedInfo(curScope)) { // identifier already in scope
            Wr1XrefUse();               // add use xref
            if ((info->flag & (F_PARAMETER | F_LABEL)) ==
                F_PARAMETER) {               // parameter not already declared
                if (dclAttr)                 // parameters cannot have attributes
                    DeclarationError(ERR76); /* CONFLICTING ATTRIBUTE WITH PARAMETER */
                if (dclType != BYTE_T && dclType != ADDRESS_T)
                    DeclarationError(ERR79); /* ILLEGAL PARAMETER TYPE, not byte or address */
                else
                    info->type = dclType;
                if (declaringBase) {         // parameters cannot be based
                    DeclarationError(ERR77); /* INVALID PARAMETER DECLARATION, BASE ILLEGAL */
                    declaringBase = NULL;
                }
            } else {
                DeclarationError(ERR78); /* DUPLICATE DECLARATION */
                continue;
            }
        } else {
            CreateInfo(curScope, dclType, curSym); // new symbol
            Wr1XrefDef();                          // include its xref
            info->flag = dclAttr;
        }

        if (dclType == LIT_T) {
            if (declaringBase)           // lits cannot be based
                DeclarationError(ERR81); /* CONFLICTING ATTRIBUTE WITH 'BASE' */
            info->lit = lastLit;
        } else if (dclType == LABEL_T) {
            if (declaringBase)
                DeclarationError(ERR80);   /* INVALID DECLARATION, LABEL MAY NOT BE BASED */
            if ((info->flag & F_EXTERNAL)) // if external then assume defined
                info->flag |= F_LABEL;
        } else {
            if (declaringBase) {
                if ((info->flag & (F_PUBLIC | F_EXTERNAL | F_AT | F_INITIAL | F_DATA))) {
                    DeclarationError(ERR81); /* CONFLICTING ATTRIBUTE WITH 'BASE' */
                    declaringBase = NULL;
                } else
                    info->flag |= F_BASED;
            }
            info->dim      = arrayDim;
            info->baseInfo = declaringBase;
            // mark any stack based vars
            if ((curProc->flag & F_REENTRANT) && !(info->flag & (F_DATA | F_BASED | F_AT)))
                info->flag |= F_AUTOMATIC;

            if (!(info->flag & F_PARAMETER) && packed++)
                info->flag |= F_PACKED;
            info->flag |= F_LABEL; // mark as declared
            if (dclType == STRUCT_T)
                CreateStructMemberInfo();
        }
    }
}

/*
    parse at, data or initial argument
*/
static void AttributeExpression(byte lexItem, uint32_t locflag) {
    if (dclAttr & F_EXTERNAL)
        Wr1TokenErrorAt(ERR41); /* CONFLICTING ATTRIBUTE */
    if (YylexMatch(L_LPAREN)) {
        Wr1Byte(lexItem);
        if (isNewVar)
            Wr1Info(topInfo);
        else
            Wr1Info(0);
        dclAttr |= locflag;
        WrNestedExpression();
    } else
        Wr1TokenErrorAt(ERR75); /* MISSING ARGUMENT OF 'AT' , 'DATA' , OR 'INITIAL' */
}

static void ParseDclInitial() {
    if (YylexMatch(L_INITIAL)) {
        ChkModuleLevel();
        AttributeExpression(T1_INITIAL, F_INITIAL);
    } else if (YylexMatch(L_DATA))
        AttributeExpression(T1_DATA, F_DATA);
    else if (dclAttr & F_STARDIM) {
        Wr1TokenErrorAt(ERR74); /* INVALID STAR DIMENSION, not WITH 'data' or 'initial' */
        dclAttr &= ~F_STARDIM;
        arrayDim = 1;
    }
}

static void ParseDclAt() {
    if (YylexMatch(L_AT))
        AttributeExpression(T1_AT, F_AT);
}

static void ParseDclScope() {
    if (YylexMatch(L_PUBLIC)) {
        ChkModuleLevel();
        dclAttr |= F_PUBLIC;
    } else if (YylexMatch(L_EXTERNAL)) {
        ChkNotStarDim();
        ChkModuleLevel();
        dclAttr |= F_EXTERNAL;
    }
}

static void ParseMemberType() {
    word type;

    if (YylexMatch(L_BYTE))
        type = BYTE_T;
    else if (YylexMatch(L_ADDRESS))
        type = ADDRESS_T;
    else {
        type = BYTE_T;
        if (YylexMatch(L_STRUCTURE)) {
            Wr1TokenErrorAt(ERR70); /* INVALID MEMBER TYPE, 'STRUCTURE' ILLEGAL */
            if (YylexMatch(L_LPAREN)) {
                RecoverToSemiOrRP();
                Yylex();
            }
        } else if (YylexMatch(L_LABEL))
            Wr1TokenErrorAt(ERR71); /* INVALID MEMBER TYPE, 'LABEL' ILLEGAL */
        else
            Wr1TokenErrorAt(ERR72); /* MISSING TYPE FOR STRUCTURE MEMBER */
    }
    member[memberCnt].type = (byte)type;
}

static void ParseMemberDim() {
    word dim;

    if (YylexMatch(L_LPAREN)) {
        if (YylexMatch(L_NUMBER))
            dim = tokenVal;
        else if (YylexMatch(L_STAR)) {
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
        if (YylexNotMatch(L_RPAREN)) {
            Wr1TokenErrorAt(ERR60); /* MISSING ') ' AT END OF DIMENSION */
            RecoverToSemiOrRP();
            Yylex();
        }
        member[memberCnt].dim = dim;
    }
}

static void ParseMember() {

    if (YylexNotMatch(L_IDENTIFIER))
        Wr1TokenErrorAt(ERR66); /* INVALID STRUCTURE MEMBER, NOT AN IDENTIFIER */
    else {
        for (int i = 0; i < memberCnt; i++) {
            if (curSym == member[i].sym)
                Wr1TokenErrorAt(ERR67); /* DUPLICATE STRUCTURE MEMBER NAME */
        }
        if (memberCnt == MAXMEMBER) {
            Wr1TokenErrorAt(ERR68); /* LIMIT EXCEEDED: NUMBER OF STRUCTURE MEMBERS */
            memberCnt--;            // overwrite last member
        }
        member[memberCnt].sym  = curSym;
        member[memberCnt].type = 0;
        member[memberCnt].dim  = 0;

        ParseMemberDim();
        ParseMemberType();
        memberCnt++;
    }
}

static void ParseStructMem() {
    memberCnt = 0;
    if (YylexNotMatch(L_LPAREN))
        Wr1TokenErrorAt(ERR64); /* MISSING STRUCTURE MEMBERS */
    else {
        while (1) {
            ParseMember();
            if (YylexNotMatch(L_COMMA))
                break;
        }
        if (YylexNotMatch(L_RPAREN)) {
            Wr1TokenErrorAt(ERR65); /* MISSING ') ' AT END OF STRUCTURE MEMBER LIST */
            RecoverToSemiOrRP();
            Yylex();
        }
    }
}

static void ParseDclDataType() {
    if (YylexMatch(L_BYTE))
        dclType = BYTE_T;
    else if (YylexMatch(L_ADDRESS))
        dclType = ADDRESS_T;
    else if (YylexMatch(L_STRUCTURE)) {
        dclType = STRUCT_T;
        ChkNotStarDim(); // illegal for structure to have * dim
        ParseStructMem();
    } else if (YylexMatch(L_LABEL)) {
        dclType = LABEL_T;
        ChkNotArray();
    } else {
        Wr1TokenErrorAt(ERR61); /* MISSING TYPE */
        dclType = BYTE_T;       // assume byte
    }
}

static void ParseArraySize() {
    if (YylexMatch(L_LPAREN)) {
        dclAttr |= F_ARRAY;
        if (YylexMatch(L_NUMBER)) {
            if (tokenVal == 0) {
                Wr1TokenErrorAt(ERR57); /* INVALID DIMENSION, ZERO ILLEGAL */
                arrayDim = 1;
            } else
                arrayDim = tokenVal;
        } else if (YylexMatch(L_STAR)) {
            if (declNameCnt > 1) {
                Wr1TokenErrorAt(ERR58); /* INVALID STAR DIMENSION IN FACTORED DECLARATION */
                arrayDim = 1;
            } else
                dclAttr |= F_STARDIM;
        } else {
            Wr1TokenErrorAt(ERR59); /* ILLEGAL DIMENSION ATTRIBUTE */
            arrayDim = 1;
        }
        if (YylexNotMatch(L_RPAREN)) {
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
    if (YylexNotMatch(L_STRING)) {
        Wr1TokenErrorAt(ERR56); /* INVALID MACRO TEXT, NOT A STRING CONSTANT */
        wTokenLen   = 1;        // give default of a single space
        tokenStr[1] = ' ';
    }
    lastLit = CreateLit(wTokenLen, (char *)tokenStr + 1);
    dclType = LIT_T;
}

static void ParseLitOrType() {
    arrayDim = 1;
    dclAttr  = 0;
    dclType  = LIT_T;
    if (YylexMatch(L_LITERALLY))
        ParseLiterally();
    else
        ParseDeclType();
}

static void ParseBaseSpecifier() {
    sym_t *base1Name, *base2Name;

    basedInfo = NULL;
    if (YylexNotMatch(L_IDENTIFIER))
        Wr1TokenErrorAt(ERR52); /* INVALID BASE, MEMBER OF BASED STRUCTURE OR ARRAY OF STRUCTURES */
    else {
        base1Name = curSym;
        if (YylexMatch(L_PERIOD)) {
            if (YylexMatch(L_IDENTIFIER))
                base2Name = curSym;
            else {
                Wr1TokenErrorAt(ERR53); /* INVALID STRUCTURE MEMBER IN BASE */
                return;
            }
        } else
            base2Name = NULL;
        curSym = base1Name;
        if (!FindInfo()) {
            Wr1TokenErrorAt(ERR54); /* UNDECLARED BASE */
            return;
        }
        Wr1XrefUse();
        if (base2Name == 0)
            basedInfo = info;
        else {
            curSym = base2Name;
            FindMemberInfo();
            if (!info) {
                Wr1TokenErrorAt(ERR55); /* UNDECLARED STRUTURE MEMBER IN BASE */
                return;
            }
            Wr1XrefUse();
            basedInfo = info;
        }
    }
} /* ParseBaseSpecifier() */

// parse <variable name specifier>
static void ParseVariableNameSpecifier() {
    if (YylexNotMatch(L_IDENTIFIER))
        Wr1TokenErrorAt(ERR48); /* ILLEGAL DECLARATION STATEMENT SYNTAX */
    else {
        if (declNameCnt == MAXFACTORED) {
            Wr1TokenErrorAt(ERR49); /* LIMIT EXCEEDED: NUMBER OF ITEMS IN FACTORED DECLARE */
            declNameCnt--;          // overwrite last one
        }

        declNames[declNameCnt]      = curSym;
        declBasedNames[declNameCnt] = NULL;
        if (!isNewVar && !FindScopedInfo(curScope)) // new var definition
            isNewVar = true;
        if (YylexMatch(L_BASED)) { // check for BASED variant
            ParseBaseSpecifier();
            if (basedInfo) {
                info = basedInfo;
                // base var has to be basic address not a based var or array var
                if ((info->flag & (F_BASED | F_ARRAY)) || info->type != ADDRESS_T) {
                    Wr1TokenErrorAt(ERR50); /* INVALID ATTRIBUTES FOR BASE */
                    basedInfo = NULL;
                } else if ((info->flag & F_MEMBER)) { // for structure var base, structure cannot be
                                                      // array or based
                    info = info->parent;
                    if ((info->flag & (F_ARRAY | F_BASED))) {
                        Wr1TokenErrorAt(ERR52); /* INVALID BASE, MEMBER OF BASED STRUCTURE OR ARRAY
                                                   OF STRUCTURES */
                        basedInfo = NULL;
                    }
                }
                declBasedNames[declNameCnt] = basedInfo;
            }
        }
        declNameCnt++;
    }
}

void ParseDeclareNames() {
    declNameCnt = 0;
    isNewVar    = false;
    if (YylexMatch(L_LPAREN)) { // factored names list?
        while (1) {             // collect the variable names
            ParseVariableNameSpecifier();
            if (YylexNotMatch(L_COMMA))
                break;
        }
        if (YylexNotMatch(L_RPAREN)) {
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
        if (declNameCnt) {
            ParseLitOrType();
            SetFactoredAttributes();
        }
        if (YylexNotMatch(L_COMMA))
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
    if (YylexMatch(L_NUMBER)) {
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
    info = curProc;

    while (1) {
        if (YylexMatch(L_PUBLIC))
            SetPublic();
        else if (YylexMatch(L_EXTERNAL))
            SetExternal();
        else if (YylexMatch(L_REENTRANT))
            SetReentrant();
        else if (YylexMatch(L_INTERRUPT))
            SetInterruptNo();
        else
            return;
    }
}

static void ParseRetType() {
    info = curProc;
    if (YylexMatch(L_BYTE))
        info->returnType = BYTE_T;
    else if (YylexMatch(L_ADDRESS))
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
    if (YylexMatch(L_LPAREN)) {

        while (1) {
            if (YylexMatch(L_IDENTIFIER)) {
                AddParam();
                paramCnt++;
            } else {
                Wr1TokenErrorAt(ERR36); /* MISSING PARAMETER */
                RecoverMissingParam();
            }
            if (YylexNotMatch(L_COMMA))
                break;
        }
        if (YylexNotMatch(L_RPAREN)) {
            Wr1TokenErrorAt(ERR37); /* MISSING ') ' AT END OF PARAMETER LIST */
            RecoverToSemiOrRP();
            Yylex();
        }
        info = curProc; // note number of params. info's follow procInfo
    }
    info->paramCnt = paramCnt;
}

// parse <procedure statement> (label already parsed)

void ParseProcStmt() {
    info_t *parentProc;

    parentProc = curProc;
    curSym     = stmtLabels[1];
    if (FindScopedInfo(curScope))         // error if procedure already exists
        Wr1SyntaxError(ERR34);            /* DUPLICATE procedure DECLARATION */
    CreateInfo(curScope, PROC_T, curSym); // create a new procedure info block with current scope
    info->flag |= F_LABEL;
    Wr1XrefDef();
    if (procCnt == 254)         // oops too many procedures
        Lexfatal(ERR35);   /* LIMIT EXCEEDED: NUMBER OF PROCEDURES */
    procInfo[++procCnt] = info; // save procedure info
    curScope = procCnt << 8;    // set scope procedure id reset do block count for this procedure
    curProc  = info;
    doBlkCnt = 0;
    PushBlock(curScope); // push current scope
    ParseParams();
    ParseRetType();
    ParseProcAttrib();
    /* write info to tx1 stream */
    info = curProc;
    if (!(info->flag & F_EXTERNAL)) { // not external
        Wr1Byte(T1_PROCEDURE);
        Wr1Info(info);
        Wr1Byte(T1_SCOPE);
        Wr1Word(curScope);
    } else {
        Wr1Byte(T1_EXTERNAL);
        Wr1Info(info);
    }
    info->procId = High(curScope);
    if (parentProc && (parentProc->flag & F_REENTRANT)) // were we in re-entrant proc?
        Wr1SyntaxError(ERR88); /* INVALID procedure NESTING, ILLEGAL IN reentrant procedure */
}
