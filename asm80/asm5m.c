/****************************************************************************
 *  asm5m.c: part of the C port of Intel's ISIS-II asm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"

static byte b5666[] = { 9, 0x2D, 0x80 }; /* bit vector 10 -> 00101101 10 */
                                         /* target, set, equ, regename, sp */
static byte b5669[] = { 0x3A, 8, 0x80, 0, 0, 0, 0, 0, 0x20 };
/* bit vector 59 -> 00001000 1000000 00000000 0000000
                    00000000 0000000 00000000 001 */
static byte op16[] = {
    /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, /* DW,DW,EQU,SET,ORG */
    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
}; /* LXI,IMM16 */
static byte chClass[] = {
    /*  0/8     1/9     2/A     3/B     4/C     5/D     6/E     7/F */
    /*00*/ CC_BAD, CC_BAD, CC_BAD,   CC_BAD,  CC_BAD,    CC_BAD, CC_BAD, CC_BAD,
    /*08*/ CC_BAD, CC_WS,  CC_EOL,   CC_BAD,  CC_WS,     CC_BAD, CC_BAD, CC_BAD,
    /*10*/ CC_BAD, CC_BAD, CC_BAD,   CC_BAD,  CC_BAD,    CC_BAD, CC_BAD, CC_BAD,
    /*18*/ CC_BAD, CC_BAD, CC_BAD,   CC_ESC,  CC_BAD,    CC_BAD, CC_BAD, CC_BAD,
    /*20*/ CC_WS,  CC_BAD, CC_BAD,   CC_BAD,  CC_DOLLAR, CC_BAD, CC_BAD, CC_QUOTE,
    /*28*/ CC_PUN, CC_PUN, CC_PUN,   CC_PUN,  CC_PUN,    CC_PUN, CC_BAD, CC_PUN,
    /*30*/ CC_DIG, CC_DIG, CC_DIG,   CC_DIG,  CC_DIG,    CC_DIG, CC_DIG, CC_DIG,
    /*38*/ CC_DIG, CC_DIG, CC_COLON, CC_SEMI, CC_BAD,    CC_BAD, CC_BAD, CC_LET,
    /*40*/ CC_LET, CC_LET, CC_LET,   CC_LET,  CC_LET,    CC_LET, CC_LET, CC_LET,
    /*48*/ CC_LET, CC_LET, CC_LET,   CC_LET,  CC_LET,    CC_LET, CC_LET, CC_LET,
    /*50*/ CC_LET, CC_LET, CC_LET,   CC_LET,  CC_LET,    CC_LET, CC_LET, CC_LET,
    /*58*/ CC_LET, CC_LET, CC_LET,   CC_BAD,  CC_BAD,    CC_BAD, CC_BAD, CC_BAR,
    /*60*/ CC_BAD, CC_LET, CC_LET,   CC_LET,  CC_LET,    CC_LET, CC_LET, CC_LET,
    /*68*/ CC_LET, CC_LET, CC_LET,   CC_LET,  CC_LET,    CC_LET, CC_LET, CC_LET,
    /*70*/ CC_LET, CC_LET, CC_LET,   CC_LET,  CC_LET,    CC_LET, CC_LET, CC_LET,
    /*78*/ CC_LET, CC_LET, CC_LET,   CC_BAD,  CC_BAD,    CC_BAD, CC_BAD, CC_BAD
};

static int macroLineSize; // current size of buffer allocated to macro expansion
#define MCHUNK 256        // grows by this many bytes as needed

void InsertSym(int tableId) {
    /* move up the top block of the symbol tables to make room */
    int nSymbols = (int)(endSymTab[TID_MACRO] - token.symbol);
    if (++endSymTab[TID_MACRO] >= &symbols[MAXSYMBOLS])
        RuntimeError(RTE_TABLE); /* table Error() */
    memmove(token.symbol + 1, token.symbol, nSymbols * sizeof(tokensym_t));
    /* insert the new symbol name */
    if (tableId == TID_SYMBOL) {
        endSymTab[TID_SYMBOL]++; // new end of symbols
        symTab[TID_MACRO]++;     // and macros moves up as well
    }
    token.symbol->name = AllocStr((char *)tokenStart, tableId == TID_MACRO);
    token.symbol->type = 0; /* clear the type */
    if ((int)strlen((char *)tokenStart) > maxSymWidth)
        maxSymWidth = (int)strlen((char *)tokenStart);
}

static bool OutSideTable(byte tableId) { /* check if token.symbol is outside table bounds */
    if (endSymTab[tableId] >= token.symbol && token.symbol >= symTab[tableId])
        return false;

    SyntaxError();
    return true;
}

void InsertMacroSym(word paramId, byte type) {
    if (OutSideTable(TID_MACRO))
        return;
    InsertSym(TID_MACRO);
    token.symbol->paramId = paramId; /* fill in the rest of the new entry */
    token.symbol->type    = type;
    token.symbol->flags   = 0;
    PopToken();
    PopToken();
    //	DumpSymbols(TID_MACRO);
}

byte labelUse;

void SetTokenType(byte type, bool isSetOrEqu) {
    token.type = type;
    if ((acc1ValType == K_REGNAME || acc1ValType == K_SP) && isSetOrEqu)
        token.type = 12 - type; /* set-> TT_SET, equ-> TT_EQU */
}

void UpdateSymbolEntry(word value, byte type) {
    byte flags, absFlag;
    bool lineSet, isSetOrEqu;
    byte origType;

    /* type = 2 -> target
          4 -> set
          5 -> equ
          6 -> reference
          9 -> word ref
         0x3a-> ??
             0x8x-> needs absolute value
    */

    origType   = token.type;
    isSetOrEqu = type == O_EQU || type == O_SET;
    absFlag    = 0;

    lineSet    = false;
    if (OutSideTable(TID_SYMBOL)) /* ignore if outside normal symbol table */
        return;
    flags = token.symbol->flags;

    if (tokenIdx > 1)
        SyntaxError();

    if (IsPhase1() && token.type == O_NAME) {
        if (createdUsrSym) {
            if (token.symbol->type >= 0x80 ||
                (type == MACRONAME && token.symbol->line != srcLineCnt)) {
                LocationError();
                absFlag = 0x80;
            }
        } else {
            InsertSym(TID_SYMBOL);
            flags = 0;
        }

        flags = (activeSeg == SEG_ABS ? 0 : UF_RBOTH) | (inPublic ? UF_PUBLIC : 0) |
                (inExtrn ? (UF_EXTRN + UF_RBOTH) : 0);
        if (labelUse == L_SETEQU) /* set or equ */
            flags = acc1RelocFlags;

        if (labelUse == L_TARGET) /* label: */
            flags |= activeSeg;

        if (hasVarRef && isSetOrEqu)
            token.type = O_SETEQUNAME;
        else
            SetTokenType(type, isSetOrEqu);
    } else if (passCnt == 2 && token.type == O_NAME && acc1ValType != O_NAME && isSetOrEqu) {
        SetTokenType(type, isSetOrEqu);
        if (token.symbol->type < 128) {
            token.symbol->type  = token.type;
            token.symbol->value = value;
            flags               = acc1RelocFlags;
            lineSet             = true;
        }
    } else if (IsPhase1() && token.type == O_REF && TestBit(type, b5666)) {
        if (inExtrn)
            token.type = O_LABEL;
        else {
            token.type = type;
            flags &= ~(UF_RBOTH | UF_SEGMASK); /* mask off seg, and relocate info*/
            if (labelUse == L_SETEQU)          /* set or equ */
                flags = acc1RelocFlags | UF_PUBLIC;
            else if (labelUse == L_TARGET &&
                     activeSeg != SEG_ABS) /* label: then add seg and relocation info */
                flags |= activeSeg | (UF_PUBLIC + UF_RBOTH);
        }
    } else if (IsPhase1() && type == O_REF && TestBit(token.type, b5666)) {
        if ((flags & (UF_PUBLIC | UF_EXTRN)) != 0)
            token.type = O_LABEL;
        else
            flags |= UF_PUBLIC;
    } else {
        if (IsPhase1())
            if ((token.type != type && token.type != TT_SET) || type == O_EQU)
                token.type = O_LABEL;

        if (!inPublic && TestBit(token.type, b5669))
            flags = acc1RelocFlags | (token.type != MACRONAME ? flags & UF_PUBLIC : 0);
        else {
            if (IsPhase1())
                token.type = O_LABEL;

            if (!(inPublic || inExtrn) && token.symbol->addr != value)
                PhaseError();
        }
    }
    absFlag |= (token.symbol->type & 0x80);

    if (IsPhase1() && (type == O_NAME || type == O_REF || origType != token.type))
        token.symbol->type = token.type | absFlag;

    byte symType = token.symbol->type;
    if (token.type == O_LABEL || symType == O_LABEL)
        MultipleDefError();

    if (symType >= 0x80)
        LocationError();

    if ((IsPhase1() && (token.type == type || (type == O_EQU && token.type == 7))) ||
        (type == 4 && BlankAsmErrCode()) || lineSet || type == MACRONAME)
        token.symbol->value = value;

    token.symbol->flags = flags;
    inPublic            = false;
    inExtrn             = false;
    if (token.symbol->type == O_REF)
        UndefinedSymbolError();

    hasVarRef = false;
    if (createdUsrSym)
        PopToken();
    //	DumpSymbols(TID_SYMBOL);
    //	DumpSymbols(TID_MACRO);
    //	DumpTokenStack();
}

/*
    two different tables types are used in lookup
    table 0: static keyword lookup the individual entries are coded as
        keyword as char *
        opcode base - byte
        // padding
        type - byte
        flags - byte
    initial entry is determined by hashing the name

    The other two tables are kept as sorted 8 byte entries to allow binary chop search
    individual entries are encoded as follows
        symbol name as char *
        word value used differently for different types of symbol
        type - byte (add 0x80 if abs value)
        flags - byte
            xxxxxnnn	nnn = seg 0->ABS, 1->CODE, 2->DATA,
            xxxx1xxx	low byte used
            xxx1xxxx	high byte used
            xx1xxxxx	is public
            x1xxxxxx	is external

    table 1: is the main symbol table which grows dynamically as new entries are inserted
    table 2: is a dynamic macro table 8 bytes per entry it holds the names of the current macro
    parameters. Both tables are contiguous. Symbol search in each table is done by binary chop
    and new symbols are inserted into the appropriate table. Other entries, possibly for both
    tables are moved upwards.

*/

byte Lookup(byte tableId) {
    tokensym_t *entryOffset;

    byte i;
    //    symEntry based entryOffset KEYWORD_T

    /* Keyword() lookup */
    if (tableId == TID_KEYWORD) { /* hash chain look up key word */
        entryOffset = in_word_set((char *)tokenStart);
        if (entryOffset && (controls.macroFile || entryOffset->type < MACRO)) {
            token.symbol = entryOffset;
            token.type   = token.symbol->type;
            if (token.type < SINGLE) /* instruction with arg */
                if (op16[token.type])
                    has16bitOperand = true;

            if (token.symbol->flags == 2 && !controls.mod85) /* RIM/SIM only valid on 8085 */
                SourceError('O');

            if (token.type == K_SP) { /* SP */
                if (!(curOp == LXI || curOp == REG16))
                    SourceError('X');
                token.type = K_REGNAME; /* reg */
            }
            return token.type & 0x7F;
        }
        return O_NAME;
    }

    /* MACRO and User() tables are stored sorted 8 bytes per entry */
    /* use binary chop to find entry */
    tokensym_t *lowOffset  = symTab[tableId];
    tokensym_t *highOffset = endSymTab[tableId];
    tokensym_t *midOffset;
    entryOffset = highOffset;

    /* binary chop search for id */

    while ((midOffset = lowOffset + ((highOffset - lowOffset) >> 1)) != entryOffset) {
        entryOffset = midOffset;
        int cmp     = strcmp(entryOffset->name, (char *)tokenStart);
        if (cmp == 0) {
            token.symbol = entryOffset;
            token.type   = token.symbol->type;
            if (token.type == O_SETEQUNAME)
                token.type = O_NAME;

            if ((usrLookupIsID = (token.type & 0x7F) == O_NAME))
                if (needsAbsValue)
                    token.symbol->type = 0x80 + O_NAME; /* update ABSVALUE flag */
            return token.type & 0x7F;
        }

        if (cmp > 0)
            highOffset = entryOffset;
        else
            lowOffset = entryOffset;
    }

    token.symbol = highOffset;
    if (tableId == TID_SYMBOL && !IsSkipping()) {
        createdUsrSym = false;
        labelUse      = L_REF;
        UpdateSymbolEntry(srcLineCnt, needsAbsValue ? 0x80 | O_NAME : O_NAME);
        /* update symbol stack to adjust pointers for entries above insert point */
        for (i = 1; i <= tokenIdx; i++) {
            if (tokenStk[i].symbol >= token.symbol)
                tokenStk[i].symbol++;
        }

        createdUsrSym = true;
    }
    return O_NAME;
}

/*
    get the next character handling command line, and macro expansion
    for macros there are special chars
    MACROEOB 0xFE	end of macro buffer causes next block to be read
    MACROPARAM 0x80	indicates a macro parameter to be expanded the parameter number is in lookahead
                    for IRPC the next char is taken which can be escaped, otherwise the macro is
                    expanded
   >MACROPARAM		indicates local variable. lookahead is the local variable number in the
                    current macro it is added to the base number for locals for the expansion
                    of this macro and converted into ??number as a unique local label
    ESC	0x1B		end of spooled macro, returns ESC to higher order routines to handle
*/
byte GetCh(void) {
    static byte curCH, prevCH;
    static byte irpcChr[3]      = { 0, 0, 0x81 }; // where irpc char is built (0x81 is end marker)
    static char localVarName[7] = {
        '?', '?', 0, 0, 0, 0, 0x80
    }; // where DoLocal vars are constructed (0x80 is end marker)
    static pointer savedMacroBufP;

    while (!reget) {
        prevCH = curCH;
        do {
            curCH = lookAhead;
            if (curCH == EOLCH)
                lookAhead = 0;
            else if (expandingMacro) {
                while ((lookAhead = *curMacro.bufP) == MACROEOB) {
                    ReadM(curMacroBlk + 1);
                    curMacro.bufP = macroBuf;
                }
                curMacro.bufP++;
            } else
                lookAhead = scanCmdLine ? GetCmdCh() : GetSrcCh();
        } while (curCH == 0 || curCH == 0x7F || curCH == FF);

        if (expandingMacro) {
            if (curCH == ESC) // reached end of spooled macro
                break;
            else if (curCH == '&') {
                if (prevCH >= MACROPARAM ||
                    lookAhead == MACROPARAM)          // macro param/Local before or after
                    continue;                         // ignore as text will be joined
            } else if (curCH == '!' && prevCH != 0) { // ! and previous char wasn't a !
                if (!(inMacroBody || (mSpoolMode & 1)) && expandingMacroParameter) {
                    curCH = 0; // will make sure !! is passed through as !
                    continue;
                }
            } else if (curCH >= MACROPARAM) {
                if (!(expandingMacroParameter = !expandingMacroParameter))
                    curMacro.bufP =
                        savedMacroBufP; // back to macro as macro parameter expansion has finished
                else {
                    savedMacroBufP = curMacro.bufP; // is parameter or Local
                    if (curCH == MACROPARAM) {      // parameter
                        if (savedMtype == M_IRPC) {
                            curMacro.bufP = irpcChr;
                            irpcChr[0]    = curMacro.pCurArg[0];
                            irpcChr[1]    = irpcChr[0] == '!' ? curMacro.pCurArg[1] : 0x81;
                        } else {
                            curMacro.bufP = curMacro.pCurArg;  // parameter text
                            while ((byte)(--lookAhead) != 0) { // skip to required parameter
                                curMacro.bufP += (*curMacro.bufP & 0x7F);
                            }
                            curMacro.bufP++; // skip over the length
                        }
                    } else { // Local
                        curMacro.bufP = (pointer)
                            localVarName; // generate Local id from instance & current DoLocal base
                        // generate DoLocal variable name
                        sprintf(localVarName, "??%04d", (lookAhead + curMacro.localIdBase) % 10000);
                        localVarName[6] = 0x80;
                    }
                }

                lookAhead = 0;
                continue;
            }
        }

        if (expandingMacro > 1) /* in pass 2 print expanded macro code if required */
            if (IsPhase2Print()) {
                if (macroPIdx >= macroLineSize - 1) // allow for '\0'
                    macroLine = safeRealloc(macroLine, macroLineSize += MCHUNK);
                macroLine[macroPIdx++] = curCH;
            }

        if (mSpoolMode & 1) /* spool char if not in excluded comments or is the end of line EOLCH
                               for none empty line */
            if ((startMacroLineIdx != macroInIdx && curCH == EOLCH) || !excludeCommentInExpansion)
                InsertByteInMacroTbl(curCH);

        if (!(prevCH == '!' || inComment)) { /* if not escaped or in comment */
            if (curCH == '>')                /* handle < > nesting */
                argNestCnt--;

            if (curCH == '<')
                argNestCnt++;
        }
        break;
    }
    reget = 0;
    return (curChar = curCH);
}

byte GetChClass(void) {
    curChar = GetCh();

    return inMacroBody ? CC_MAC : chClass[curChar];
}