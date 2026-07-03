/****************************************************************************
 *  plm0e.c: part of the C port of Intel's ISIS-II plm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>        *
 *                                                                          *
 *  This module handles lexical analysis, token processing, and            *
 *  declaration parsing for the PL/M-80 compiler.                          *
 *                                                                          *
 *  Main responsibilities:                                                  *
 *  - Tokenization and lexical analysis (Yylex)                            *
 *  - Macro expansion                                                       *
 *  - Expression parsing                                                    *
 *  - DECLARE statement processing                                          *
 *  - PROCEDURE statement processing                                        *
 *  - Symbol table management for declarations                              *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"
#include <ctype.h>

// Token alternatives for EOF recovery (alternates END and SEMICOLON)
static uint8_t ENDorSEMICOLON[2] = { L_END, L_SEMICOLON };

// Mapping from character class to token type
static uint8_t tokenTypeTable[]  = {
    L_NUMBER, L_NUMBER, L_NUMBER, L_IDENTIFIER, L_IDENTIFIER, L_PLUSSIGN, L_MINUSSIGN,
    L_STAR,   L_SLASH,  L_LPAREN, L_RPAREN,     L_COMMA,      L_COLON,    L_SEMICOLON,
    L_STRING, L_PERIOD, L_EQ,     L_LT,         L_GT,         0,          0,
    0,        0,        0,        L_IDENTIFIER
};

/****************************************************************************
 * Token2Num() - Convert token string to numeric value
 * 
 * Parses numeric constants in various bases:
 * - Binary (suffix B): base 2
 * - Octal (suffix O or Q): base 8
 * - Decimal (suffix D or no suffix): base 10
 * - Hexadecimal (suffix H): base 16
 * 
 * Validates digits are appropriate for the base and value <= 65535
 * 
 * Errors generated:
 * - ERR4: Illegal numeric constant type (invalid suffix)
 * - ERR5: Invalid character in numeric constant
 * - ERR94: Value exceeds 65535
 ****************************************************************************/
static void Token2Num() {
    uint8_t ch;
    uint8_t radix    = 10;                     // Default to decimal

    uint8_t tokenLen = tokenStr[0];
    
    // Check for base suffix (last character)
    if (!isdigit(ch = tokenStr[tokenLen])) {
        tokenLen--;
        if (ch == 'H')
            radix = 16;                        // Hexadecimal
        else if (ch == 'Q' || ch == 'O')
            radix = 8;                         // Octal
        else if (ch == 'B')
            radix = 2;                         // Binary
        else if (ch != 'D') {
            Wr1TokenErrorAt(ERR4);             /* ILLEGAL NUMERIC CONSTANT TYPE */
            tokenVal = 0;
            return;
        }
    }
    
    int trial    = 0;
    bool errored = false;
    
    // Convert digit by digit
    for (int i = 1; i <= tokenLen; i++) {
        ch = tokenStr[i];
        
        // Validate digit is valid for this radix
        if (!isxdigit(ch) || (ch = isdigit(ch) ? ch - '0' : ch - 'A' + 10) >= radix) {
            if (!errored) {
                Wr1TokenErrorAt(ERR5);         /* INVALID CHARACTER IN NUMERIC CONSTANT */
                errored = true;
            }
            ch = 0;                            // Treat invalid digit as 0
        }
        
        // Accumulate value and check overflow
        if ((trial = trial * radix + ch) > 65535) {
            Wr1TokenErrorAt(ERR94);            /* ILLEGAL CONSTANT, VALUE > 65535 */
            trial = 0;
            break;
        }
    }
    tokenVal = (uint16_t)trial;
}

/****************************************************************************
 * NestMacro() - Setup macro expansion
 * 
 * Pushes current parsing state and begins expanding a macro/literal.
 * Maintains a stack of macro expansions to handle nested macros.
 * 
 * Sets the macro info type to MACRO_T to detect recursive expansions.
 * Maximum nesting depth is 10 levels.
 ****************************************************************************/
static void NestMacro() {
    Wr1XrefUse();
    
    if (macroDepth == 10)
        Wr1TokenErrorAt(ERR7);                 /* LIMIT EXCEEDED: MACROS NESTED TOO DEEPLY */
    else {
        info->type = MACRO_T;                  // Mark as MACRO_T to detect recursive expansion
        macroPtrs[++macroDepth].text    = inChrP;        // Push current location
        macroPtrs[macroDepth].macroInfo = macroInfo;     // Push current macro info
        inChrP    = (uint8_t *)(info->lit->str) - 1;    // Point to macro text (-1 for GNxtCh increment)
        macroInfo = info;                      // Set up new macro info
    }
}

/****************************************************************************
 * expandLit() - Attempt literal/macro expansion
 * 
 * Looks up identifier in symbol table. If it's a LITERALLY definition,
 * expands it by pushing the literal text onto the macro expansion stack.
 * 
 * Returns: true if expansion occurred, false otherwise
 * 
 * Detects recursive macro expansion and reports error ERR6.
 ****************************************************************************/
static bool expandLit() {
    Lookup((pstr_t *)tokenStr);
    markedSym = curSym;
    
    // Check if it's a reserved word (infoChain >= MAXINFO)
    if (curSym->infoChain >= infotab + MAXINFO)
        tokenType = ToIdx(curSym->infoChain) - MAXINFO;
    else {
        info_t *savInfo = info;                // Save current info
        
        if (FindInfo()) {
            if (info->type == LIT_T) {         // Literal to expand
                NestMacro();
                info = savInfo;
                return true;
            } else if (info->type == MACRO_T) { // Recursive expansion detected
                Wr1TokenErrorAt(ERR6);         /* ILLEGAL MACRO REFERENCE, RECURSIVE EXPANSION */
                info = savInfo;
                return true;
            }
        }
    }
    return false;
}

/****************************************************************************
 * GetName() - Parse identifier or number token
 * 
 * Updated version that allows underscore (_) in identifiers.
 * In numbers, both $ and _ are ignored as separators.
 * 
 * Parameters:
 *   maxlen: Maximum token length allowed
 * 
 * Converts identifiers to uppercase and stores in tokenStr.
 * Generates ERR3 if token exceeds maximum length.
 ****************************************************************************/
static void GetName(uint16_t maxlen) {
    uint16_t curOff = 0;

    bool isNum  = isdigit(nextCh);
    
    // Collect alphanumeric characters, $ and _
    while (isalnum(nextCh) || nextCh == '$' || nextCh == '_') {
        // Skip $ always, skip _ in numbers, respect maxlen
        if (nextCh != '$' && (!isNum || nextCh != '_') && curOff++ < maxlen)
            tokenStr[curOff] = toupper(nextCh); // Store in uppercase
        GNxtCh();
    }
    
    // Check for length overflow
    if (curOff > maxlen) {
        Wr1TokenErrorAt(ERR3);                 /* IDENTIFIER, STRING, or NUMBER TOO LONG, TRUNCATED */
        curOff = maxlen;
    }
    tokenStr[0] = (uint8_t)curOff;             // Store length in first byte
}

/****************************************************************************
 * ParseString() - Parse string constant
 * 
 * Strings are delimited by single quotes (').
 * Double quotes ('') within a string produce a single quote in the result.
 * CR and LF characters are ignored.
 * 
 * Maximum string length is MAXSTRING (255 bytes for tokenStr[0]).
 * If string is too long and contains ';', assumes string ends there.
 * 
 * Errors generated:
 * - ERR189: Null string (empty string)
 * - ERR3: String too long, truncated
 * - ERR85: Long string assumed closed at semicolon
 ****************************************************************************/
static void ParseString() {
    bool tooLong = false;
    uint16_t curOff  = 1;

    while (1) {
        GNxtCh();
        
        // Skip CR/LF in strings
        if (nextCh == '\r' || nextCh == '\n')
            continue;
            
        // Handle quote character
        if (nextCh == QUOTE) {
            GNxtCh();
            if (nextCh != QUOTE)               // Single quote ends string
                break;
            // Double quote ('') becomes single quote in string
        }
        
        // Store character if room available
        if (curOff != MAXSTRING)
            tokenStr[curOff++] = nextCh;
        else {
            tooLong = true;
            // Special handling for long strings with semicolon
            if (nextCh == ';') {
                Wr1TokenErrorAt(ERR85);        /* LONG STRING ASSUMED CLOSED AT NEXT SEMICOLON OR QUOTE */
                break;
            }
        }
    }
    
    wTokenLen   = (uint16_t)(curOff - 1);      // Record actual length
    tokenStr[0] = wTokenLen < 256 ? (uint8_t)wTokenLen : 255;
    
    if (wTokenLen == 0)
        Wr1TokenErrorAt(ERR189);               /* NULL STRING NOT ALLOWED */
    if (tooLong)
        Wr1TokenErrorAt(ERR3);                 /* IDENTIFIER, STRING, or NUMBER TOO LONG, TRUNCATED */
}

/****************************************************************************
 * LocYylex() - Core lexical analyzer
 * 
 * Main tokenizer that:
 * - Skips whitespace
 * - Recognizes operators and punctuation
 * - Parses identifiers, numbers, and strings
 * - Handles comments (/* ... *\/)
 * - Expands LITERALLY definitions (macros)
 * - Detects multi-character operators (:=, <>, <=, >=)
 * 
 * Sets global variables:
 * - tokenType: Type of token found
 * - tokenStr: Token text (for identifiers/strings)
 * - tokenVal: Numeric value (for numbers)
 * 
 * Errors for invalid and unprintable characters.
 ****************************************************************************/
static void LocYylex() {
    while (1) {
        uint8_t chrClass = cClass[nextCh];     // Character class of lookahead
        tokenType     = tokenTypeTable[chrClass];
        
        switch (chrClass) {
        default:                                /* White space */
            do {
                GNxtCh();
            } while (cClass[nextCh] == CC_WSPACE);
            break;
            
        case CC_DIGIT:                          // Number
            GetName(31);                        // Max 31 chars for numbers
            Token2Num();                        // Convert to value
            return;
            
        case CC_HEXCHAR:                        // A-F (hex digit or identifier)
        case CC_ALPHA:                          // Letter
        case CC_UNDERBAR:                       // Underscore
            GetName(255);                       // Max 255 chars for identifiers
            if (!expandLit())                   // Try macro expansion
                return;
            GNxtCh();                           // If expanded, continue processing
            break;
            
        // Single-character tokens
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
            
        case CC_SLASH:                          // Division or comment start
            GNxtCh();
            if (nextCh != '*')                  // Not comment start
                return;
            // Skip comment
            GNxtCh();
            do {
                while (nextCh != '*')
                    GNxtCh();
                GNxtCh();
            } while (nextCh != '/');            // Found comment end
            GNxtCh();
            break;
            
        case CC_COLON:                          // Colon or assignment (:=)
            GNxtCh();
            if (nextCh == '=') {
                tokenType = L_COLON_EQUALS;
                GNxtCh();
            }
            return;
            
        case CC_QUOTE:                          // String constant
            ParseString();
            return;
            
        case CC_LESS:                           // < or <> or <=
            GNxtCh();
            if (nextCh == '>') {
                tokenType = L_NE;               // Not equal
                GNxtCh();
            } else if (nextCh == '=') {
                tokenType = L_LE;               // Less or equal
                GNxtCh();
            }
            return;
            
        case CC_GREATER:                        // > or >=
            GNxtCh();
            if (nextCh == '=') {
                tokenType = L_GE;               // Greater or equal
                GNxtCh();
            }
            return;
            
        case CC_DOLLAR:                         // Invalid in PL/M-80
        case CC_INVALID:
            Wr1TokenErrorAt(ERR1);             /* INVALID PL/M-80 CHARACTER */
            GNxtCh();
            break;
            
        case CC_NONPRINT:                       // Non-printable ASCII
            Wr1TokenErrorAt(ERR2);             /* UNPRINTABLE ASCII CHARACTER */
            GNxtCh();
            break;
        }
    }
}

/****************************************************************************
 * Yylex() - Main lexical analyzer entry point
 * 
 * Wrapper around LocYylex() that handles:
 * - Token push-back (yyAgain flag)
 * - EOF recovery (alternates END and SEMICOLON tokens after EOF)
 * 
 * After EOF, returns alternating L_END and L_SEMICOLON to allow
 * error recovery and proper termination.
 ****************************************************************************/
void Yylex(void) {
    static bool endToggle = false;             // Alternates between false/true

    if (yyAgain)
        yyAgain = false;                       // Use pushed-back token
    else if (afterEOF)                         // After EOF, alternate END/SEMICOLON
        tokenType = ENDorSEMICOLON[endToggle = !endToggle];
    else {
        LocYylex();
        if (tokenType == L_EOF) {
            afterEOF  = true;
            tokenType = L_END;                 // First EOF becomes END
        }
    }
}

/****************************************************************************
 * SetYyAgain() - Push back current token
 * 
 * Sets flag to reuse current token on next Yylex() call.
 * Used for lookahead when token doesn't match expected type.
 ****************************************************************************/
void SetYyAgain(void) {
    yyAgain = true;
}

/****************************************************************************
 * YylexMatch() - Try to match specific token type
 * 
 * Gets next token and checks if it matches expected type.
 * If not matched, pushes token back for re-scanning.
 * 
 * Parameters:
 *   token: Expected token type
 * Returns: true if matched, false otherwise
 ****************************************************************************/
bool YylexMatch(uint8_t token) {
    Yylex();
    if (tokenType == token)
        return true;
    else {
        SetYyAgain();                          // Push back for re-scan
        return false;
    }
}

/**
 * YylexNotMatch() - Negated version of YylexMatch
 */
bool YylexNotMatch(uint8_t token) {
    return !YylexMatch(token);
}

/****************************************************************************
 * WrNestedExpression() - Write expression within parentheses
 * 
 * Writes tokens from current position through matching right parenthesis.
 * Tracks nesting level to handle nested parentheses correctly.
 * 
 * Cannot contain statement keywords like CALL, DECLARE, DISABLE, DO,
 * ENABLE, END, GO(TO), HALT, IF, PROCEDURE or RETURN.
 * 
 * Generates ERR82 if statement keyword or semicolon encountered,
 * or if unmatched left parenthesis.
 ****************************************************************************/
static void WrNestedExpression() {
    uint16_t nesting = 1;
    Wr1LexToken();                             // Write opening token
    Yylex();

    while (1) {
        if (tokenType == L_LPAREN)
            nesting++;
        else if (tokenType == L_RPAREN) {
            if ((--nesting) == 0) {
                Wr1LexToken();                 // Write final ')'
                return;
            }
        } 
        // Detect invalid statement keywords in expression
        else if (tokenType == L_SEMICOLON || (L_CALL <= tokenType && tokenType <= L_RETURN)) {
            Wr1TokenErrorAt(ERR82);            /* INVALID SYNTAX, MISMATCHED '(' */
            while (nesting--)                  // Generate missing ')'
                Wr1Byte(T1_RPAREN);
            SetYyAgain();                      // Re-scan ending token
            return;
        }
        Wr1LexToken();                         // Write token
        Yylex();
    }
}

/****************************************************************************
 * ParseExpresion() - Parse and write expression
 * 
 * Converts expression tokens to internal representation until:
 * - endTok is encountered
 * - Semicolon is found
 * - Declaration or unit keyword is found
 * 
 * Parameters:
 *   endTok: Token type that terminates expression
 * 
 * Pushes back terminating token for re-scanning.
 ****************************************************************************/
void ParseExpresion(uint8_t endTok) {
    Yylex();
    while (tokenType != endTok && tokenType != L_SEMICOLON) {
        // Stop at statement keywords
        if (L_CALL <= tokenType && tokenType <= L_RETURN)
            break;
        Wr1LexToken();
        Yylex();
    }
    SetYyAgain();                              // Push back terminator
}

/****************************************************************************
 * RecoverToSemiOrRP() - Error recovery to semicolon or right paren
 * 
 * Skips tokens until:
 * - Semicolon found (unconditional)
 * - Right paren found at nesting level 0
 * 
 * Tracks parenthesis nesting to avoid stopping at ')' inside nested
 * expressions.
 * 
 * Pushes back terminating token.
 ****************************************************************************/
static void RecoverToSemiOrRP() {
    uint16_t nesting = 0;

    while (1) {
        if (tokenType == L_SEMICOLON)
            break;
        if (tokenType == L_LPAREN)
            nesting++;
        else if (tokenType == L_RPAREN && nesting-- == 0)
            break;
        Yylex();
    }
    SetYyAgain();
}

/****************************************************************************
 * RecoverMissingParam() - Error recovery in parameter list
 * 
 * Skips tokens until:
 * - Semicolon
 * - Comma at nesting level 0 (next parameter)
 * - Right paren at nesting level 0 (end of parameter list)
 * 
 * Used to recover from missing or malformed parameters.
 * Pushes back terminating token.
 ****************************************************************************/
static void RecoverMissingParam() {
    uint16_t nesting = 0;
    
    while (tokenType != L_RPAREN || nesting--) {
        if (tokenType == L_SEMICOLON || (tokenType == L_COMMA && !nesting))
            break;
        if (tokenType == L_LPAREN)
            nesting++;
        Yylex();
    }
    SetYyAgain();
}

/****************************************************************************
 * Declaration Processing Variables
 * 
 * These variables maintain state during DECLARE statement processing,
 * particularly for factored declarations like: DECLARE (A, B, C) BYTE;
 ****************************************************************************/
static sym_t *declNames[MAXFACTORED];          // Array of declared symbol names
static info_t *declBasedNames[MAXFACTORED];    // Array of base info for BASED vars
static uint16_t declNameCnt;                   // Count of names in factored declaration

static info_t *basedInfo;                      // Info for base variable in BASED clause
static uint32_t dclAttr;                       // Declaration attributes (F_PUBLIC, etc.)
static uint8_t dclType;                        // Declaration type (BYTE_T, ADDRESS_T, etc.)
static pstr_t const *lastLit;                  // Last literal for LITERALLY definitions
static uint16_t arrayDim;                      // Array dimension

// Structure member information
static struct {
    sym_t *sym;                                // Member symbol
    uint16_t dim;                              // Member dimension (0 if scalar)
    uint8_t type;                              // Member type (BYTE_T or ADDRESS_T)
} member[MAXMEMBER];

static uint16_t memberCnt;                     // Count of structure members
static bool isNewVar;                          // True if declaring new variable

/**
 * DeclarationError() - Report declaration-related error
 */
static void DeclarationError(uint16_t errcode) {
    Wr1TokenError((uint8_t)errcode, curSym);
}

/**
 * ChkModuleLevel() - Verify declaration is at module level
 * 
 * Certain attributes (PUBLIC, EXTERNAL, INITIAL) are only valid
 * at module level (scope 0x100).
 */
static void ChkModuleLevel() {
    if (curScope != 0x100)
        Wr1TokenErrorAt(ERR73);                /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
}

/**
 * ChkNotArray() - Verify declaration is not an array
 * 
 * Labels cannot be arrays. Removes array attributes if present.
 */
static void ChkNotArray() {
    if (dclAttr & F_ARRAY) {
        Wr1TokenErrorAt(ERR63);                /* INVALID DIMENSION WITH THIS ATTRIBUTE */
        dclAttr &= ~(F_ARRAY | F_STARDIM);     // Remove array flags
        arrayDim = 0;
    }
}

/**
 * ChkNotStarDim() - Verify declaration doesn't use star dimension
 * 
 * Star dimension (*) is invalid for STRUCTURE and EXTERNAL.
 * Converts to single instance if found.
 */
static void ChkNotStarDim() {
    if (dclAttr & F_STARDIM) {
        Wr1TokenErrorAt(ERR62);                /* INVALID STAR DIMENSION WITH 'STRUCTURE' OR 'EXTERNAL' */
        dclAttr &= ~F_STARDIM;
        arrayDim = 1;
    }
}

/****************************************************************************
 * CreateStructMemberInfo() - Create info entries for structure members
 * 
 * For each member in the member[] array, creates an info entry with:
 * - F_MEMBER flag set
 * - F_LABEL flag set (declared)
 * - F_ARRAY if dimensioned
 * - Parent pointer to structure info
 ****************************************************************************/
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

/****************************************************************************
 * SetFactoredAttributes() - Apply attributes to factored declarations
 * 
 * Processes each variable in a factored declaration (e.g., DECLARE (A,B,C) BYTE)
 * and applies the parsed attributes (type, dimension, scope, etc.).
 * 
 * Handles:
 * - Parameter redeclaration (from procedure parameter list)
 * - Duplicate declaration detection
 * - Based variable setup
 * - Array dimensions
 * - Automatic variables (in REENTRANT procedures)
 * - Structure member creation
 * - Packed attribute for multiple scalar variables
 * 
 * Validates parameter consistency and conflicting attributes.
 ****************************************************************************/
static void SetFactoredAttributes() {
    uint8_t packed = 0;

    for (int i = 0; i < declNameCnt; i++) {
        info_t *declaringBase = declBasedNames[i];
        curSym                = declNames[i];

        // Check if already declared in current scope
        if (FindScopedInfo(curScope)) {
            Wr1XrefUse();
            
            // Allow parameter redeclaration with type
            if ((info->flag & (F_PARAMETER | F_LABEL)) == F_PARAMETER) {
                if (dclAttr)                   // Parameters cannot have attributes
                    DeclarationError(ERR76);   /* CONFLICTING ATTRIBUTE WITH PARAMETER */
                    
                if (dclType != BYTE_T && dclType != ADDRESS_T)
                    DeclarationError(ERR79);   /* ILLEGAL PARAMETER TYPE */
                else
                    info->type = dclType;
                    
                if (declaringBase) {           // Parameters cannot be BASED
                    DeclarationError(ERR77);   /* INVALID PARAMETER DECLARATION, BASE ILLEGAL */
                    declaringBase = NULL;
                }
            } else {
                DeclarationError(ERR78);       /* DUPLICATE DECLARATION */
                continue;
            }
        } else {
            // New symbol - create info entry
            CreateInfo(curScope, dclType, curSym);
            Wr1XrefDef();
            info->flag = dclAttr;
        }

        // Handle type-specific attributes
        if (dclType == LIT_T) {
            if (declaringBase)                 // LITERALLY cannot be BASED
                DeclarationError(ERR81);       /* CONFLICTING ATTRIBUTE WITH 'BASE' */
            info->lit = lastLit;
        } else if (dclType == LABEL_T) {
            if (declaringBase)
                DeclarationError(ERR80);       /* INVALID DECLARATION, LABEL MAY NOT BE BASED */
            if ((info->flag & F_EXTERNAL))     // EXTERNAL labels are defined
                info->flag |= F_LABEL;
        } else {
            // Handle BASED variables
            if (declaringBase) {
                if ((info->flag & (F_PUBLIC | F_EXTERNAL | F_AT | F_INITIAL | F_DATA))) {
                    DeclarationError(ERR81);   /* CONFLICTING ATTRIBUTE WITH 'BASE' */
                    declaringBase = NULL;
                } else
                    info->flag |= F_BASED;
            }
            
            info->dim      = arrayDim;
            info->baseInfo = declaringBase;
            
            // Mark automatic variables (stack-based in REENTRANT)
            if ((curProc->flag & F_REENTRANT) && !(info->flag & (F_DATA | F_BASED | F_AT)))
                info->flag |= F_AUTOMATIC;

            // Set packed flag for non-first non-parameter scalars
            if (!(info->flag & F_PARAMETER) && packed++)
                info->flag |= F_PACKED;
                
            info->flag |= F_LABEL;             // Mark as declared
            
            // Create structure member info entries
            if (dclType == STRUCT_T)
                CreateStructMemberInfo();
        }
    }
}

/****************************************************************************
 * AttributeExpression() - Parse AT, DATA, or INITIAL attribute
 * 
 * Syntax: AT(expr) or DATA(expr) or INITIAL(expr)
 * 
 * Parameters:
 *   lexItem: Token to write (T1_AT, T1_DATA, or T1_INITIAL)
 *   locflag: Flag to set (F_AT, F_DATA, or F_INITIAL)
 * 
 * Writes the attribute and nested expression to intermediate file.
 * Cannot be used with EXTERNAL attribute.
 ****************************************************************************/
static void AttributeExpression(uint8_t lexItem, uint32_t locflag) {
    if (dclAttr & F_EXTERNAL)
        Wr1TokenErrorAt(ERR41);                /* CONFLICTING ATTRIBUTE */
        
    if (YylexMatch(L_LPAREN)) {
        Wr1Byte(lexItem);
        
        if (isNewVar)
            Wr1Info(topInfo);                  // Write info reference
        else
            Wr1Info(0);
            
        dclAttr |= locflag;
        WrNestedExpression();                  // Write (expr)
    } else
        Wr1TokenErrorAt(ERR75);                /* MISSING ARGUMENT OF 'AT' , 'DATA' , OR 'INITIAL' */
}

/**
 * ParseDclInitial() - Parse INITIAL or DATA attribute
 * 
 * INITIAL is only valid at module level.
 * If star dimension (*) used, DATA or INITIAL is required.
 */
static void ParseDclInitial() {
    if (YylexMatch(L_INITIAL)) {
        ChkModuleLevel();
        AttributeExpression(T1_INITIAL, F_INITIAL);
    } else if (YylexMatch(L_DATA))
        AttributeExpression(T1_DATA, F_DATA);
    else if (dclAttr & F_STARDIM) {
        Wr1TokenErrorAt(ERR74);                /* INVALID STAR DIMENSION, not WITH 'data' or 'initial' */
        dclAttr &= ~F_STARDIM;
        arrayDim = 1;
    }
}

/**
 * ParseDclAt() - Parse AT attribute
 * 
 * Syntax: AT(address_expr)
 * Used to place variable at specific memory address.
 */
static void ParseDclAt() {
    if (YylexMatch(L_AT))
        AttributeExpression(T1_AT, F_AT);
}

/**
 * ParseDclScope() - Parse PUBLIC or EXTERNAL attribute
 * 
 * Both are only valid at module level.
 * EXTERNAL cannot have star dimension.
 */
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

/**
 * ParseMemberType() - Parse structure member type
 * 
 * Valid types: BYTE or ADDRESS
 * Invalid: STRUCTURE (generates ERR70) or LABEL (generates ERR71)
 */
static void ParseMemberType() {
    uint16_t type;

    if (YylexMatch(L_BYTE))
        type = BYTE_T;
    else if (YylexMatch(L_ADDRESS))
        type = ADDRESS_T;
    else {
        type = BYTE_T;                         // Default to BYTE
        
        if (YylexMatch(L_STRUCTURE)) {
            Wr1TokenErrorAt(ERR70);            /* INVALID MEMBER TYPE, 'STRUCTURE' ILLEGAL */
            if (YylexMatch(L_LPAREN)) {
                RecoverToSemiOrRP();
                Yylex();
            }
        } else if (YylexMatch(L_LABEL))
            Wr1TokenErrorAt(ERR71);            /* INVALID MEMBER TYPE, 'LABEL' ILLEGAL */
        else
            Wr1TokenErrorAt(ERR72);            /* MISSING TYPE FOR STRUCTURE MEMBER */
    }
    member[memberCnt].type = (uint8_t)type;
}

/**
 * ParseMemberDim() - Parse structure member dimension
 * 
 * Syntax: (number)
 * Star dimension (*) not allowed for structure members (generates ERR69).
 * Zero dimension generates ERR57.
 */
static void ParseMemberDim() {
    uint16_t dim;

    if (YylexMatch(L_LPAREN)) {
        if (YylexMatch(L_NUMBER))
            dim = tokenVal;
        else if (YylexMatch(L_STAR)) {
            dim = 1;
            Wr1TokenErrorAt(ERR69);            /* INVALID STAR DIMENSION WITH STRUCTURE MEMBER */
        } else {
            dim = 1;
            Wr1TokenErrorAt(ERR59);            /* ILLEGAL DIMENSION ATTRIBUTE */
        }
        
        if (dim == 0) {
            dim = 1;
            Wr1TokenErrorAt(ERR57);            /* ILLEGAL DIMENSION ATTRIBUTE */
        }
        
        if (YylexNotMatch(L_RPAREN)) {
            Wr1TokenErrorAt(ERR60);            /* MISSING ')' AT END OF DIMENSION */
            RecoverToSemiOrRP();
            Yylex();
        }
        member[memberCnt].dim = dim;
    }
}

/**
 * ParseMember() - Parse single structure member
 * 
 * Syntax: identifier [(dimension)] type
 * Checks for duplicate member names within structure.
 */
static void ParseMember() {
    if (YylexNotMatch(L_IDENTIFIER))
        Wr1TokenErrorAt(ERR66);                /* INVALID STRUCTURE MEMBER, NOT AN IDENTIFIER */
    else {
        // Check for duplicate member name
        for (int i = 0; i < memberCnt; i++) {
            if (curSym == member[i].sym)
                Wr1TokenErrorAt(ERR67);        /* DUPLICATE STRUCTURE MEMBER NAME */
        }
        
        if (memberCnt == MAXMEMBER) {
            Wr1TokenErrorAt(ERR68);            /* LIMIT EXCEEDED: NUMBER OF STRUCTURE MEMBERS */
            memberCnt--;                       // Overwrite last member
        }
        
        member[memberCnt].sym  = curSym;
        member[memberCnt].type = 0;
        member[memberCnt].dim  = 0;

        ParseMemberDim();
        ParseMemberType();
        memberCnt++;
    }
}

/**
 * ParseStructMem() - Parse structure member list
 * 
 * Syntax: (member1, member2, ...)
 */
static void ParseStructMem() {
    memberCnt = 0;
    
    if (YylexNotMatch(L_LPAREN))
        Wr1TokenErrorAt(ERR64);                /* MISSING STRUCTURE MEMBERS */
    else {
        while (1) {
            ParseMember();
            if (YylexNotMatch(L_COMMA))
                break;
        }
        
        if (YylexNotMatch(L_RPAREN)) {
            Wr1TokenErrorAt(ERR65);            /* MISSING ') ' AT END OF STRUCTURE MEMBER LIST */
            RecoverToSemiOrRP();
            Yylex();
        }
    }
}

/**
 * ParseDclDataType() - Parse variable data type
 * 
 * Valid types: BYTE, ADDRESS, STRUCTURE, LABEL
 * LABEL cannot be array. STRUCTURE cannot have star dimension.
 */
static void ParseDclDataType() {
    if (YylexMatch(L_BYTE))
        dclType = BYTE_T;
    else if (YylexMatch(L_ADDRESS))
        dclType = ADDRESS_T;
    else if (YylexMatch(L_STRUCTURE)) {
        dclType = STRUCT_T;
        ChkNotStarDim();                       // STRUCTURE cannot use (*)
        ParseStructMem();
    } else if (YylexMatch(L_LABEL)) {
        dclType = LABEL_T;
        ChkNotArray();                         // LABEL cannot be array
    } else {
        Wr1TokenErrorAt(ERR61);                /* MISSING TYPE */
        dclType = BYTE_T;                      // Default to BYTE
    }
}

/**
 * ParseArraySize() - Parse array dimension specifier
 * 
 * Syntax: (number) or (*)
 * Star dimension (*) not allowed in factored declarations.
 */
static void ParseArraySize() {
    if (YylexMatch(L_LPAREN)) {
        dclAttr |= F_ARRAY;
        
        if (YylexMatch(L_NUMBER)) {
            if (tokenVal == 0) {
                Wr1TokenErrorAt(ERR57);        /* INVALID DIMENSION, ZERO ILLEGAL */
                arrayDim = 1;
            } else
                arrayDim = tokenVal;
        } else if (YylexMatch(L_STAR)) {
            if (declNameCnt > 1) {
                Wr1TokenErrorAt(ERR58);        /* INVALID STAR DIMENSION IN FACTORED DECLARATION */
                arrayDim = 1;
            } else
                dclAttr |= F_STARDIM;
        } else {
            Wr1TokenErrorAt(ERR59);            /* ILLEGAL DIMENSION ATTRIBUTE */
            arrayDim = 1;
        }
        
        if (YylexNotMatch(L_RPAREN)) {
            Wr1TokenErrorAt(ERR60);            /* MISSING ') ' AT END OF DIMENSION */
            RecoverToSemiOrRP();
            Yylex();
        }
    }
}

/**
 * ParseDeclType() - Parse complete type specification
 * 
 * Parses: [dimension] type [scope] [AT] [INITIAL|DATA]
 */
static void ParseDeclType() {
    ParseArraySize();
    ParseDclDataType();
    ParseDclScope();
    ParseDclAt();
    ParseDclInitial();
}

/**
 * ParseLiterally() - Parse LITERALLY definition
 * 
 * Syntax: identifier LITERALLY 'string'
 * Creates a literal (macro) that will be expanded during compilation.
 */
static void ParseLiterally() {
    if (YylexNotMatch(L_STRING)) {
        Wr1TokenErrorAt(ERR56);                /* INVALID MACRO TEXT, NOT A STRING CONSTANT */
        wTokenLen   = 1;
        tokenStr[1] = ' ';                     // Default to single space
    }
    lastLit = CreateLit(wTokenLen, (char *)tokenStr + 1);
    dclType = LIT_T;
}

/**
 * ParseLitOrType() - Parse LITERALLY or type specification
 * 
 * Handles both LITERALLY definitions and normal variable declarations.
 */
static void ParseLitOrType() {
    arrayDim = 1;
    dclAttr  = 0;
    dclType  = LIT_T;
    
    if (YylexMatch(L_LITERALLY))
        ParseLiterally();
    else
        ParseDeclType();
}

/****************************************************************************
 * ParseBaseSpecifier() - Parse BASED clause base specifier
 * 
 * Syntax: identifier or identifier.member
 * 
 * Validates:
 * - Base variable must be ADDRESS type
 * - Base cannot be BASED or ARRAY
 * - If member reference, structure cannot be BASED or ARRAY
 * 
 * Sets basedInfo to the base variable/member info, or NULL if invalid.
 ****************************************************************************/
static void ParseBaseSpecifier() {
    sym_t *base1Name, *base2Name;

    basedInfo = NULL;
    
    if (YylexNotMatch(L_IDENTIFIER))
        Wr1TokenErrorAt(ERR52);                /* INVALID BASE */
    else {
        base1Name = curSym;
        
        // Check for structure member reference (base.member)
        if (YylexMatch(L_PERIOD)) {
            if (YylexMatch(L_IDENTIFIER))
                base2Name = curSym;
            else {
                Wr1TokenErrorAt(ERR53);        /* INVALID STRUCTURE MEMBER IN BASE */
                return;
            }
        } else
            base2Name = NULL;
            
        // Look up base variable
        curSym = base1Name;
        if (!FindInfo()) {
            Wr1TokenErrorAt(ERR54);            /* UNDECLARED BASE */
            return;
        }
        Wr1XrefUse();
        
        if (base2Name == 0)
            basedInfo = info;
        else {
            // Look up structure member
            curSym = base2Name;
            FindMemberInfo();
            if (!info) {
                Wr1TokenErrorAt(ERR55);        /* UNDECLARED STRUTURE MEMBER IN BASE */
                return;
            }
            Wr1XrefUse();
            basedInfo = info;
        }
    }
}

/****************************************************************************
 * ParseVariableNameSpecifier() - Parse variable name with optional BASED
 * 
 * Syntax: identifier [BASED basespec]
 * 
 * Handles:
 * - New variable detection
 * - BASED clause parsing and validation
 * - Factored declaration name collection
 ****************************************************************************/
static void ParseVariableNameSpecifier() {
    if (YylexNotMatch(L_IDENTIFIER))
        Wr1TokenErrorAt(ERR48);                /* ILLEGAL DECLARATION STATEMENT SYNTAX */
    else {
        if (declNameCnt == MAXFACTORED) {
            Wr1TokenErrorAt(ERR49);            /* LIMIT EXCEEDED: NUMBER OF ITEMS IN FACTORED DECLARE */
            declNameCnt--;
        }

        declNames[declNameCnt]      = curSym;
        declBasedNames[declNameCnt] = NULL;
        
        // Check if this is a new variable
        if (!isNewVar && !FindScopedInfo(curScope))
            isNewVar = true;
            
        // Handle BASED clause
        if (YylexMatch(L_BASED)) {
            ParseBaseSpecifier();
            
            if (basedInfo) {
                info = basedInfo;
                // Validate base is ADDRESS, not BASED or ARRAY
                if ((info->flag & (F_BASED | F_ARRAY)) || info->type != ADDRESS_T) {
                    Wr1TokenErrorAt(ERR50);    /* INVALID ATTRIBUTES FOR BASE */
                    basedInfo = NULL;
                } else if ((info->flag & F_MEMBER)) {
                    // For structure member base, structure cannot be BASED/ARRAY
                    info = info->parent;
                    if ((info->flag & (F_ARRAY | F_BASED))) {
                        Wr1TokenErrorAt(ERR52); /* INVALID BASE */
                        basedInfo = NULL;
                    }
                }
                declBasedNames[declNameCnt] = basedInfo;
            }
        }
        declNameCnt++;
    }
}

/**
 * ParseDeclareNames() - Parse variable name list
 * 
 * Syntax: name or (name1, name2, ...)
 * Collects names for factored declaration.
 */
void ParseDeclareNames() {
    declNameCnt = 0;
    isNewVar    = false;
    
    if (YylexMatch(L_LPAREN)) {                // Factored declaration
        while (1) {
            ParseVariableNameSpecifier();
            if (YylexNotMatch(L_COMMA))
                break;
        }
        
        if (YylexNotMatch(L_RPAREN)) {
            Wr1TokenErrorAt(ERR47);            /* MISSING ')' AT END OF FACTORED DECLARATION */
            RecoverToSemiOrRP();
            Yylex();
        }
    } else
        ParseVariableNameSpecifier();
}

/****************************************************************************
 * ParseDeclareElementList() - Parse DECLARE statement element list
 * 
 * Syntax: DECLARE name type [, name type]... ;
 * 
 * Processes comma-separated declarations, each with its own type/attributes.
 * Labels are not allowed on DECLARE statements.
 ****************************************************************************/
void ParseDeclareElementList() {
    if (stmtLabelCnt != 0)                     // DECLARE cannot have labels
        Wr1SyntaxError(ERR46);                 /* ILLEGAL USE OF LABEL */
        
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

/****************************************************************************
 * Procedure Attribute Setters
 * 
 * These functions set procedure-level attributes and validate:
 * - Attributes are at module level (scope 0x100)
 * - No duplicate attributes
 * - No conflicting attributes
 ****************************************************************************/

/**
 * SetPublic() - Set PUBLIC attribute on procedure
 */
static void SetPublic() {
    if (info->scope != 0x100)
        Wr1TokenErrorAt(ERR39);                /* INVALID ATTRIBUTE, NOT AT MODULE LEVEL */
    else if ((info->flag & F_PUBLIC))
        Wr1TokenErrorAt(ERR40);                /* DUPLICATE ATTRIBUTE */
    else if ((info->flag & F_EXTERNAL))
        Wr1TokenErrorAt(ERR41);                /* CONFLICTING ATTRIBUTE */
    else
        info->flag |= F_PUBLIC;
}

/**
 * SetExternal() - Set EXTERNAL attribute on procedure
 */
static void SetExternal() {
    if (info->scope != 0x100)
        Wr1TokenErrorAt(ERR39);
    else if ((info->flag & F_EXTERNAL))
        Wr1TokenErrorAt(ERR40);                /* DUPLICATE ATTRIBUTE */
    else if ((info->flag & (F_REENTRANT | F_PUBLIC)))
        Wr1TokenErrorAt(ERR41);                /* CONFLICTING ATTRIBUTE */
    else
        info->flag |= F_EXTERNAL;
}

/**
 * SetReentrant() - Set REENTRANT attribute on procedure
 */
static void SetReentrant() {
    if (info->scope != 0x100)
        Wr1TokenErrorAt(ERR39);
    else if ((info->flag & F_REENTRANT))
        Wr1TokenErrorAt(ERR40);                /* DUPLICATE ATTRIBUTE */
    else if ((info->flag & F_EXTERNAL))
        Wr1TokenErrorAt(ERR41);                /* CONFLICTING ATTRIBUTE */
    else
        info->flag |= F_REENTRANT;
}

/**
 * SetInterruptNo() - Set INTERRUPT attribute with interrupt number
 * 
 * Validates:
 * - Interrupt number 0-255
 * - Procedure has no parameters
 * - Procedure has no return type
 */
static void SetInterruptNo() {
    if (YylexMatch(L_NUMBER)) {
        if (tokenVal > 255) {
            Wr1TokenErrorAt(ERR42);            /* INVALID interrupt VALUE */
            tokenVal = 0;
        }
    } else {
        Wr1TokenErrorAt(ERR43);                /* MISSING interrupt VALUE */
        tokenVal = 0;
    }
    
    if (info->scope != 0x100)
        Wr1TokenErrorAt(ERR39);
    else if ((info->flag & F_INTERRUPT))
        Wr1TokenErrorAt(ERR40);                /* DUPLICATE ATTRIBUTE */
    else if (info->paramCnt)
        Wr1TokenErrorAt(ERR44);                /* ILLEGAL ATTRIBUTE, 'interrupt' WITH PARAMETERS */
    else if (info->returnType)
        Wr1TokenErrorAt(ERR45);                /* ILLEGAL ATTRIBUTE, 'interrupt' WITH TYPED procedure */
    else if ((info->flag & F_EXTERNAL))
        Wr1TokenErrorAt(ERR41);                /* CONFLICTING ATTRIBUTE */
    else {
        info->flag |= F_INTERRUPT;
        info->intno = (uint8_t)tokenVal;
    }
}

/**
 * ParseProcAttrib() - Parse procedure attribute list
 * 
 * Valid attributes: PUBLIC, EXTERNAL, REENTRANT, INTERRUPT
 */
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

/**
 * ParseRetType() - Parse procedure return type
 * 
 * Valid return types: BYTE, ADDRESS
 */
static void ParseRetType() {
    info = curProc;
    
    if (YylexMatch(L_BYTE))
        info->returnType = BYTE_T;
    else if (YylexMatch(L_ADDRESS))
        info->returnType = ADDRESS_T;
}

/**
 * AddParam() - Add parameter to current procedure
 * 
 * Parameters are initially created as BYTE type.
 * Actual type will be set during DECLARE processing.
 */
static void AddParam() {
    if (FindScopedInfo(curScope))
        Wr1TokenErrorAt(ERR38);                /* DUPLICATE PARAMETER NAME */
        
    CreateInfo(curScope, BYTE_T, curSym);      // Initial type BYTE
    Wr1XrefDef();
    info->flag |= F_PARAMETER;
}

/**
 * ParseParams() - Parse procedure parameter list
 * 
 * Syntax: (param1, param2, ...)
 */
static void ParseParams() {
    uint8_t paramCnt = 0;
    
    if (YylexMatch(L_LPAREN)) {

        while (1) {
            if (YylexMatch(L_IDENTIFIER)) {
                AddParam();
                paramCnt++;
            } else {
                Wr1TokenErrorAt(ERR36);        /* MISSING PARAMETER */
                RecoverMissingParam();
            }
            if (YylexNotMatch(L_COMMA))
                break;
        }
        if (YylexNotMatch(L_RPAREN)) {
            Wr1TokenErrorAt(ERR37);            /* MISSING ') ' AT END OF PARAMETER LIST */
            RecoverToSemiOrRP();
            Yylex();
        }
        info = curProc; // note number of params. info's follow procInfo
    }
    info->paramCnt = paramCnt;
}

/****************************************************************************
 * ParseProcStmt() - Parse PROCEDURE statement
 * 
 * Syntax: label: PROCEDURE [(params)] [type] [attributes] ;
 * 
 * Creates procedure info entry, sets up new scope, and validates:
 * - No duplicate procedure name
 * - Not nested in REENTRANT procedure
 * - Within procedure count limits
 * 
 * Writes PROCEDURE or EXTERNAL token to intermediate file.
 * For EXTERNAL procedures, no body is expected.
 ****************************************************************************/
void ParseProcStmt() {
    info_t *parentProc;

    parentProc = curProc;
    curSym     = stmtLabels[1];                // Procedure name from label
    
    if (FindScopedInfo(curScope))              // Check for duplicate
        Wr1SyntaxError(ERR34);                 /* DUPLICATE procedure DECLARATION */
        
    CreateInfo(curScope, PROC_T, curSym);      // Create procedure info
    info->flag |= F_LABEL;
    Wr1XrefDef();
    
    if (procCnt == 254)                        // Check procedure limit
        Lexfatal(ERR35);                       /* LIMIT EXCEEDED: NUMBER OF PROCEDURES */
        
    procInfo[++procCnt] = info;                // Save in procedure table
    curScope = procCnt << 8;                   // New scope = proc# << 8
    curProc  = info;
    doBlkCnt = 0;                              // Reset DO block count
    PushBlock(curScope);
    
    ParseParams();
    ParseRetType();
    ParseProcAttrib();
    
    // Write to intermediate file
    info = curProc;
    if (!(info->flag & F_EXTERNAL)) {
        Wr1Byte(T1_PROCEDURE);
        Wr1Info(info);
        Wr1Byte(T1_SCOPE);
        Wr1Word(curScope);
    } else {
        Wr1Byte(T1_EXTERNAL);
        Wr1Info(info);
    }
    
    info->procId = High(curScope);
    
    // Check for nested procedure in REENTRANT
    if (parentProc && (parentProc->flag & F_REENTRANT))
        Wr1SyntaxError(ERR88);                 /* INVALID procedure NESTING, ILLEGAL IN reentrant procedure */
}
