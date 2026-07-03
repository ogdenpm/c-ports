/****************************************************************************
 *  plm0f.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
 * plm0f.c — PL/M-80 program parser (phase 1 state machine)
 *
 * Overview
 * --------
 * This module implements the top-level structural parser for a PL/M-80
 * source program.  It is driven by a explicit parse stack (stateStack[])
 * rather than recursive descent, allowing the parser to be re-entered
 * safely and to handle the language's block-structured grammar without
 * deep C-call-stack recursion.
 *
 * The public entry point is ParseProgram(), which initialises the stack,
 * seeds it with state 0 and loops until the stack is empty, dispatching
 * each popped state to its handler function.
 *
 * Grammar elements handled
 * ------------------------
 *  Module header        State0   – "name: DO;"
 *  Module body          State1/2 – module-level unit list and END
 *  Declarations         State3/19 – DECLARE element lists
 *  Procedure statement  State4/5/6/21 – header, EXTERNAL variant, ending
 *  Do blocks            State12  – iterative DO, DO WHILE, DO CASE, plain DO
 *  Do ending            State13  – END with optional label match
 *  Case block           State14/15 – CASE unit list, jump-to-end stitching
 *  Conditional clause   State16  – IF … THEN
 *  Else clause          State17/18 – optional ELSE branch
 *  Plain unit           State11  – assignment, CALL, GOTO, HALT, ENABLE,
 *                                   DISABLE, RETURN, null statement
 *  Unit list            State9   – repeated unit dispatch
 *  Unexpected decl      State20  – out-of-place DECLARE/PROCEDURE recovery
 *  Block skip           State7/8 – error recovery: skip nested blocks
 *
 * Lexical stream output
 * ---------------------
 * Parsed constructs are serialised into the phase-1 lexical stream via
 * Wr1Byte() / Wr1Word() / Wr1Info() / Wr1LexToken() etc.  Token constants
 * are T1_xxx.  Label definitions, scope changes and statement counts are
 * also written here.
 *
 * State-stack protocol
 * --------------------
 * Each State*() function pushes the states it wants to run *after* it
 * returns, in reverse order.  The main loop in ParseProgram() pops one
 * state per iteration, so the last-pushed state is executed next.
 * PushStateWord() / PopStateWord() manage the stack; integers other than
 * state numbers are also pushed as opaque data values (label indices,
 * info-table offsets, symbol-table offsets, saved doBlkCnt).
 */

#include "plm.h"

/* curState – the state number currently being executed (informational). */
uint16_t curState;

/* endSeen – set by State20() when an unexpected END is encountered during
   out-of-place declaration recovery; cleared at the start of State0(). */
bool endSeen;

/*
 * SkipToSemiColon – discard tokens until L_SEMICOLON is consumed.
 *
 * Used for error recovery.  Resets yyAgain so the semicolon is not
 * re-processed by the caller.
 */
static void SkipToSemiColon() {
    while (tokenType != L_SEMICOLON)
        Yylex();
    yyAgain = false;
}

/*
 * ExpectSemiColon – consume a mandatory semicolon.
 *
 * If the next token is not L_SEMICOLON, ERR32 is raised and
 * SkipToSemiColon() is called to resync.
 */
static void ExpectSemiColon() {
    if (YylexNotMatch(L_SEMICOLON)) {
        Wr1TokenErrorAt(ERR32); /* INVALID SYNTAX, TEXT IGNORED UNTIL ';' */
        SkipToSemiColon();
    }
}

/*
 * ErrorSkip – skip to the next semicolon and re-arm yyAgain.
 *
 * Used after an illegal statement is detected so the parser re-examines
 * the semicolon as the start of the next statement.
 */
static void ErrorSkip() {
    SkipToSemiColon();
    SetYyAgain();
}

/*
 * PopStateWord – pop and return the top word from the parse stack.
 *
 * Calls Lexfatal(ERR97) if the stack is already empty (underflow).
 */
static uint16_t PopStateWord() {
    if (stateSP == 0)
        Lexfatal(ERR97); /* COMPILER Error: PARSE STACK UNDERFLOW */
    return stateStack[stateSP--];
}

/*
 * NewLocalLabel – reserve a new compiler-generated local label index.
 *
 * Increments localLabelCnt so that later passes (code generation) have a
 * slot pre-allocated for a branch target (e.g. end-of-case, end-of-if).
 */
static void NewLocalLabel() {
    localLabelCnt++;
}

/*
 * ParseStmtLabels – collect the label prefix(es) of a statement.
 *
 * Scans for zero or more "identifier ':'  " sequences.  Each label symbol
 * is stored in stmtLabels[] (max 9; ERR30 if exceeded) and stmtLabelCnt
 * is updated.  On return, stmtStartToken and stmtStartSymbol hold the
 * first non-label token that ended the scan.
 */
static void ParseStmtLabels() {
    stmtLabelCnt = 0;

    while (1) {
        stmtStartToken  = tokenType;
        stmtStartSymbol = curSym;
        if (tokenType != L_IDENTIFIER || !YylexMatch(L_COLON))
            return;
        if (stmtLabelCnt == 9)
            Wr1TokenErrorAt(ERR30); /* LIMIT EXCEEDED: NUMBER OF LABELS ON STATEMENT */
        else
            stmtLabelCnt++;
        stmtLabels[stmtLabelCnt] = curSym;
        Yylex();
    }
}

/*
 * startLexCodeMap – maps stmtStartCode (S_xxx) to the T1_xxx token that
 * opens the corresponding statement in the phase-1 lexical stream.
 *
 * Index  S_xxx constant     T1_xxx token       Statement kind
 *  0     S_STATEMENT        T1_STATEMENT       assignment
 *  1     S_SEMICOLON        T1_SEMICOLON       null statement
 *  2     S_CALL             T1_CALL            CALL
 *  3     S_DECLARE          T1_LINEINFO        DECLARE
 *  4     S_DISABLE          T1_DISABLE         DISABLE  (8080)
 *  5     S_DO               T1_DO              DO*
 *  6     S_ENABLE           T1_ENABLE          ENABLE   (8080)
 *  7     S_END              T1_END             END
 *  8     S_GO               T1_GO              GO TO
 *  9     S_GOTO             T1_GOTO            GOTO
 * 10     S_HALT             T1_HALT            HALT     (8080)
 * 11     S_IF               T1_IF              IF
 * 12     S_PROCEDURE        T1_PROCEDURE       PROCEDURE
 * 13     S_RETURN           T1_RETURN          RETURN
 */
static uint8_t startLexCodeMap[] = { T1_STATEMENT, T1_SEMICOLON, T1_CALL,   T1_LINEINFO,
                                     T1_DISABLE,   T1_DO,        T1_ENABLE, T1_END,
                                     T1_GO,        T1_GOTO,      T1_HALT,   T1_IF,
                                     T1_PROCEDURE, T1_RETURN };

/*
 * ParseStartStmt – lex the first token(s) of a new statement.
 *
 * Reads the next token (with XREF suppressed so label references are not
 * cross-referenced here), increments curStmtCnt, emits T1_STMTCNT to the
 * lexical stream, raises ERR86 if the previous line was truncated, then
 * calls ParseStmtLabels() to collect any label prefix.
 *
 * Sets stmtStartCode (S_xxx) from the first non-label token:
 *   L_SEMICOLON  → S_SEMICOLON (re-arm yyAgain)
 *   L_IDENTIFIER → S_STATEMENT
 *   L_CALL–L_RETURN → mapped via the keyword range offset to S_CALL–S_RETURN
 *   anything else → ERR29 (ILLEGAL STATEMENT), S_SEMICOLON, ErrorSkip()
 *
 * startLexCode is set to startLexCodeMap[stmtStartCode].
 * If endSeen is true on entry, it is cleared and the function returns
 * immediately (the END has already been handled by State20).
 */
static void ParseStartStmt() {
    bool tmp;

    if (endSeen) {
        endSeen = false;
        return;
    }
    tmp  = XREF;
    XREF = false;
    Yylex();
    XREF = tmp;
    curStmtCnt++;
    if (linfo.stmtCnt == 0) {
        linfo.stmtCnt = curStmtCnt;
        linfo.blkCnt  = curBlkCnt;
    }
    Wr1Byte(T1_STMTCNT);
    Wr1Word(curStmtCnt);
    if (trunc) {
        Wr1SyntaxError(ERR86); /* LIMIT EXCEEDED: SOURCE LINE length() */
        trunc = false;
    }
    ParseStmtLabels();
    if (stmtStartToken == L_SEMICOLON) {
        stmtStartCode = S_SEMICOLON;
        SetYyAgain();
    } else if (stmtStartToken == L_IDENTIFIER)
        stmtStartCode = S_STATEMENT;
    else if (stmtStartToken >= L_CALL && stmtStartToken <= L_RETURN)
        stmtStartCode = stmtStartToken - L_CALL + S_CALL; /* maps to S_CALL through S_RETURN */
    else {
        Wr1TokenErrorAt(ERR29); /* ILLEGAL STATEMENT */
        stmtStartCode = S_SEMICOLON;
        ErrorSkip();
    }
    startLexCode = startLexCodeMap[stmtStartCode];
}

/*
 * WrLabelDefs – write label-definition records for the current statement's
 *               label prefix to the phase-1 lexical stream.
 *
 * For each symbol in stmtLabels[1..stmtLabelCnt]:
 *   • If the symbol already exists at the current scope and has F_LABEL set,
 *     ERR33 (DUPLICATE LABEL) is raised.
 *   • If it exists but was forward-referenced without F_LABEL, T1_LABELDEF
 *     and the info index are written and F_LABEL is set.
 *   • If it is new, a LABEL_T info entry is created, T1_LABELDEF is written
 *     and F_LABEL is set.
 * Cross-reference records (Wr1XrefDef / Wr1XrefUse) are also emitted.
 * At module scope (curScope == 0x100) an extra T1_MODULE token is written.
 */
static void WrLabelDefs() {

    sym_t *tmp = curSym; // save to restore at end
    if (stmtLabelCnt != 0) {
        for (uint16_t i = 1; i <= stmtLabelCnt; i++) { // check each label
            curSym = stmtLabels[i];
            if (FindScopedInfo(curScope)) { /* already seen at this scope */
                if ((info->flag & F_LABEL))
                    Wr1TokenError(ERR33, curSym); /* DUPLICATE LABEL DECLARATION */
                else {
                    Wr1Byte(T1_LABELDEF); // log label def to lexical stream
                    Wr1Info(info);
                    info->flag |= F_LABEL; // mark as defined
                }
                Wr1XrefUse(); // write xref
            } else {
                CreateInfo(curScope, LABEL_T, curSym); // its new so create
                Wr1XrefDef();                          // write xref
                Wr1Byte(T1_LABELDEF);
                Wr1Info(info);
                info->flag |= F_LABEL;
            }
        }
        if (curScope == 0x100)
            Wr1Byte(T1_MODULE); // record at module level
    }
    curSym = tmp;
} /* WrLabelDefs() */

/*
 * IsEndOfModule – validate and consume the closing END of the module.
 *
 * Optionally matches an identifier after END and raises ERR20 if it does
 * not match the module name (procInfo[1]->sym).  Then calls ExpectSemiColon().
 *
 * If afterEOF is true (already past the synthetic EOF token, i.e. in
 * recovery mode), T1_END is written and true is returned immediately.
 * Otherwise the next token is examined:
 *   • afterEOF true  → this was the real final END; T1_END written, returns true.
 *   • afterEOF false → stray END that is not the module end; ERR93 raised,
 *                      yyAgain re-armed, returns false.
 */
static bool IsEndOfModule() {
    if (YylexMatch(L_IDENTIFIER) && procInfo[1]->sym != curSym)
        Wr1TokenErrorAt(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
    ExpectSemiColon();
    if (afterEOF) {      // END after EOF i.e. recovery mode
        Wr1Byte(T1_END); // write closing END to lex stream
        return true;     // no more processing
    } else {
        Yylex();
        if (afterEOF) {            // was final end
            Wr1Byte(T1_END);       // write the END to lex stream
            afterEOF = false;      // no need to add recovery tokens
            return true;           // no more processing
        } else {                   // report error and keep going
            Wr1SyntaxError(ERR93); /* MISSING 'DO' FOR 'END' , 'END' IGNORED */
            SetYyAgain();
            return false;
        }
    }
} /* IsEndOfModule() */

/*
 * ProcEnding – process the END of a defined procedure.
 *
 * Calls PopBlock() to restore the enclosing scope, optionally matches an
 * identifier and raises ERR20 if it does not match the procedure name.
 * Scans all formal parameters of the procedure and raises ERR25 for any
 * that were never declared (F_LABEL not set).
 * Restores doBlkCnt and curProc from the parse stack (values saved in
 * State4) and calls ExpectSemiColon().
 */
static void ProcEnding() {
    PopBlock();
    if (YylexMatch(L_IDENTIFIER) && curProc->sym != curSym)
        Wr1TokenErrorAt(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */

    info = curProc;
    for (int i = info->paramCnt; i > 0; i--) { // scan any parameters (info after proc info)
        AdvNxtInfo();
        if (!(info->flag & F_LABEL))         // not declared?
            Wr1TokenError(ERR25, info->sym); /* UNDECLARED PARAMETER */
    }
    doBlkCnt = PopStateWord(); // restore doBlkCnt & curProcInfoP to parent of proc
    info = curProc = FromIdx(PopStateWord());
    ExpectSemiColon(); // finish off statement
}

/*
 * PushStateWord – push a word onto the parse stack.
 *
 * Calls Lexfatal(ERR31) if the stack is full (overflow at depth 99).
 */
static void PushStateWord(uint16_t state) {
    if (stateSP != 99)
        stateStack[++stateSP] = state;
    else
        Lexfatal(ERR31); /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
}

/*
 * CreateModuleInfo – create the implicit PROC_T info entry for the module.
 *
 * Looks up symPtr in the symbol table, creates a PROC_T info entry at
 * scope 0, sets F_LABEL and writes a cross-reference definition.  Sets
 * curProc, procId 1, procCnt 1, curScope 0x100 (proc=1 do-level=0).
 * Writes T1_DO / T1_SCOPE / curScope to the lexical stream and calls
 * PushBlock() to enter the module scope.
 */
static void CreateModuleInfo(sym_t *symPtr) {
    curSym = symPtr;
    CreateInfo(0, PROC_T, curSym);
    info->flag |= F_LABEL;
    Wr1XrefDef();
    curProc      = info;
    info->procId = 1;
    procCnt      = 1;
    procInfo[1]  = info;
    curScope     = 0x100; /* proc = 1,  do level = 0 */
    Wr1Byte(T1_DO);
    Wr1Byte(T1_SCOPE);
    Wr1Word(curScope);
    PushBlock(curScope);
}

/*
 * State0 – parse the module header: "name : DO ;"
 *
 * Calls ParseStartStmt() to read the first statement.  If the statement
 * does not start with S_DO, ERR89 is raised, a synthetic "MODULE" name is
 * generated, and the parser jumps straight to declaration parsing (state 19).
 * Otherwise the label before DO is used as the module name (ERR90 if
 * missing; ERR18 if more than one label is present).  After creating the
 * module info record, state 3 is pushed to start declaration parsing.
 * State 1 (module-level unit check) is always pushed above the rest.
 */
static void State0() {
    ParseStartStmt();
    PushStateWord(1); // check for module level <unit>
    if (stmtStartCode != S_DO) {
        Wr1SyntaxError(ERR89);         /* MISSING 'DO' FOR MODULE */
        Lookup((pstr_t *)"\x6MODULE"); // generate a default module name
        CreateModuleInfo(curSym);
        PushStateWord(19); // next parse <declaration> using stmtStartCode
    } else {
        if (stmtLabelCnt == 0) {
            Wr1SyntaxError(ERR90);         /* MISSING NAME FOR MODULE */
            Lookup((pstr_t *)"\x6MODULE"); // generate default name
            stmtLabelCnt  = 1;
            stmtLabels[1] = curSym;
        } else if (stmtLabelCnt > 1) // module can have only one name
            Wr1SyntaxError(ERR18);   /* INVALID MULTIPLE LABELS AS MODULE NAMES */
        CreateModuleInfo(stmtLabels[1]);
        ExpectSemiColon();
        PushStateWord(3); // get start statement and parse <declaration>
    }
} /* State0() */

/*
 * State1 – check for a module-level unit.
 *
 * If the current statement is not S_END, sets haveModuleLevelUnit, writes
 * T1_MODULE to mark the transition from declarations to executable code,
 * and pushes state 2 to parse the module-level unit list.
 * If it is S_END, validates the module-closing END via IsEndOfModule().
 * On a non-final END, the state machine loops back to states 1 and 10.
 * A label on the END without a matching module unit raises ERR19.
 */
static void State1() {
    if (stmtStartCode != S_END) {
        haveModuleLevelUnit = true;
        Wr1Byte(T1_MODULE);
        PushStateWord(2); // parse module level <unit>...
    } else {
        if (stmtLabelCnt)
            Wr1TokenErrorAt(ERR19); /* INVALID LABEL IN MODULE WITHOUT MAIN PROGRAM */
        if (!IsEndOfModule()) {
            PushStateWord(1);  // check for module level <unit>
            PushStateWord(10); // get the statement start
        }
    }
} /* State1() */

/*
 * State2 – parse module-level unit list.
 *
 * If the current statement is not S_END, pushes states 2 / 10 / 11 to
 * re-enter this state after the next unit is parsed.
 * On S_END, calls WrLabelDefs() for any labels on the END, then either
 * finishes the module (IsEndOfModule() + T1_HALT) or re-queues states
 * 2 / 10 on a non-final END.
 */
static void State2() {
    if (stmtStartCode != S_END) {
        PushStateWord(2);  // parse module level <unit>...
        PushStateWord(10); // get start statement after processing
        PushStateWord(11); // parse <unit>
    } else {
        WrLabelDefs(); // write any END label prefixes
        if (IsEndOfModule())
            Wr1Byte(T1_HALT); // write the final halt
        else {
            PushStateWord(2);  // parse module level <unit>...
            PushStateWord(10); // get start statement
        }
    }
} /* State2() */

/*
 * State3 – get a statement start then parse a declaration.
 *
 * Calls ParseStartStmt() and pushes state 19 to process the resulting
 * stmtStartCode as a declaration or procedure header.
 */
static void State3() {
    ParseStartStmt();  // get statement start
    PushStateWord(19); // next state parse <declaration>
}

/*
 * State19 – parse <declaration>.
 *
 * If stmtStartCode is S_DECLARE, calls ParseDeclareElementList(), pushes
 * state 3 to continue with the next statement, then calls
 * ExpectSemiColon().
 * If stmtStartCode is S_PROCEDURE, pushes states 3 and 4 so that after the
 * procedure body is parsed, declaration scanning resumes.
 * Any other stmtStartCode (including S_END) falls through silently,
 * allowing the caller (State3) to continue into unit parsing.
 */
static void State19() { /* check for declare or procedure */
    if (stmtStartCode == S_DECLARE) {
        ParseDeclareElementList();
        PushStateWord(3);  // next state get statement start & <declaration>
        ExpectSemiColon(); // but first check for semicolon
    } else if (stmtStartCode == S_PROCEDURE) {
        PushStateWord(3); // after processing procedure check for another declare or procedure
        PushStateWord(4); // next <procedure statement>
    }
}

/*
 * State4 – parse the start of a PROCEDURE statement.
 *
 * Requires exactly one label (the procedure name); ERR21 if none, ERR22 if
 * more than one.  Saves the current curProc info index and doBlkCnt on the
 * stack for restoration in ProcEnding().  Calls ParseProcStmt() to process
 * parameter list, return type and attributes, then ExpectSemiColon().
 *
 * For EXTERNAL procedures: pushes state 5 (declaration + ending).
 * For defined procedures:  pushes states 6 / 21 / 3 to handle the body
 * (declarations, non-null unit check, ending).
 * On missing label: pushes state 7 to skip the bad nested block.
 */
static void State4() {
    if (stmtLabelCnt == 0) {
        Wr1SyntaxError(ERR21); /* MISSING procedure NAME */
        PushStateWord(7);      // skip bad nested proc/do
    } else {
        if (stmtLabelCnt != 1) {
            Wr1TokenErrorAt(ERR22); /* INVALID MULTIPLE LABELS AS PROCEDURE NAMES */
            stmtLabelCnt = 1;
        }
        PushStateWord(ToIdx(curProc)); // save current procInfoP
        PushStateWord(doBlkCnt);       // and block count restored in PraseEnding
        ParseProcStmt();               // parse optional parameters, return type and attributes
        ExpectSemiColon();             // finish with semicolon
        info = curProc;                // test if external proc
        if ((info->flag & F_EXTERNAL))
            PushStateWord(5); // next <declaration> <ending>
        else {
            PushStateWord(6);  // finish with parse <ending>
            PushStateWord(21); // parse non null <unit>
            PushStateWord(3);  // next new start & <declaration>
        }
    }
}

/*
 * State5 – parse <declaration> and <ending> for an EXTERNAL procedure.
 *
 * Calls ParseStartStmt() for each statement in the external procedure body.
 * Only DECLARE statements are legal here; anything else raises ERR24.
 * Nested PROCEDURE or DO blocks in error are skipped via state 7.
 * The loop repeats (via PushStateWord(5)) until S_END is reached.
 * On S_END the label must be absent (ERR23 if present) and ProcEnding()
 * closes the scope.
 */
static void State5() {
    ParseStartStmt();
    if (stmtStartCode == S_END) { // cannot label END in external proc def
        if (stmtLabelCnt != 0) {
            Wr1TokenErrorAt(ERR23); /* INVALID LABELLED END IN EXTERNAL PROCEDURE */
            stmtLabelCnt = 0;
        }
        ProcEnding();
    } else {
        PushStateWord(5); // repeat until final end
        if (stmtStartCode == S_DECLARE) {
            ParseDeclareElementList(); // get the parameter declarations
            ExpectSemiColon();         // finish
        } else {
            Wr1TokenErrorAt(ERR24); /* INVALID STATEMENT IN external procedure */
            if (stmtStartCode == S_PROCEDURE || stmtStartCode == S_DO)
                PushStateWord(7); // skip bad nested proc/do
            else
                SkipToSemiColon(); // else skip to end of this statement
        }
    }
}

/*
 * State21 – check that a procedure body is non-null.
 *
 * If the first statement of the body is already S_END, ERR174 (NULL
 * PROCEDURE) is raised.  Otherwise state 9 is pushed to parse the unit list.
 */
static void State21() {
    if (stmtStartCode == S_END)
        Wr1TokenErrorAt(ERR174); /* INVALID NULL procedure */
    else
        PushStateWord(9); // parse <unit>
}

/*
 * State6 – parse the <ending> (END …;) of a defined procedure.
 *
 * Calls WrLabelDefs() for any labels on the END statement, then
 * ProcEnding() to validate the procedure name, check parameters and
 * restore scope.  Finally writes T1_END to the lexical stream.
 */
static void State6() {
    WrLabelDefs(); // write any labels on the END to the lex stream
    ProcEnding();
    Wr1Byte(T1_END); // add end to the lexical stream
}

/*
 * State7 – begin skipping a bad nested block.
 *
 * Skips to the semicolon of the current (malformed) statement, then pushes
 * state 8 to scan forward through any further nested blocks until the
 * matching END is found.
 */
static void State7() {
    SkipToSemiColon(); // skip this statement
    PushStateWord(8);  // skip to end of proc / do
}

/*
 * State8 – skip nested blocks until a matching END is reached.
 *
 * Calls ParseStartStmt() to read each statement.  Nested PROCEDURE or DO
 * blocks are recursively handled by pushing states 8 / 7 again.  All other
 * statements are skipped to their semicolons.  The loop exits when S_END is
 * seen without re-pushing state 8.
 */
static void State8() {
    ParseStartStmt();
    if (stmtStartCode == S_PROCEDURE || /* nested block */
        stmtStartCode == S_DO) {
        PushStateWord(8); // skip rest of this proc/do
        PushStateWord(7); // after skipping nested proc/do
    } else {
        SkipToSemiColon();          /* skip to end of statement */
        if (stmtStartCode != S_END) /* if ! an end then go again */
            PushStateWord(8);       // keep skipping
    }
}

/*
 * State9 – parse a <unit> list.
 *
 * If stmtStartCode is not S_END, pushes states 9 / 10 / 11 to process
 * the next unit after the current one.  On S_END the list terminates and
 * nothing is pushed (the enclosing state handles the END).
 */
static void State9() {
    if (stmtStartCode != S_END) {
        PushStateWord(9);  // repeat parsing statement after this statement is parsed
        PushStateWord(10); // get start statement after processing
        PushStateWord(11); // parse <unit>
    }
}

/*
 * State10 – get the statement start token.
 *
 * Thin wrapper that calls ParseStartStmt() to advance the token stream
 * and set stmtStartCode / stmtStartToken / stmtStartSymbol.
 */
static void State10() {
    ParseStartStmt();
}

/*
 * State11 – parse a single <unit>.
 *
 * Dispatches on stmtStartCode:
 *   S_DECLARE / S_PROCEDURE / S_END
 *                  → ERR26 (out of place), push state 20 for recovery.
 *   S_DO           → push state 12 to parse the do block.
 *   S_IF           → push state 16 to parse the conditional clause.
 *   anything else  → write label defs, emit startLexCode, parse the rest of
 *                    the basic statement via ParseExpresion(L_SEMICOLON), and
 *                    call ExpectSemiColon().  Assignment statements also emit
 *                    T1_IDENTIFIER for the left-hand-side variable.
 *                    Null statements (S_SEMICOLON) skip expression parsing.
 */
static void State11() {
    if (stmtStartCode == S_DECLARE || stmtStartCode == S_PROCEDURE || stmtStartCode == S_END) {
        Wr1TokenErrorAt(ERR26); /* INVALID DECLARATION, STATEMENT OUT OF PLACE */
        PushStateWord(20);      // parse unexpected <declaration>
    } else if (stmtStartCode == S_DO)
        PushStateWord(12); // parse rest of <do block>
    else if (stmtStartCode == S_IF)
        PushStateWord(16);                      // parse rest of <conditional clause>
    else {                                      // <label definition> <unit>
        WrLabelDefs();                          // write labels
        Wr1Byte(startLexCode);                  // and first lex token of <basic statement>
        if (stmtStartCode != S_SEMICOLON) {     /* Semicolon() */
            if (stmtStartCode == S_STATEMENT) { /* Variable() */
                Wr1Byte(T1_IDENTIFIER);         // write var for <assignment statement>
                Wr1Word((uint16_t)(stmtStartSymbol - symtab));
            }
            ParseExpresion(L_SEMICOLON);
        }
        ExpectSemiColon();
    }
}

/*
 * State20 – parse an unexpected <declaration>.
 *
 * Called when a DECLARE or PROCEDURE is found in a position where only
 * executable statements are valid (ERR26 was already raised in State11).
 *
 * Handles:
 *   S_DECLARE   → ParseDeclareElementList(), re-push state 20 and 10.
 *   S_PROCEDURE → re-push state 20, 10, 4 to process and then continue.
 *   S_END       → set endSeen so ParseStartStmt() short-circuits next call.
 *   anything else → push state 11 to parse the unit normally.
 */
static void State20() {
    if (stmtStartCode == S_DECLARE) {
        ParseDeclareElementList();
        ExpectSemiColon();
        PushStateWord(20); // parse unexpected <declaration>
        PushStateWord(10); // get start statement
    } else if (stmtStartCode == S_PROCEDURE) {
        PushStateWord(20); // parse unexpected <declaration>
        PushStateWord(10); // get start statement
        PushStateWord(4);  // parse start <procedure statement>
    } else if (stmtStartCode == S_END)
        endSeen = true;
    else
        PushStateWord(11); // parse <unit>
}

/*
 * State12 – parse a DO block (DO keyword already consumed).
 *
 * Calls WrLabelDefs() for labels on the DO statement and pushes the last
 * label's symbol-table offset (or 0) for later matching by State13.
 *
 * Determines the DO variant by peeking at the next token:
 *   identifier  → iterative DO VARIABLE = exp TO exp [BY exp];
 *                 writes T1_DOLOOP, emits the index variable, parses the
 *                 range expression.  Pushes states 13 / 9 / 10.
 *   WHILE       → DO WHILE (expr);  writes T1_WHILE.  Pushes 13 / 9 / 10.
 *   CASE        → DO CASE (expr);   writes T1_CASE, reserves an end-of-case
 *                 label via NewLocalLabel(), pushes state 14.
 *   (other)     → simple DO;        writes T1_DO.  Pushes 13 / 9 / 3
 *                 (declarations allowed inside a plain DO block).
 *
 * Calls ExpectSemiColon(), increments doBlkCnt (ERR27 if > 255), updates
 * curScope, calls PushBlock(), and writes T1_SCOPE / curScope.
 */
static void State12() {
    WrLabelDefs(); // write any prefix labels
    if (stmtLabelCnt != 0)
        PushStateWord(
            (uint16_t)(stmtLabels[stmtLabelCnt] - symtab)); // push the address of the last label
    else
        PushStateWord(0); // or null if none
    if (YylexMatch(L_IDENTIFIER)) {
        Wr1Byte(T1_DOLOOP); // convert <iterative do statement> to lex format
        Wr1LexToken();      // the <index variable>
        ParseExpresion(L_SEMICOLON);
        PushStateWord(13);            // parse <ending>  labels already collected
        PushStateWord(9);             // parse <unit>...
        PushStateWord(10);            // get start statement
    } else if (YylexMatch(L_WHILE)) { // <do-while block>
        Wr1Byte(T1_WHILE);            // convert <do-while statement> to lex format
        ParseExpresion(L_SEMICOLON);
        PushStateWord(13);           // parse <ending>  labels already collected
        PushStateWord(9);            // parse <unit>...
        PushStateWord(10);           // get start statement
    } else if (YylexMatch(L_CASE)) { // <do-case block>
        Wr1Byte(T1_CASE);            // convert <do-case statement> to lex format
        ParseExpresion(L_SEMICOLON);
        NewLocalLabel(); // make sure later passes have space for the end of case target
        PushStateWord(localLabelCnt); // push the index of end of case target for later
        PushStateWord(14);            // parse case <unit>... <ending>
    } else {
        Wr1Byte(T1_DO);    // convert <simple do statement> to lex format
        PushStateWord(13); // parse <ending>  labels already collected
        PushStateWord(9);  // parse <unit>...
        PushStateWord(3);  // get start statement and parse <declaration>
    }
    ExpectSemiColon(); /* we should now see a semicolon at end of do ... statement */
    if (doBlkCnt >= 255)
        Wr1SyntaxError(ERR27); /* LIMIT EXCEEDED: NUMBER OF do BLOCKS */
    else
        doBlkCnt++;

    curScope = (curScope & 0xff00) | doBlkCnt;
    PushBlock(curScope); // push new scope
    Wr1Byte(T1_SCOPE);   // and write to lex stream
    Wr1Word(curScope);
}

/*
 * State13 – parse the <ending> (END …;) of a DO block.
 *
 * Calls WrLabelDefs() for any labels on the END, pops the block scope via
 * PopBlock(), recovers the DO's label symbol via PopStateWord(), optionally
 * matches an identifier after END and raises ERR20 on mismatch.  Writes
 * T1_END to the lexical stream then calls ExpectSemiColon().
 */
static void State13() {
    sym_t *labelPtr;

    WrLabelDefs();                      // write labels
    PopBlock();                         // restore scope to parent block
    labelPtr = symtab + PopStateWord(); // get the do block label
    if (YylexMatch(L_IDENTIFIER))       // if we have "end identifier" do they match
        if (curSym != labelPtr)
            Wr1TokenErrorAt(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
    Wr1Byte(T1_END);                // write lex END token
    ExpectSemiColon();
}

/*
 * State14 – parse the case-unit list and DO CASE ending.
 *
 * Calls ParseStartStmt() to read the next statement.
 * For each non-END statement:
 *   • Reserves a new local label (T1_CASELABEL + index) as the entry point
 *     of this case arm.
 *   • Pushes state 15 (emit end-of-case jump) and state 11 (parse the unit).
 * On S_END:
 *   • Pops the end-of-case label index.
 *   • Calls WrLabelDefs() for labels on the END.
 *   • If labels are present, emits T1_JMP to the end-of-case target.
 *   • Calls PopBlock() and recovers the DO's label for identifier matching.
 *   • Writes T1_END and T1_LOCALLABEL + endCase to close the case structure.
 *   • Validates an optional end-label and calls ExpectSemiColon().
 */
static void State14() { /* process CASE statements */
    sym_t *labelPtr;

    ParseStartStmt();
    if (stmtStartCode != S_END) { // not at end of do case
        NewLocalLabel();          // reserve space for this case target
        Wr1Byte(T1_CASELABEL);    // and add to the lex stream
        Wr1Word(localLabelCnt);
        PushStateWord(15); // write jmp to end of case & re-enter state14
        PushStateWord(11); // parse <unit>
    } else {
        uint16_t endCase = PopStateWord(); /* get the label index for end of case target */
        WrLabelDefs();                     // write any labels associated with the end
        if (stmtLabelCnt != 0) {           // write jmp to end target if there are labels
            Wr1Byte(T1_JMP);
            Wr1Word(endCase);
        }
        PopBlock();                         // restore scope
        labelPtr = symtab + PopStateWord(); // get any prefix label to do
        Wr1Byte(T1_END);                    // write end
        Wr1Byte(T1_LOCALLABEL);             // & define target label
        Wr1Word(endCase);
        /* check end label if (present */
        if (YylexMatch(L_IDENTIFIER))
            if (curSym != labelPtr)
                Wr1SyntaxError(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
        ExpectSemiColon();
    }
}

/*
 * State15 – emit a jump to the end of the DO CASE and re-enter State14.
 *
 * Pops the end-of-case label index, writes T1_JMP + index, re-saves the
 * index on the stack and pushes state 14 to parse the next case arm.
 */
static void State15() {
    uint16_t caseEnd = PopStateWord(); // get index of end of case label
    Wr1Byte(T1_JMP);                   // write jmp & target
    Wr1Word(caseEnd);
    PushStateWord(caseEnd); // resave
    PushStateWord(14);      // parse case <unit>... <ending>
}

/*
 * State16 – parse the <conditional clause> (IF … THEN …).
 *
 * Calls WrLabelDefs() for labels on the IF statement, writes T1_IF, then
 * calls ParseExpresion(L_THEN) to emit the condition.
 * If THEN is missing, ERR28 is raised and a dummy T1_JMPFALSE 0 is written.
 * Otherwise:
 *   • A new local label is reserved for the end of the true branch.
 *   • The label index is pushed (for State17 to recover).
 *   • T1_JMPFALSE + index is written.
 *   • States 17 / 11 / 10 are pushed to parse the true unit and then
 *     handle the optional ELSE.
 */
static void State16() {
    WrLabelDefs();  // write any prefix labels
    Wr1Byte(T1_IF); // convert <if condition> to lex stream
    ParseExpresion(L_THEN);
    if (YylexNotMatch(L_THEN)) { // oops no THEN
        Wr1TokenErrorAt(ERR28);  /* MISSING 'THEN' */
        Wr1Byte(T1_JMPFALSE);    // write jmpfalse 0 to lex stream
        Wr1Word(0);
    } else {
        NewLocalLabel();              // end of <true unit> target label
        PushStateWord(localLabelCnt); // save on stack
        Wr1Byte(T1_JMPFALSE);         // write jmp for false to lex stream
        Wr1Word(localLabelCnt);
        PushStateWord(17); // parse optional ELSE <false element>
        PushStateWord(11); // parse <unit> (<true unit>)
        PushStateWord(10); // get the statement start
    }
}

/*
 * State17 – parse optional ELSE <false element>.
 *
 * Pops the end-of-true-unit label index.  Suppresses line-info emission
 * while emitting T1_LOCALLABEL (the branch target that marks the end of the
 * true branch) so listing output is not perturbed.
 *
 * If the next token is L_ELSE:
 *   • A new local label is reserved for the end of the false element.
 *   • T1_JMP + index is written to jump around the false element.
 *   • States 18 / 11 / 10 are pushed to parse the false element and
 *     define its end label.
 * Otherwise yyAgain is re-armed to re-process the non-ELSE token.
 * T1_LOCALLABEL is always written to define the end-of-true-branch target.
 */
static void State17() { /* process optional else */

    uint16_t endTrueLabel = PopStateWord(); /* labelref for end of <true unit> */
    Yylex();
    bool savToWrite = lineInfoToWrite; /* supress line info for labeldefs etc */
    lineInfoToWrite = false;
    if (tokenType == L_ELSE) { // ELSE <false element>
        NewLocalLabel();       // reserve space for end of <false element>
        Wr1Byte(T1_JMP);       // write jmp around <false element> to lex stream
        Wr1Word(localLabelCnt);
        PushStateWord(localLabelCnt); /* save labelref for end of <false element> */
        PushStateWord(18);            // parse end of <false element>
        PushStateWord(11);            // parse <unit> (<false element>)
        PushStateWord(10);            // get the statement start
    } else
        SetYyAgain();

    Wr1Byte(T1_LOCALLABEL); /* emit label for end <true unit> */
    Wr1Word(endTrueLabel);
    lineInfoToWrite = savToWrite; // restore status of lineInfoToWrite
}

/*
 * State18 – define the end label of the ELSE <false element>.
 *
 * Pops the false-element end label index, suppresses line-info emission,
 * writes T1_LOCALLABEL + index to close the false branch, then restores
 * lineInfoToWrite.
 */
static void State18() { /* end of else */

    bool savToWrite     = lineInfoToWrite; /* supress line info for labeldefs */

    lineInfoToWrite     = false;
    uint16_t falseLabel = PopStateWord(); /* labelref for end <false element> */
    Wr1Byte(T1_LOCALLABEL);               /* emit label */
    Wr1Word(falseLabel);
    lineInfoToWrite = savToWrite; // restore status of lineInfoTo
}

/*
 * ParseProgram – top-level entry point for the PL/M-80 structural parser.
 *
 * Initialises the parse stack, calls GNxtCh() to prime the lexer, seeds the
 * stack with state 0 (module header), then loops: pops the top state,
 * dispatches to its handler function and repeats until the stack is empty.
 *
 * The full grammar is implemented across states 0–21:
 *
 *  State  Handler       Grammar element
 *  -----  -------       ---------------
 *    0    State0        module header "name : DO ;"
 *    1    State1        check for module-level unit / END
 *    2    State2        module-level unit list
 *    3    State3        get statement start + <declaration>
 *    4    State4        PROCEDURE statement header
 *    5    State5        EXTERNAL procedure body (DECLARE only)
 *    6    State6        defined procedure ending
 *    7    State7        begin skip of bad nested block
 *    8    State8        skip nested blocks to matching END
 *    9    State9        <unit> list
 *   10    State10       get statement start token
 *   11    State11       single <unit>
 *   12    State12       DO block body (all variants)
 *   13    State13       DO block ending
 *   14    State14       DO CASE arm list and ending
 *   15    State15       emit end-of-case jump and re-enter State14
 *   16    State16       IF … THEN
 *   17    State17       optional ELSE
 *   18    State18       end of ELSE
 *   19    State19       <declaration> (DECLARE / PROCEDURE)
 *   20    State20       out-of-place declaration recovery
 *   21    State21       non-null procedure body check
 */
void ParseProgram(void) { // core state machine to parse program
    stateSP = 0;
    endSeen = false;
    GNxtCh();         // get the first char
    PushStateWord(0); // start with parse <module name> : 'DO' ;
    while (stateSP) {
        curState = stateStack[stateSP--]; // pop state
        switch (curState) {
        case 0:
            State0();
            break; // parse <module name> : 'DO' ;
        case 1:
            State1();
            break; // check for module level <unit>
        case 2:
            State2();
            break; // parse module level <unit>...
        case 3:
            State3();
            break; // get start statement and parse <declaration>
        case 4:
            State4();
            break; // parse start <procedure statement>
        case 5:
            State5();
            break; // parse <declaration> <ending> for external procedure
        case 6:
            State6();
            break; // parse <ending> for defined procedure
        case 7:
            State7();
            break; // states 7 & 8 skip to end of block, handling nested blocks
        case 8:
            State8();
            break; // states 7 & 8 skip to end of block, handling nested blocks
        case 9:
            State9();
            break; // parse <unit>...
        case 10:
            State10();
            break; // get the statement start
        case 11:
            State11();
            break; // parse <unit>
        case 12:
            State12();
            break; // parse <do block> // do seen
        case 13:
            State13();
            break; // parse <ending>  labels already collected
        case 14:
            State14();
            break; // parse case <unit>... <ending>
        case 15:
            State15();
            break; // write jmp to end of case and re-enter state14
        case 16:
            State16();
            break; // parse <conditional clause> if seen
        case 17:
            State17();
            break; // parse optional ELSE <false element>
        case 18:
            State18();
            break; // parse end of <false element>
        case 19:
            State19();
            break; // parse <declaration>
        case 20:
            State20();
            break; // parse unexpected <declaration>
        case 21:
            State21();
            break; // check for non null <unit>
        }
    }
}