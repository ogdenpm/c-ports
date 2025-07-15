/****************************************************************************
 *  plm0f.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

word curState;
bool endSeen;

static void SkipToSemiColon() {
    while (tokenType != L_SEMICOLON)
        Yylex();
    yyAgain = false;
}

static void ExpectSemiColon() {
    if (YylexNotMatch(L_SEMICOLON)) {
        Wr1TokenErrorAt(ERR32); /* INVALID SYNTAX, TEXT IGNORED UNTIL ';' */
        SkipToSemiColon();
    }
}

static void ErrorSkip() {
    SkipToSemiColon();
    SetYyAgain();
}

static word PopStateWord() {
    if (stateSP == 0)
        Lexfatal(ERR97); /* COMPILER Error: PARSE STACK UNDERFLOW */
    return stateStack[stateSP--];
}

// make sure that there will be space for case label info
static void NewLocalLabel() {
    localLabelCnt++;
}

/*
    collect labels (max 9)
    looks for Variable ':'
    sybmol references stored in stmtLabels array
    count is in stmtLabelCnt
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

static byte startLexCodeMap[] = { T1_STATEMENT, T1_SEMICOLON, T1_CALL,      T1_LINEINFO, T1_DISABLE,
                                  T1_DO,        T1_ENABLE,    T1_END,       T1_GO,       T1_GOTO,
                                  T1_HALT,      T1_IF,        T1_PROCEDURE, T1_RETURN };

/* parse start of statement
    startLexCode   startStmtCode
    T1_STATEMENT         0           assignment statement
    T1_SEMICOLON         1           null statement
    T1_CALL              2           call statement
    T1_LINEINFO          3           declare statement
    T1_DISABLE           4           disable statement (8080 dependent statement)
    T1_DO                5           do* statement
    T1_ENABLE            6           enable statement (8080 dependent statement)
    T1_END               7           end statement
    T1_GO                8           goto statement
    T1_GOTO              9           goto statement
    T1_HALT              10          halt statement (8080 dependent statement)
    T1_IF                11          conditional clause
    T1_PROCEDURE         12          procedure statement
    T1_RETURN            13          return statement
*/
/// <summary>
/// Collects labels before a statement and returns the start of statement token
/// </summary>
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

static void WrLabelDefs() {

    sym_t *tmp = curSym; // save to restore at end
    if (stmtLabelCnt != 0) {
        for (word i = 1; i <= stmtLabelCnt; i++) { // check each label
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

// check for end module
static bool IsEndOfModule() {
    if (YylexMatch(L_IDENTIFIER) && procInfo[1]->sym != curSym)
        Wr1TokenErrorAt(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
    ExpectSemiColon();
    if (afterEOF) {     // END after EOF i.e. recovery mode
        Wr1Byte(T1_END); // write closing END to lex stream
        return true;    // no more processing
    } else {
        Yylex();
        if (afterEOF) {            // was final end
            Wr1Byte(T1_END);        // write the END to lex stream
            afterEOF = false;      // no need to add recovery tokens
            return true;           // no more processing
        } else {                   // report error and keep going
            Wr1SyntaxError(ERR93); /* MISSING 'DO' FOR 'END' , 'END' IGNORED */
            SetYyAgain();
            return false;
        }
    }
} /* IsEndOfModule() */

// parse <ending> after labels
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

static void PushStateWord(word state) {
    if (stateSP != 99)
        stateStack[++stateSP] = state;
    else
        Lexfatal(ERR31); /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
}

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

//// <summary>
/// parse <module name> : 'DO' ;
/// if module name or 'DO' missing default to the name "MODULE"
/// </summary>
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

// check for module level <unit>
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

// parse module level <unit>...
static void State2() {
    if (stmtStartCode != S_END) {
        PushStateWord(2);  // parse module level <unit>...
        PushStateWord(10); // get the statement start
        PushStateWord(11); // parse <unit>
    } else {
        WrLabelDefs(); // write any END label prefixes
        if (IsEndOfModule())
            Wr1Byte(T1_HALT); // write the final halt
        else {
            PushStateWord(2);  // parse module level <unit>...
            PushStateWord(10); // get the statement start
        }
    }
} /* State2() */

// get start statement and parse <declaration>
static void State3() {
    ParseStartStmt();  // get statement start
    PushStateWord(19); // next state parse <declaration>
}

/* parse <declaration> */
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

/* parse start <procedure statement> */
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

// parse <declaration> <ending> for external procedure
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

// check for non null <unit>
static void State21() {
    if (stmtStartCode == S_END)
        Wr1TokenErrorAt(ERR174); /* INVALID NULL procedure */
    else
        PushStateWord(9); // parse <unit>
}

// parse <ending> for defined procedure
static void State6() {
    WrLabelDefs(); // write any labels on the END to the lex stream
    ProcEnding();
    Wr1Byte(T1_END); // add end to the lexical stream
}

/* states 7 & 8 skip to end of block, handling nested blocks */
static void State7() {
    SkipToSemiColon(); // skip this statement
    PushStateWord(8);  // skip to end of proc / do
}

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

// parse <unit>...
static void State9() {
    if (stmtStartCode != S_END) {
        PushStateWord(9);  // repeat parsing statement after this statement is parsed
        PushStateWord(10); // get start statement after processing
        PushStateWord(11); // parse <unit>
    }
}

// get the statement start
static void State10() {
    ParseStartStmt();
}

// parse <unit>
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
                Wr1Byte(T1_IDENTIFIER);          // write var for <assignment statement>
                Wr1Word((uint16_t)(stmtStartSymbol - symtab));
            }
            ParseExpresion(L_SEMICOLON);
        }
        ExpectSemiColon();
    }
}

// parse unexpected <declaration>
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

// parse <do block> -- do already seen
static void State12() {
    WrLabelDefs(); // write any prefix labels
    if (stmtLabelCnt != 0)
        PushStateWord(
            (uint16_t)(stmtLabels[stmtLabelCnt] - symtab)); // push the address of the last label
    else
        PushStateWord(0); // or null if none
    if (YylexMatch(L_IDENTIFIER)) {
        Wr1Byte(T1_DOLOOP); // convert <iterative do statement> to lex format
        Wr1LexToken();     // the <index variable>
        ParseExpresion(L_SEMICOLON);
        PushStateWord(13);            // parse <ending>  labels already collected
        PushStateWord(9);             // parse <unit>...
        PushStateWord(10);            // get start statement
    } else if (YylexMatch(L_WHILE)) { // <do-while block>
        Wr1Byte(T1_WHILE);             // convert <do-while statement> to lex format
        ParseExpresion(L_SEMICOLON);
        PushStateWord(13);           // parse <ending>  labels already collected
        PushStateWord(9);            // parse <unit>...
        PushStateWord(10);           // get start statement
    } else if (YylexMatch(L_CASE)) { // <do-case block>
        Wr1Byte(T1_CASE);             // convert <do-case statement> to lex format
        ParseExpresion(L_SEMICOLON);
        NewLocalLabel(); // make sure later passes have space for the end of case target
        PushStateWord(localLabelCnt); // push the index of end of case target for later
        PushStateWord(14);            // parse case <unit>... <ending>
    } else {
        Wr1Byte(T1_DO);     // convert <simple do statement> to lex format
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
    Wr1Byte(T1_SCOPE);    // and write to lex stream
    Wr1Word(curScope);
}

// parse <ending>  labels already collected
static void State13() {
    sym_t *labelPtr;

    WrLabelDefs();                      // write labels
    PopBlock();                         // restore scope to parent block
    labelPtr = symtab + PopStateWord(); // get the do block label
    if (YylexMatch(L_IDENTIFIER))       // if we have "end identifier" do they match
        if (curSym != labelPtr)
            Wr1TokenErrorAt(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
    Wr1Byte(T1_END);                 // write lex END token
    ExpectSemiColon();
}

// parse case <unit>... <ending>
static void State14() { /* process CASE statements */
    sym_t *labelPtr;

    ParseStartStmt();
    if (stmtStartCode != S_END) { // not at end of do case
        NewLocalLabel();          // reserve space for this case target
        Wr1Byte(T1_CASELABEL);     // and add to the lex stream
        Wr1Word(localLabelCnt);
        PushStateWord(15); // write jmp to end of case & re-enter state14
        PushStateWord(11); // parse <unit>
    } else {
        word endCase = PopStateWord(); /* get the label index for end of case target */
        WrLabelDefs();                 // write any labels associated with the end
        if (stmtLabelCnt != 0) {       // write jmp to end target if there are labels
            Wr1Byte(T1_JMP);
            Wr1Word(endCase);
        }
        PopBlock();                         // restore scope
        labelPtr = symtab + PopStateWord(); // get any prefix label to do
        Wr1Byte(T1_END);                     // write end
        Wr1Byte(T1_LOCALLABEL);              // & define target label
        Wr1Word(endCase);
        /* check end label if (present */
        if (YylexMatch(L_IDENTIFIER))
            if (curSym != labelPtr)
                Wr1SyntaxError(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
        ExpectSemiColon();
    }
}

// write jmp to end of case and re-enter state14
static void State15() {
    word caseEnd = PopStateWord(); // get index of end of case label
    Wr1Byte(T1_JMP);                // write jmp & target
    Wr1Word(caseEnd);
    PushStateWord(caseEnd); // resave
    PushStateWord(14);      // parse case <unit>... <ending>
}

// parse <conditional clause> if seen
static void State16() {
    WrLabelDefs(); // write any prefix labels
    Wr1Byte(T1_IF); // convert <if condition> to lex stream
    ParseExpresion(L_THEN);
    if (YylexNotMatch(L_THEN)) { // oops no THEN
        Wr1TokenErrorAt(ERR28);  /* MISSING 'THEN' */
        Wr1Byte(T1_JMPFALSE);     // write jmpfalse 0 to lex stream
        Wr1Word(0);
    } else {
        NewLocalLabel();              // end of <true unit> target label
        PushStateWord(localLabelCnt); // save on stack
        Wr1Byte(T1_JMPFALSE);          // write jmp for false to lex stream
        Wr1Word(localLabelCnt);
        PushStateWord(17); // parse optional ELSE <false element>
        PushStateWord(11); // parse <unit> (<true unit>)
        PushStateWord(10); // get the statement start
    }
}

// parse optional ELSE <false element>
static void State17() { /* process optional else */

    word endTrueLabel = PopStateWord(); /* labelref for end of <true unit> */
    Yylex();
    bool savToWrite = lineInfoToWrite; /* supress line info for labeldefs etc */
    lineInfoToWrite = false;
    if (tokenType == L_ELSE) { // ELSE <false element>
        NewLocalLabel();       // reserve space for end of <false element>
        Wr1Byte(T1_JMP);        // write jmp around <false element> to lex stream
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

// parse end of <false element>
static void State18() { /* end of else */

    bool savToWrite = lineInfoToWrite; /* supress line info for labeldefs */

    lineInfoToWrite = false;
    word falseLabel = PopStateWord(); /* labelref for end <false element> */
    Wr1Byte(T1_LOCALLABEL);            /* emit label */
    Wr1Word(falseLabel);
    lineInfoToWrite = savToWrite; // restore status of lineInfoTo
}

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
