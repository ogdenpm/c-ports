/****************************************************************************
 * mac.c - Macro Assembler for Intel 8080/8085
 *
 * This is a macro assembler that processes assembly language source files,
 * handles macro definitions and expansions, evaluates expressions, and
 * generates Intel HEX output files.
 *
 * Main Features:
 * - Two-pass assembly process
 * - Macro definition and expansion with parameters
 * - Symbol table management with hashing
 * - Expression evaluation
 * - Intel HEX file generation
 * - Conditional assembly (IF/ENDIF)
 * - Repeat blocks (REPT, IRP, IRPC)
 * - Library file support
 ****************************************************************************/

#include "mac.h"
#include "utility.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Loop control states
#define STMTLOOP 1 // Continue processing statements
#define LINELOOP 2 // Continue processing lines
#define PASSLOOP 3 // Start next pass
#define FINISH   4 // Assembly complete

// Global assembler state variables
uint8_t l11c2;  // Parameter length counter
uint16_t label; // Current label symbol reference
uint16_t l11c5;
uint16_t XFerAdr; // Transfer/start address for program
uint8_t outCol;   // Current output column position
uint8_t l11ca;
uint8_t ifNesting;       // Current IF nesting depth
uint8_t macroNesting;    // Current macro nesting depth
uint8_t ifLevel;         // IF stack level
uint8_t ifStack[IF_Dep]; // Stack for nested IF conditions
uint16_t macroChain;     // Linked list of macro definitions
uint16_t l11d8;
uint8_t supp_0;      // Leading zero suppression flag
uint16_t val_16;     // Current 16-bit value being processed
uint16_t localCnt;   // Local label counter
uint16_t afterComma; // Position after comma in parameter list

// Character handling variables
uint8_t prevCh;  // Previous character
uint8_t ValBase; // Numeric base for value conversion (2, 8, 10, 16)
uint16_t l1613;
uint8_t chevronCnt; // Angle bracket nesting counter

uint8_t F_CHR; // Format character

/****************************************************************************
 * Symbol Table Structure
 *
 * Each symbol entry consists of:
 * - link: Pointer to next symbol in hash chain
 * - typeSize: Upper nibble = type, lower nibble = name length
 * - name: Variable length symbol name
 * - val: 16-bit value (stored after name bytes)
 ****************************************************************************/
typedef struct {
    uint16_t link;    // Link to next symbol in hash chain
    uint8_t typeSize; // Type (high nibble) and size (low nibble)
    uint8_t name[1];  // Start of variable-length name
    // Rest of structure:
    // uint8_t name[n-1];   // Remaining name bytes
    // uint16_t val;        // 16-bit symbol value
} sym_t;

uint8_t _1_opt; // Option flag

uint16_t macHashTab[SLen]; // Hash table for macro symbols
uint8_t Balance;           // Nesting balance counter

/****************************************************************************
 * Stack Structure for Macro Expansion and Collection Modes
 *
 * Maintains state when entering/exiting macros, repeat blocks, etc.
 ****************************************************************************/
struct {
    uint8_t collectMode; // Collection mode: 0=none, 1=macro, 2+=rept/irp/irpc
    uint16_t topPtr;     // Top of memory pointer
    uint16_t l2ed4;
    uint16_t chPtr;      // Character pointer
    uint8_t curCh;       // Current character
    uint16_t prevTopPtr; // Previous top pointer
    uint8_t l2f44;       // (unused)
    uint8_t ifLevel;     // IF nesting level at this stack level
} Stack[16];

// Symbol sorting workspace (overlapped data area)
uint8_t l2ea4;
uint8_t l2ea6;
uint8_t l2ea7;
uint16_t symList[28]; // List of symbols for sorting

uint8_t expandChIdx; // Character index during expansion
uint8_t lookAhead;   // Look-ahead character

/****************************************************************************
 * Print Line Buffer
 * Holds formatted output line for listing file
 ****************************************************************************/
struct {
    uint8_t len;
    uint8_t str[37 + 1]; // Buffer + null terminator
} PrnLine;

char OutLine[120]; // Output line buffer

// Token and expression variables
uint8_t OutLen;  // Output line length
uint8_t tokType; // Token type: _id, _number, _string, _punct, _error
uint16_t Value;  // Current numeric value

/****************************************************************************
 * Token Buffer
 * Stores the current token being processed
 ****************************************************************************/
struct {
    uint8_t len;
    char str[ArgMax + 1]; // Token string + null terminator
} token;

uint16_t ExprVal; // Expression evaluation result

// Symbol table pointers
uint16_t SymTop; // Top of symbol table
uint16_t topPtr; // Current top pointer for dynamic allocation
uint8_t PassNr;  // Current pass number (0 or 1)
uint16_t CurHEX; // Current hex address for output
uint16_t LocCtr; // Location counter (current assembly address)

uint16_t SymBot;  // Bottom of symbol table
uint16_t curSym;  // Current symbol being processed
uint16_t symBase; // Base address of symbol data

uint8_t collectDepth;    // Depth of macro body collection
uint8_t curCh;           // Current character being processed
uint8_t collectMode;     // Current collection mode
bool InLIB;              // true if processing from library file
uint8_t S_opt = _SPENA;  // Symbol table listing option
uint8_t M_opt = _SPENA;  // Macro expansion listing option
uint16_t startMacroBody; // Start address of macro body
char const *Titleptr;    // Pointer to title string
uint8_t Q_opt;           // Quiet option
uint8_t L_opt;           // List option
uint8_t P_opt;           // Print option
uint8_t R_opt;           // Relocatable option
uint8_t expandTab;       // Tab expansion setting

mnemonic_t *pMnem; // Pointer to current mnemonic

// File handles and names
FILE *fpLIB;         // Library file pointer
char const *libFile; // Library file name
FILE *fpSRC;         // Source file pointer
char const *srcFile; // Source file name
FILE *fpPRN;         // Print/listing file pointer
char const *prnFile; // Print file name
FILE *fpHEX;         // HEX output file pointer
char const *hexFile; // HEX file name
char const *symFile; // Symbol file name

uint8_t mem[0x10000]; // 64K memory buffer for assembled code

/****************************************************************************
 * Help text displayed when invalid arguments provided
 ****************************************************************************/
char const help[] =
    "Usage: %s [options] sourceFile[.asm]\n"
    "Where options are\n"
    "  -p file[.prn]  override listing file. Default sourceFile.prn\n"
    "  -s file[.sym]  override symbol file. Default sourcefile.sym\n"
    "  -h file[.hex]  override hex file. Default sourcfile.hex\n"
    "  -c controls+   set mac control options\n"
    "  -t[=n]         expand tabs to every nth column. If n missing then 8 is used\n"
    "Notes:\n"
    " * file is the filename or - for standard output or . to disable output\n"
    "   If no extent is specified then the relevant extent is added.\n"
    " * controls are as per MAC (+|-|*|x)(1|L|M|Q|R|S). lowercase letters are also supported\n"
    "   Due to OS shell processing, controls may need to be quoted to allow * and spaces\n"
    "   Note x is an alias for *. If quotes are not used this will avoid shell wild card "
    "   expansion\n";

/****************************************************************************
 * main() - Program entry point
 *
 * Performs two-pass assembly:
 * Pass 0: Build symbol table, collect macro definitions
 * Pass 1: Generate code and listing output
 ****************************************************************************/
int main(int argc, char **argv) {
    PassNr       = 0;
    collectDepth = 0;
    IniMAC(argc, argv); // Initialize assembler, parse command line
    IniField_1();       // Initialize field 1 processing
    macroChain  = 0;

    uint8_t ret = PASSLOOP;
    while (ret != FINISH) {
        IniField_2(); // Initialize field 2 processing
        Balance              = 0;
        Stack[0].collectMode = 0;
        Stack[0].prevTopPtr  = topPtr;
        l17f1();
        IniPass(); // Initialize for current pass
        localCnt = label = 0;
        // Initialize addresses: use TPA if relocatable, else 0
        XFerAdr = LocCtr = CurHEX = R_opt ? TPA : 0;
        ifLevel                   = 0;

        // Main assembly loop
        do {
            GetToken();
        } while ((ret = MAC_loop()) == LINELOOP);
    }
    finish(); // Clean up and close files
    exit(0);
}

/****************************************************************************
 * MAC_loop() - Main statement processing loop
 *
 * Returns: STMTLOOP to continue, LINELOOP for next line, or FINISH when done
 ****************************************************************************/
uint8_t MAC_loop() {
    while (1) {
        uint8_t ret;
        // Skip numeric tokens (line numbers)
        if (tokType == _number)
            return LINELOOP;
        if ((ret = MAC_sub()) != STMTLOOP)
            return ret;
    }
}

/****************************************************************************
 * l02d6() - Process macro parameters
 *
 * Parses and stores macro parameters, handling:
 * - Parameter substitution (%)
 * - Comma-separated parameters
 * - Proper nesting and termination
 ****************************************************************************/
uint8_t l02d6() {
    l11c2               = getParamLen(); // Get parameter count
    uint16_t saveTopPtr = topPtr;

    if (l11c2 != 0) {
        while (1) {
            uint8_t state = 0;
            uint8_t a;

            if (tokType == _punct) {
                a = token.str[0];
                // Check for end of instruction
                if (testEndInst(a))
                    break;

                // Handle parameter substitution (%)
                if (a == '%') {
                    val_16 = getWordValue();
                    if (tokType != _punct) {
                        SetErr('S');
                        break;
                    }
                    a         = token.str[0];
                    token.len = sprintf(token.str, "%d", val_16);
                    l03db();
                    state = 1;
                }
                // Handle comma separator
                else if (token.str[0] == ',') {
                    l03d7();
                    state = 2;
                }
            }

            if (state == 0) {
                l03db();
                GetToken();
                if (tokType != _punct) {
                    SetErr('S');
                    break;
                }
                a = token.str[0];
            }

            // Check for comma or end
            if (state <= 1 && a != ',') {
                if (!testEndInst(a))
                    SetErr('S');
                break;
            }

            if (l11c2 == 0)
                break;
            l1afc();
        }
    }

    // Clear remaining parameters
    while (l11c2 != 0)
        l03d7();
    uint16_t tmp = symBase + 1;

    // Skip to end of statement
    while (!testEndStmt(curCh))
        GetToken();

    curCh          = 0;
    Stack[0].curCh = 0;
    NewPage();
    Stack[0].ifLevel = ifLevel;
    nest(); // Push stack level
    Stack[0].chPtr       = tmp;
    Stack[0].prevTopPtr  = saveTopPtr;
    Stack[0].curCh       = 0;
    Stack[0].collectMode = 1;
    return LINELOOP;
}

/****************************************************************************
 * StoreSymbol() - Save current token to symbol table
 *
 * Stores token string into symbol table memory with length prefix
 ****************************************************************************/
void StoreSymbol() {
    uint16_t tmp = SymTop;
    symBase      = SymTop;

    // Copy token characters to symbol table
    for (uint8_t i = 0; i < token.len; i++)
        PutSym(token.str[i]);

    // Store length byte at start
    BYTE(SymTop = tmp) = token.len;
}

/****************************************************************************
 * LoadSymbol() - Load symbol from symbol table into token buffer
 *
 * Retrieves symbol name from symbol table at SymTop position
 ****************************************************************************/
void LoadSymbol() {
    token.len = BYTE(SymTop);
    symBase   = SymTop;

    for (uint8_t i = 0; i < token.len; i++)
        token.str[i] = SymbolByte();
}

/****************************************************************************
 * testMacro() - Check if current token is a MACRO directive
 *
 * Returns: true if token is the MACRO pseudo-op
 ****************************************************************************/
bool testMacro() {
    return tokType == _error && (pMnem = GetMnemo(token.str, token.len)) &&
           pMnem->type == _pseudo && pMnem->code == _macro;
}

/****************************************************************************
 * testEndInst() - Test if character ends an instruction
 *
 * Checks for: semicolon (;), carriage return (\r), or statement end
 * Returns: true if character terminates instruction
 ****************************************************************************/
bool testEndInst(uint8_t ch) {
    return ch == ';' || ch == '\r' || testEndStmt(ch);
}

/****************************************************************************
 * testEndStmt() - Test if character ends a statement
 *
 * Checks for: newline (\n), EOF marker, or exclamation (!)
 * Returns: true if character terminates statement
 ****************************************************************************/
bool testEndStmt(uint8_t ch) {
    return ch == '\n' || ch == cpmEOF || ch == '!';
}

/****************************************************************************
 * NewPage() - Start new page in listing output
 *
 * Emits current line with balance indicator if nested, resets output
 ****************************************************************************/
void NewPage() {
    // Mark line with '+' if in nested structure
    if (Balance)
        OutLine[5] = '+';
    emitLine();
    OutLen = _ASCbyt;
}

/****************************************************************************
 * l03c1() - Unnest to balance point or error
 *
 * Unwinds stack until balanced, errors if not in collectable context
 ****************************************************************************/
void l03c1() {
    while (Balance) {
        // Can't unnest from these collection modes
        if (Stack[0].collectMode >= _irpc_ || Stack[0].collectMode == _macro_)
            return;
        unnest();
    }
    SetErr('B'); // Balance error
}

/****************************************************************************
 * l03d7() - Clear token and process
 *
 * Empties token buffer and calls l03db()
 ****************************************************************************/
void l03d7() {
    token.len = 0;
    l03db();
}

/****************************************************************************
 * l03db() - Token processing helper
 *
 * Calls token processing functions in sequence
 ****************************************************************************/
void l03db() {
    l1f87();
    GetSymbol();
    l1fa5();
    l11c2--; // Decrement parameter counter
}

uint8_t MAC_sub() {
    uint8_t cval;
    if (tokType == _punct) {
        if (token.str[0] == '$') {
            if (hasLabel())
                return EndLine('S');
            if (curCh == '-')
                cval = _DISA;
            else if (curCh == '+')
                cval = _ENA;
            else if (curCh == '*')
                cval = _PART;
            else
                return EndLine('S');
            GetToken();
            if (curCh == 'M')
                M_opt = cval;
            else if (curCh == 'P')
                P_opt = cval;
            else
                return EndLine('S');
            GetToken();
        }
        return EndLine(0);
    } else if (tokType != _id)
        return EndLine('S');
    if ((pMnem = GetMnemo(token.str, token.len)))
        return MAC_Mnemo();
    if (!SymSrc()) { // new symbol
        PostSym();
        if (PassNr)
            SetErr('P');                  // e39
    } else if (GetSymType() == T_MACRO) { // 3e9
        uint16_t l11e0 = 0;
        if (PassNr) {
            while (GetSymVal() > curSym) { // 23c
                l11e0 = curSym;
                lookup((pstr_t *)&token, WORD(curSym));
                if (curSym) {
                    if (GetSymType() != T_MACRO)
                        return EndLine('P');
                } else {
                    l1afc();
                    if (!testMacro() || l11e0 != l11d8)
                        return EndLine('P');
                    label = l11d8;
                    return macro();
                }
            }
        }
        StoreSymbol();
        l1afc();
        if (!testMacro()) {
            uint16_t tmp = curSym; // l02c1
            CheckLabel(LocCtr);
            if (M_opt == _DISA)
                StWord(LocCtr);
            curSym = tmp;
            return l02d6();
        }
        if (label)
            SetErr('S');
        if (PassNr) {
            if (l11e0 != l11d8)
                return EndLine('P');
            label = l11d8;
        } else {
            LoadSymbol();
            GetHashIdx((pstr_t *)&token);
            PostSym();
            label = curSym;
        }
        return macro();
    }
    if (label)
        SetErr('L');
    label = curSym;
    GetToken();
    if (tokType != _punct || token.str[0] != ':')
        return STMTLOOP;
    return LINELOOP;
}

bool IsMacro(uint8_t type) {
    return type == _macro || type == _rept || type == _irp || type == _irpc;
}

bool collectMacroBody(uint8_t mode) {
    collectMode = mode;
    while (1) {
        if (tokType == _punct) {
            if ((token.str[0] == '\r' || token.str[0] == '!'))
                break;
            else if (token.str[0] == cpmEOF)
                return false;
        }
        SetErr('S');
        GetToken();
    }

    startMacroBody = symBase;
    collectDepth   = 1;
    GetToken();

    while (1) {
        uint16_t tokStart = symBase;
        GetToken();
        if (tokType == _punct && token.str[0] == cpmEOF)
            return false;
        if (tokType == _id && (pMnem = GetMnemo(token.str, token.len))) {
            if (collectMode == _macro_ && PassNr == 0 && token.len != 1 && pMnem->tokId) {
                symBase = tokStart;
                PutSym(pMnem->tokId | 0x80); // replace token with its id
                if (curCh)                   // append prepread character
                    PutSym(curCh);
            }
            if (pMnem->type == _pseudo) {
                if (IsMacro(pMnem->code)) {
                    if (collectDepth++ == 0xff)
                        return false;
                } else if (pMnem->code == _endm && --collectDepth == 0)
                    break;
            }
        }
    }
    if (collectMode == _macro_) {
        label = 0;
        if (InLIB)
            PutSymVal(0);
        else {
            PutSymVal(macroChain);
            macroChain = curSym;
        }
        if (PassNr)
            return true;
    }
    if (BYTE(symBase) != '\r')
        SetErr('S');
    BYTE(symBase) = '\r';
    PutSym(0);
    return true;
}

void PutIF(uint8_t a) {
    if (ifLevel >= IF_Dep)
        SetErr('O');
    else
        ifStack[ifLevel++] = a;
}
uint8_t GetIF() {
    if (ifLevel == 0) {
        SetErr('B');
        return 0;
    } else
        return ifStack[--ifLevel];
}

void SET_EQU(uint8_t a) {
    uint16_t tmp = LocCtr;
    StWord(LocCtr = getWordValue());
    LocCtr     = tmp;
    OutLine[6] = a;
    return;
}

uint8_t macro() {
    if (!hasLabel()) {
        SetErr('L');
        return EndLine(0);
    }
    if (PassNr != 0) {
        if (curSym != l11d8)
            SetErr('P');
        else
            l11d8 = GetSymVal();
    } else
        SetSymType(T_MACRO);
    uint8_t paramterCnt = 0;
    if (PassNr == 0)
        setParameterCnt(0);
    while (GetToken(), tokType == _id) {
        if (PassNr == 0)
            addParameter();
        paramterCnt++;
        GetToken();
        if (tokType != _punct || token.str[0] != ',')
            break;
    }
    if (!collectMacroBody(_macro_))
        return EndFile(true);
    if (PassNr == 0)
        setParameterCnt(paramterCnt);
    GetToken();
    return EndLine(0);
}

/****************************************************************************
 * l0552() - Handle macro/repeat iteration
 *
 * Manages execution of REPT, IRP, and IRPC blocks:
 * - REPT: Repeat block N times
 * - IRP: Iterate over comma-separated parameters
 * - IRPC: Iterate over each character in string
 *
 * Returns: LINELOOP to process next line
 ****************************************************************************/
uint8_t l0552() {
    uint16_t ptr;

    // Handle ENDM mode (repeat counter)
    if (Stack[0].collectMode == _endm) {
        uint16_t hl = Stack[0].topPtr;
        if ((ptr = WORD(hl)) != 0) {
            WORD(hl)       = ptr - 1; // Decrement repeat counter
            OutLen         = _ASCbyt;
            Stack[0].chPtr = Stack[0].l2ed4;
            curCh          = 0;
            return LINELOOP;
        }
    } else {
        uint16_t hl = Stack[0].topPtr;
        ptr         = WORD(hl);

        // Check if not end of iteration
        if (BYTE(ptr) != '\r') {
            if (BYTE(ptr) != 0) {
                // Handle IRP (iterate over parameters)
                if (Stack[0].collectMode != _irpc_) {
                    uint8_t saveLookAhead = lookAhead;
                    lookAhead             = 0;
                    uint8_t saveCh        = curCh;
                    curCh                 = 0;
                    Stack[0].chPtr        = ptr;

                    // Handle comma-separated parameter
                    if (BYTE(ptr) == ',') {
                        ptr++;
                        token.len = 0;
                        l1f87();
                        l11ee(ptr);
                        PrnLine.len = 0;
                    } else {
                        // Parse parameter
                        l1afc();
                        l11e4(ptr);
                        l1f87();
                        ptr = Stack[0].chPtr;

                        if (BYTE(ptr) == 0)
                            BYTE(ptr) = '\r';
                        else {
                            ptr = afterComma;
                            GetToken();
                            if (token.str[0] != ',')
                                SetErr('S');
                            l11ee(ptr);
                            PrnLine.len = 0;
                        }
                    }
                    WORD(Stack[0].topPtr) = ptr;
                    curCh                 = saveCh;
                    lookAhead             = saveLookAhead;
                }
                // Handle IRPC (iterate over each character)
                else {
                    WORD(hl)     = ptr + 1; // Advance to next character
                    token.len    = 1;
                    token.str[0] = BYTE(ptr); // Get single character
                    l1f87();
                }
                symBase = Stack[0].topPtr + 1;
                GetSymbol();
                l1fa5();
            } else {
                BYTE(ptr) = '\r';
                token.len = 0;
                l1f87();
            }
            OutLen         = _ASCbyt;
            Stack[0].chPtr = Stack[0].l2ed4;
            curCh          = 0;
            return LINELOOP;
        }
    }

    // End of iteration - restore state and unnest
    NewPage();
    topPtr = Stack[0].prevTopPtr; // Restore top pointer
    unnest();
    ifLevel = Stack[0].ifLevel;
    if ((curCh = Stack[0].curCh))
        StChar(curCh);
    return LINELOOP;
}

/****************************************************************************
 * l0a7e() - Setup macro/repeat expansion
 *
 * Prepares stack for macro body expansion or repeat block execution
 * Returns: Result from l0552()
 ****************************************************************************/
uint8_t l0a7e() {
    NewPage();
    GetToken();
    Stack[0].ifLevel = ifLevel;
    Stack[0].curCh   = curCh != '\n' ? curCh : 0;
    nest(); // Push stack level
    Stack[0].prevTopPtr = topPtr;

    // Copy macro body to stack
    while (symBase >= l11c5)
        BYTE(--topPtr) = BYTE(symBase--);

    SymTop          = symBase + 1;
    Stack[0].topPtr = topPtr;

    // Initialize pointer based on collection mode
    if (collectMode != _rept_)
        WORD(topPtr) = topPtr + BYTE(topPtr);

    Stack[0].l2ed4       = topPtr + (curSym - l11c5);
    Stack[0].collectMode = collectMode;
    return l0552();
}

/****************************************************************************
 * _ENDM_() - Handle ENDM (end macro) directive
 *
 * Terminates macro expansion or repeat block
 * Returns: LINELOOP to continue processing
 ****************************************************************************/
uint8_t _ENDM_() {
    CheckLabel(LocCtr);
    l03c1();
    OutLine[5] = '+';

    // Different handling for macro vs repeat blocks
    if (Stack[0].collectMode < _irpc_) { // macro
        PopParam();
    } else {
        // Save and restore for nested ENDM
        uint16_t tmp        = Stack[0].prevTopPtr;
        Stack[0].prevTopPtr = Stack[0].topPtr;
        PopParam();
        Stack[0].prevTopPtr = tmp;

        // If processing ENDM within repeat, continue iteration
        if (pMnem->code == _endm)
            return l0552();
    }

    // Exit macro - restore state
    NewPage();
    topPtr = Stack[0].prevTopPtr; // Restore top pointer
    unnest();
    ifLevel = Stack[0].ifLevel;
    if ((curCh = Stack[0].curCh))
        StChar(curCh);
    return LINELOOP;
}

/****************************************************************************
 * __FALSE() - Skip lines during false IF condition
 *
 * Scans ahead to matching ELSE or ENDIF, tracking nested IF/MACRO blocks
 * Handles nested structures correctly by counting depth
 *
 * Parameters:
 *   l11ca: IF state (_if_ or _else_)
 * Returns: Result from _ENDM_() or EndLine()
 ****************************************************************************/
uint8_t __FALSE(uint8_t l11ca) {
    macroNesting = ifNesting = 0;

    while (1) {
        // Skip to end of line
        while (1) {
            if (tokType == _punct) {
                if (token.str[0] == '\r') {
                    GetToken(); // Consume '\n'
                    break;
                } else if (token.str[0] == '!')
                    break;
                else if (token.str[0] == cpmEOF)
                    return EndFile(true);
            }
            GetToken();
        }

        GetToken();

        // Skip line numbers
        if (tokType == _number)
            GetToken();

        if (tokType != _id)
            continue;

        // Check for directive
        if (!(pMnem = GetMnemo(token.str, token.len))) {
            // Check if it's a label definition
            GetToken();
            if (tokType == _punct) {
                if (token.str[0] != ':') // Not a label
                    continue;
                GetToken();
            }
            if (tokType != _id || !(pMnem = GetMnemo(token.str, token.len)))
                continue;
        }

        // Only process pseudo-ops
        if (pMnem->type != _pseudo)
            continue;

        // Handle nested IF blocks
        if (pMnem->code == _if) {
            if (ifNesting++ == 255) // Too deep
                SetErr('O');
            continue;
        }
        // Handle ELSE - switch from false to true
        else if (pMnem->code == _else) {
            if (ifNesting)
                continue;
            if (l11ca == _else_)
                SetErr('B');
            PutIF(_else_);
            GetToken();
        }
        // Handle ENDIF - end of IF block
        else if (pMnem->code == _endif) {
            if (ifNesting--)
                continue;
            if (macroNesting)
                SetErr('B');
            GetToken();
        }
        // Handle nested MACRO blocks
        else if (IsMacro(pMnem->code)) {
            if (macroNesting++ == 0xff)
                SetErr('O');
            continue;
        }
        // Handle ENDM
        else if (pMnem->code != _endm || macroNesting--)
            continue;
        else
            return _ENDM_();

        return EndLine(0);
    }
}

/****************************************************************************
 * MAC_Mnemo() - Process mnemonic or directive
 *
 * Main dispatcher for all assembly instructions and pseudo-ops.
 * Handles:
 * - Data definition directives (DB, DW, DS)
 * - Assembly control (ORG, END, EQU, SET)
 * - Conditional assembly (IF, ELSE, ENDIF)
 * - Macro directives (MACRO, ENDM, REPT, IRP, IRPC, LOCAL)
 * - 8080/8085 instructions
 *
 * Returns: LINELOOP, STMTLOOP, or FINISH status
 ****************************************************************************/
uint8_t MAC_Mnemo() {
    if (pMnem->type == _pseudo) {
        switch (pMnem->code) {
        case _db: // Define Byte
            CheckLabel(LocCtr);
            do {
                GetToken();
                // Handle string constants
                if (tokType == _string && token.len != 1) {
                    for (uint8_t i = 0; i < token.len; i++)
                        PutCode(token.str[i]);
                    GetToken();
                } else {
                    // Handle numeric expressions
                    Expression();
                    PutCode(CheckByteOper(ExprVal));
                }
                LocCtr = CurHEX;
            } while (haveComma());
            break;

        case _ds: // Define Storage
            CheckLabel(LocCtr);
            StWord(LocCtr);
            CurHEX = (LocCtr += getWordValue());
            break;

        case _dw: // Define Word
            CheckLabel(LocCtr);
            do {
                uint16_t hl = getWordValue();
                PutCode(hl & 0xff); // Low byte first
                PutCode(hl >> 8);   // High byte
                LocCtr = CurHEX;
            } while (haveComma());
            break;

        case _end: // End of Assembly
            CheckLabel(LocCtr);
            StWord(LocCtr);
            if (OutLine[0] != ' ')
                EndLine(0);
            uint16_t hl = getWordValue();
            if (OutLine[0] == ' ')
                XFerAdr = hl; // Set transfer address
            OutLine[0] = ' '; // Clear error
            if (ifLevel)
                SetErr('B'); // Unclosed IF
            GetToken();
            if (tokType != _punct || token.str[0] != '\n')
                return EndLine('S');
            return EndFile(Balance != 0);

        case _endif: // End IF
            CheckLabel(LocCtr);
            GetIF();
            GetToken();
            break;

        case _endm:  // End Macro
        case _exitm: // Exit Macro
            return _ENDM_();

        case _equ: // Equate
            if (!hasLabel())
                EndLine('S');
            SET_EQU('=');
            CheckLabel(ExprVal);
            break;

        case _if: // Conditional Assembly
            CheckLabel(LocCtr);
            uint16_t opr1 = getWordValue();
            if (OutLine[0] != ' ')
                return __FALSE(OutLine[0]);
            if (!(opr1 & 1)) // If false (even)
                return __FALSE(_if_);
            PutIF(_if_);
            break;

        case _macro: // Macro Definition
            return macro();

        case _org: // Set Origin
            uint16_t val = getWordValue();
            if (OutLine[0] == ' ') {
                if (R_opt)
                    val += 0x100;
                LocCtr = CurHEX = val;
                CheckLabel(LocCtr);
                StWord(LocCtr);
            }
            break;

        case _set: // Set Symbol Value
            if (!hasLabel())
                return EndLine('S');
            uint8_t type;
            if ((type = GetSymType()) && type != T_SET)
                SetErr('L');
            SetSymType(T_SET);
            SET_EQU('#');
            hasLabel();
            PutSymVal(ExprVal);
            label = 0;
            break;

        case _title: // Set Title
            CheckLabel(LocCtr);
            GetToken();
            if (tokType != _string || collectDepth)
                return EndLine('S');
            // Free previous title and set new one
            free((void *)Titleptr);
            Titleptr = safeStrdup(token.str);
            GetToken();
            break;

        case _else: // ELSE clause
            CheckLabel(LocCtr);
            if (GetIF() != _if_) {
                SetErr('B');
                GetToken();
                break;
            }
            return __FALSE(_else_);

        case _irp:  // Iterate over Parameters
        case _irpc: // Iterate over Characters
            collectMode = pMnem->code == _irp ? _irp_ : _irpc_;
            CheckLabel(LocCtr);
            GetToken();
            if (tokType == _id) {
                l11c5   = SymTop;
                symBase = SymTop - 1;
                // Store parameter info
                PutSym(3 + min(token.len, SymLen));
                PutSym(0);
                addParameter();
                GetToken();

                // Get parameter list
                if (tokType == _punct && token.str[0] == ',') {
                    l1afc();
                    if (token.len == 0)
                        GetToken();
                    else if (tokType != _punct || !IsTerminator(token.str[0])) {
                        // Copy parameter list
                        for (uint8_t i = 0; i < token.len; i++)
                            PutSym(token.str[i]);
                        PutSym('\r');
                        GetToken();
                    }
                    PutSym(0);
                    curSym = SymTop;
                    if (!collectMacroBody(collectMode))
                        return EndFile(true);
                    return l0a7e();
                }
            } else {
                SetErr('S');
                collectMacroBody(collectMode);
                GetToken();
            }
            break;

        case _rept:                   // Repeat Block
            val     = getWordValue(); // Get repetition count
            l11c5   = SymTop;
            symBase = SymTop - 1;
            // Store repeat count
            PutSym(val & 0xff);
            PutSym(val >> 8);
            curSym = SymTop;
            if (!collectMacroBody(_rept_))
                return EndFile(true);
            return l0a7e();

        // Unsupported directives (for compatibility)
        case _aseg:
        case _cseg:
        case _dseg:
        case _name:
        case _extrn:
        case _inpage:
        case _public:
        case _stkln:
            SetErr('N'); // Not supported
            GetToken();
            break;

        case _page: // Page Control
            CheckLabel(LocCtr);
            GetToken();
            if (tokType != _punct || !IsTerminator(token.str[0])) {
                Expression();
                if (OutLine[0] == ' ')
                    LstPage((uint8_t)ExprVal);
            } else {
                NewPage();
                if (PassNr)
                    Header();
            }
            break;

        case _local: // Local Labels
            if (Balance != 0) {
                do {
                    GetToken();
                    if (tokType != _id)
                        break;
                    uint16_t tmp = SymTop;
                    symBase      = SymTop - 1;
                    addParameter();

                    // Generate unique local label
                    if (++localCnt > 9999)
                        SetErr('O');
                    token.len = sprintf(token.str, "??%04d", localCnt % 10000);
                    l1f87();
                    SymTop  = tmp;
                    symBase = SymTop - 1;
                    GetSymbol();
                    l1fa5();
                    GetToken();

                    if (tokType == _punct && IsTerminator(token.str[0]))
                        return EndLine(0);
                } while (tokType == _punct && token.str[0] == ',');
            }
            SetErr('S');
            break;

        case _maclib: // Macro Library
            CheckLabel(LocCtr);
            if (macroChain || InLIB || !GetLibName()) {
                SetErr('S');
                break;
            }
            if (PassNr) {
                GetToken();
                break;
            }

            // Open and process library file
            OpenLib();
            if (L_opt)
                Header();
            while (curCh != '\n' && curCh != cpmEOF)
                GetStrChr();
            NewPage();
            PrepLib();
            return LINELOOP;
        }
    }
    // Handle 8080/8085 instruction mnemonics
    else {
        uint8_t code = pMnem->code;
        switch (pMnem->type) {
        default:
            return EndLine('S');

        case _1byte: // Single byte instruction
            PutCode(code);
            GetToken();
            break;

        case _rp16: // Register pair with immediate word
            GetRegPair(code);
            GetComma();
            GetImWord();
            break;

        case _rp: // Register pair instruction
            GetRegPair(code);
            break;

        case _pshp: // PUSH/POP register pair
            uint8_t dst = GetDstReg();
            if (dst != (_A << 3) && (dst & (1 << 3)))
                SetErr('R'); // C, E or L not valid
            PutCode(code | (dst & DSTxmsk));
            break;

        case _jpcl: // Jump/Call with address
            PutCode(code);
            GetImWord();
            break;

        case _mov: // MOV instruction
            code |= GetDstReg();
            GetComma();
            PutCode(code | GetSrcReg());
            break;

        case _mvi: // Move immediate
            code |= GetDstReg();
            PutCode(code);
            GetComma();
            GetImByte();
            break;

        case _im8: // Immediate byte instruction
            PutCode(code);
            GetImByte();
            break;

        case _lsax: // LDAX/STAX
            dst = GetDstReg();
            if (dst & ((_H + 1) << 3)) // Allow B and D only
                SetErr('R');
            PutCode(code | (dst & (2 << 3)));
            break;

        case _lsd: // Load/Store direct
            PutCode(code);
            GetImWord();
            break;

        case _alur: // ALU with register
            PutCode(code | GetSrcReg());
            break;

        case _rop: // Rotate operation
            PutCode(code | GetDstReg());
            break;

        case _rpop: // Register pair operation
            dst = GetDstReg();
            if (dst & (1 << 3)) // no A, C, E, L
                SetErr('R');
            PutCode(code | (dst & DSTxmsk));
            break;

        case _rst: // RST instruction
            PutCode(code | GetDstReg());
            break;

        case _io: // IN/OUT instruction
            PutCode(code);
            GetImByte();
            break;
        }
        CheckLabel(LocCtr);
        LocCtr = CurHEX;
    }
    return EndLine(0);
}

/****************************************************************************
 * Helper Functions for Expression Evaluation and Code Generation
 ****************************************************************************/

/**
 * haveComma() - Check for comma separator
 * Returns: true if comma present, false otherwise
 */
bool haveComma() {
    if (tokType == _punct && token.str[0] == ',')
        return true;
    if (tokType != _punct || (token.str[0] != ';' && token.str[0] != '\r'))
        SetErr('D');
    return false;
}

/**
 * getWordValue() - Get 16-bit value from expression
 * Returns: Evaluated expression value
 */
uint16_t getWordValue() {
    GetToken();
    Expression();
    return ExprVal;
}

/**
 * GetByteOper() - Get byte operand from expression
 * Returns: 8-bit value
 */
uint8_t GetByteOper() {
    return CheckByteOper(getWordValue());
}

/**
 * CheckByteOper() - Validate value fits in byte
 * Errors if value out of range (-255 to 255)
 */
uint8_t CheckByteOper(uint16_t val) {
    if ((val & ~0xff) != 0 && (val & ~0xff) != 0xff00)
        SetErr('V');
    return val & 0xff;
}

/**
 * GetSrcReg() - Get source register operand (0-7)
 */
uint8_t GetSrcReg() {
    uint8_t reg = GetByteOper();
    if (reg > _A)
        SetErr('V');
    return reg & _A;
}

/**
 * GetDstReg() - Get destination register operand (shifted to bits 3-5)
 */
uint8_t GetDstReg() {
    return GetSrcReg() << 3;
}

/**
 * GetRegPair() - Parse register pair and emit code
 */
void GetRegPair(uint8_t code) {
    uint8_t reg = GetDstReg();
    if (reg & (1 << 3)) // no Accu, C, E or L
        SetErr('R');
    PutCode(code | (reg & ~(1 << 3))); // Make valid reg pair
}

/**
 * GetImByte() - Get immediate byte and emit
 */
void GetImByte() {
    PutCode(GetByteOper());
}

/**
 * GetImWord() - Get immediate word and emit
 */
void GetImWord() {
    PutWord(getWordValue());
}

/**
 * GetComma() - Expect comma, error if not found
 */
void GetComma() {
    if (tokType != _punct || token.str[0] != ',')
        SetErr('C');
}

/****************************************************************************
 * EndLine() - Finish processing current line
 *
 * Validates line syntax, handles line terminators, and sets up for next line
 *
 * Parameters:
 *   err: Error code to report ('S' for syntax, 0 for none)
 * Returns: LINELOOP for next line, or EndFile result
 ****************************************************************************/
uint8_t EndLine(uint8_t err) {
    if (err)
        SetErr(err);

    if (err != 'S') {
        CheckLabel(LocCtr);
        if (tokType != _punct)
            SetErr('S');
        else if (token.str[0] == '\r') {
            GetToken(); // Consume '\n'
            return LINELOOP;
        } else if (token.str[0] == ';')
            CheckLabel(LocCtr);
        else if (token.str[0] == '!')
            return LINELOOP;
        else if (token.str[0] == cpmEOF)
            return EndFile(Balance != 0);
        else
            SetErr('S');
    }

    // Skip to end of line
    while (1) {
        GetToken();
        if (tokType == _punct) {
            if (token.str[0] == cpmEOF)
                return EndFile(Balance != 0);
            if (token.str[0] == '\n' || token.str[0] == '!')
                return LINELOOP;
        }
    }
}

/****************************************************************************
 * EndFile() - Handle end of file
 *
 * Processes end of source file, manages multi-pass assembly
 *
 * Parameters:
 *   err: true if unbalanced nesting error
 * Returns: PASSLOOP to start next pass, or FINISH when done
 ****************************************************************************/
uint8_t EndFile(bool err) {
    if (err)
        SetErr('B'); // Balance error

    collectDepth = 0;

    // After pass 0, prepare for pass 1
    if (PassNr++ == 0) {
        l11d8 = 0xffff;
        // Reverse macro chain for proper ordering
        while (macroChain) {
            curSym      = macroChain;
            uint16_t hl = GetSymVal();
            PutSymVal(l11d8);
            l11d8      = macroChain;
            macroChain = hl;
        }
        return PASSLOOP;
    }
    return FINISH;
}

/****************************************************************************
 * finish() - Complete assembly and output results
 *
 * Closes files, writes symbol table if requested, displays statistics
 ****************************************************************************/
void finish() {
    GetToken();
    printf("End: %04XH\n", LocCtr);

    // Write symbol table if enabled
    if (S_opt) {
        l2ea4 = 1;
        PrepSYM();
        WriteSymbols();
    }

    // Display memory usage statistics
    printf("Use Factor %2.1f%%\n", (double)(SymTop - SymBot) / (double)(topPtr - SymBot) * 100.0);

    CurHEX = XFerAdr;
    Exit(0);
}

/****************************************************************************
 * Label and Code Generation Functions
 ****************************************************************************/

/**
 * hasLabel() - Check if label is defined for current instruction
 * Returns: true if label exists
 */
bool hasLabel() {
    return (curSym = label) != 0;
}

/**
 * CheckLabel() - Process label if present
 *
 * On pass 0: Define symbol with given location
 * On pass 1: Verify symbol matches pass 0 value
 */
void CheckLabel(uint16_t loc) {
    if (hasLabel()) {
        label = 0;
        if (PassNr == 0) {
            uint8_t type;
            if ((type = GetSymType()) & TYPMASK)
                SetErr('L'); // Multiply defined
            SetSymType(type | T_DEFINED);
            PutSymVal(loc);
        } else if ((GetSymType() & TYPMASK) == 0 || GetSymVal() != loc)
            SetErr('P'); // Phase error
    }
}

/**
 * PutCode() - Emit code byte
 *
 * Writes to HEX file (pass 1) and listing
 */
void PutCode(uint8_t val) {
    if (PassNr)
        putHEX(val);
    if (OutLine[1] == ' ') // Add address if not present
        StWord(LocCtr);
    if (outCol < _ASCbyt) // Truncate if too many bytes
        StByte(val);
    CurHEX++;
}

/**
 * PutWord() - Emit 16-bit word (low byte first)
 */
void PutWord(uint16_t val) {
    PutCode(val & 0xff);
    PutCode(val >> 8);
}

/**
 * Hex formatting helper
 */
char hexDigits[16] = { "0123456789ABCDEF" };

/**
 * StByte() - Store byte as hex in output line
 */
void StByte(uint8_t val) {
    OutLine[outCol++] = hexDigits[val >> 4];
    OutLine[outCol++] = hexDigits[val & 0xf];
}

/**
 * StWord() - Store word as hex in output line
 */
void StWord(uint16_t val) {
    outCol = 1;
    StByte(val >> 8);
    StByte(val & 0xff);
    outCol++;
}

/****************************************************************************
 * Symbol Table Functions
 ****************************************************************************/

/**
 * Ch2Tok() - Convert character to token index for symbol sorting
 *
 * Returns: 0x00-0x19 for A-Z, 0x1A for ?, 0x1B for others
 */
uint8_t Ch2Tok(uint8_t ch) {
    if (isalpha(ch))
        return ch - 'A';
    if (ch == '?')
        return 0x1a;
    return 0x1b;
}

uint16_t symList[0x1c]; // Symbol list for sorting

/**
 * WriteSymbols() - Write sorted symbol table to output
 *
 * Sorts symbols alphabetically and formats for listing
 */
void WriteSymbols() {
    curSym = SymBot;

    // Build sorted symbol lists (one per starting character)
    while (curSym < SymTop) {
        symBase = SymBot;
        uint8_t type;

        if ((type = GetSymType()) == 6) { // Macro
            getParamLen();                // Advance to macro params
            while (SymbolByte() != 0)     // Skip to end of names
                ;
            curSym = symBase + 1;
        } else {
            if (type == l2ea4) {
                uint16_t reference = curSym;
                symBase            = curSym + 1;
                GetSymbol();

                // Skip internal labels (starting with ??)
                if (Q_opt || token.len < 2 || token.str[0] != '?' || token.str[1] != '?') {
                    // Insert into sorted list
                    sym_t *chain = (sym_t *)&symList[Ch2Tok(token.str[0])];
                    while ((curSym = chain->link) != 0) {
                        sym_t *sym   = SYMP(curSym);
                        uint8_t slen = (sym->typeSize & LOMASK) + 1;
                        int8_t cmp   = memcmp(sym->name, token.str, min(slen, token.len));
                        if (cmp > 0 || (cmp == 0 && slen > token.len))
                            break;
                        chain = sym;
                    }
                    uint16_t tmp       = curSym;
                    curSym             = reference;
                    SYMP(curSym)->link = tmp;
                    chain->link        = curSym;
                }
            }
            curSym += SymSize() + 5; // Move to next symbol
        }
    }

    // Output sorted symbols with tab formatting
    uint8_t symCol = 0;
    outCol         = 0;
    for (uint8_t i = 0; i < 0x1c; i++) {
        curSym = symList[i];
        while (curSym) {
            uint8_t symWidth = (BYTE(curSym + 2) & 0xf) + 1;
            symBase          = curSym + 2;

            // Format with tabs
            if (symCol) {
                StASC('\t');
                symCol = (symCol & 0xf8) + 8;
                if (symCol & 0xf) {
                    symCol += 8;
                    StASC('\t');
                }
            }

            // Start new line if too long
            if (symCol + symWidth + 5 >= 80) {
                OutLen = outCol;
                emitLine();
                symCol = outCol = 0;
            }

            // Output value and name
            uint16_t hl = GetSymVal();
            StByte(hl >> 8);
            StByte(hl & 0xff);
            StASC(' ');
            symCol += 5;

            for (uint8_t i = symWidth; i; i--) {
                StASC(SymbolByte());
                symCol++;
            }
            curSym = WORD(curSym);
        }
    }

    if ((OutLen = outCol))
        emitLine();
}

/**
 * StASC() - Store ASCII character in output line
 */
void StASC(uint8_t ch) {
    OutLine[outCol++] = ch;
}

/**
 * l11e4() - Clear afterComma marker if matched
 */
void l11e4(uint16_t de) {
    if (de == afterComma)
        BYTE(afterComma) = 0;
}

/**
 * l11ee() - Update character pointer
 */
void l11ee(uint16_t hl) {
    Stack[0].chPtr = hl;
    if (BYTE(hl) == '\r')
        BYTE(hl) = 0;
}

/****************************************************************************
 * Expression Evaluation System
 ****************************************************************************/

// Expression evaluation variables
bool Parenth;              // Parenthesis flag
uint8_t ExprCode[ExprCod]; // Operator codes
uint8_t ExprPrio[ExprCod]; // Operator priorities
uint16_t ExprStk[ExprDep]; // Operand stack
uint8_t ExprDepth;         // Operator stack depth
uint8_t ExprLen;           // Operand stack depth

/**
 * PushExpr() - Push operand onto expression stack
 */
void PushExpr(uint16_t val) {
    if (ExprLen >= ExprDep) {
        SetErr('E');
        ExprLen = 0;
    } else
        ExprStk[ExprLen++] = val;
}

/**
 * St_Code_Prio() - Store operator code and priority
 */
void St_Code_Prio(uint8_t code, uint8_t priority) {
    if (ExprDepth >= ExprCod) {
        ExprDepth = 0;
        SetErr('E');
    } else {
        ExprCode[ExprDepth]   = code;
        ExprPrio[ExprDepth++] = priority;
    }
}

/**
 * PopExpr() - Pop operand from expression stack
 */
uint16_t PopExpr() {
    if (ExprLen == 0) {
        SetErr('E');
        return 0;
    }
    return ExprStk[--ExprLen];
}

/**
 * PopTwoOper() - Pop two operands for binary operation
 */
pair_t PopTwoOper() {
    pair_t pair;
    pair.de = PopExpr(); // Right operand
    pair.hl = PopExpr(); // Left operand
    return pair;
}

/**
 * Term() - Evaluate operator and push result
 *
 * Handles all arithmetic, logical, relational, and bitwise operators
 */
void Term(uint8_t op) {
    pair_t operand;
    uint16_t result;

    switch (op) {
    case _mul: // Multiplication
        operand = PopTwoOper();
        result  = operand.hl * operand.de;
        break;

    case _div: // Division
        operand = PopTwoOper();
        if (operand.de)
            result = operand.hl / operand.de;
        else {
            SetErr('Z'); // Divide by zero
            result = 0xffff;
        }
        break;

    case _mod: // Modulo
        operand = PopTwoOper();
        if (operand.de)
            result = operand.hl % operand.de;
        else {
            SetErr('Z');
            result = operand.hl;
        }
        break;

    case _shl: // Shift left
        operand = PopTwoOper();
        if (operand.de > 16) {
            SetErr('E');
            result = 0;
        } else
            result = operand.hl << operand.de;
        break;

    case _shr: // Shift right
        operand = PopTwoOper();
        if (operand.de > 16) {
            SetErr('E');
            result = 0;
        } else
            result = operand.hl >> operand.de;
        break;

    case _plus: // Addition
        operand = PopTwoOper();
        result  = operand.hl + operand.de;
        break;

    case _minus: // Subtraction
        operand = PopTwoOper();
        result  = operand.hl - operand.de;
        break;

    case _uminus: // Unary minus
        result = -PopExpr();
        break;

    case _eq: // Equal
        operand = PopTwoOper();
        result  = operand.hl == operand.de ? TRUE : FALSE;
        break;

    case _lt: // Less than
        operand = PopTwoOper();
        result  = operand.hl < operand.de ? TRUE : FALSE;
        break;

    case _le: // Less or equal
        operand = PopTwoOper();
        result  = operand.hl <= operand.de ? TRUE : FALSE;
        break;

    case _gt: // Greater than
        operand = PopTwoOper();
        result  = operand.hl > operand.de ? TRUE : FALSE;
        break;

    case _ge: // Greater or equal
        operand = PopTwoOper();
        result  = operand.hl >= operand.de ? TRUE : FALSE;
        break;

    case _ne: // Not equal
        operand = PopTwoOper();
        result  = operand.hl != operand.de ? TRUE : FALSE;
        break;

    case _not: // Bitwise NOT
        result = ~PopExpr();
        break;

    case _and: // Bitwise AND
        operand = PopTwoOper();
        result  = operand.hl & operand.de;
        break;

    case _or: // Bitwise OR
        operand = PopTwoOper();
        result  = operand.hl | operand.de;
        break;

    case _xor: // Bitwise XOR
        operand = PopTwoOper();
        result  = operand.hl ^ operand.de;
        break;

    case _high: // High byte
        result = PopExpr() >> 8;
        break;

    case _low: // Low byte
        result = PopExpr() & 0xff;
        break;

    default:
        SetErr('E');
        return;
    }

    PushExpr(result);
}

bool IsExprDel() {
    return tokType == _punct && strchr("\r;!", token.str[0]);
}

bool IsExprDelOrComma() {
    return tokType == _punct && strchr(",\r;!", token.str[0]);
}

void Expression() {
    ExprDepth = 0;
    ExprLen   = 0;
    Parenth   = true;
    ExprVal   = 0;
    while (1) {
        if (IsExprDelOrComma()) {
            while (ExprDepth)
                Term(ExprCode[--ExprDepth]);
            if (ExprLen != 1)
                SetErr('E');
            if (OutLine[0] == ' ')
                ExprVal = ExprStk[0];
            return;
        }
        if (OutLine[0] == ' ') {
            if (tokType == _string) {
                if (token.len == 0 || token.len > 2)
                    SetErr('E');
                // expression relies on string being null terminated
                PushGood(token.len == 0 ? 0 : token.str[0] + (token.str[1] << 8));
            } else if (tokType == _number)
                PushGood(Value);
            else if ((pMnem = GetMnemo(token.str, token.len))) { // Expr_Mnemo?
                uint8_t type = pMnem->type;
                uint8_t pri  = pMnem->code;
                if (type < _regs) {
                    if (type == _nul) {
                        l1afc();
                        if (!IsExprDel() && (tokType != _string || token.len ||
                                             (GetToken(), !IsExprDelOrComma()))) {
                            do {
                                l1afc();
                            } while (!IsExprDel());
                            PushGood(FALSE);
                        } else
                            PushGood(TRUE);
                        continue;
                    } else if (type == _lpar) { // Expr_Expr?
                        if (!Parenth)
                            SetErr('E');
                        St_Code_Prio(type, pri);
                        Parenth = true;

                    } else if (Parenth && type == _plus)
                        ;
                    else {
                        if (Parenth) {
                            if (type == _minus)
                                type++; // convert to unary minus
                            else if (type != _not && type != _high && type != _low)
                                SetErr('E');
                        }
                        while (ExprDepth && ExprPrio[ExprDepth - 1] >= pri)
                            Term(ExprCode[--ExprDepth]);
                        if (type == _rpar) { // 1530
                            if (ExprDepth == 0 || ExprCode[--ExprDepth] != _lpar)
                                SetErr('E');
                            Parenth = false;
                        } else {
                            St_Code_Prio(type, pri);
                            Parenth = true;
                        }
                    }
                } else {
                    if (type == _maclib)
                        SetErr('E');
                    PushGood(pri);
                }
            } else if (tokType == _punct) {
                if (token.str[0] == '$')
                    PushGood(LocCtr);
                else {
                    SetErr('E');
                    PushGood(0);
                }
            } else {
                if (!SymSrc()) {
                    SetErr('P');
                    PostSym();
                } else if ((GetSymType() & TYPMASK) == 0)
                    SetErr('U');
                PushGood(GetSymVal());
            }
        }
        GetToken();
    }
}

void PushGood(uint16_t val) {
    if (!Parenth)
        SetErr('E');
    Parenth = false;
    PushExpr(val);
}

uint8_t RdSrc() {
    uint8_t ch;

    while (Balance) {
        if ((ch = BYTE(Stack[0].chPtr))) {
            Stack[0].chPtr++;
            if (ch == ',')
                afterComma = Stack[0].chPtr;
            break;
        }
        if (Stack[0].collectMode != 2) {
            if (F_CHR++ != 0xff)
                return 0;
            SetErr('B');
            emitLine();
        }
        unnest();
        ch = Stack[0].curCh;
        if (ch)
            return ch;
    }
    if (Balance == 0)
        ch = mgetc();
    F_CHR = ch;
    return tokType == _string ? ch : toupper(ch);
}

uint8_t StChar(uint8_t ch) {
    if (ch != '\r' && ch != '\n' && OutLen < LINLEN)
        OutLine[OutLen++] = ch;
    return ch;
}

bool GetId() {
    PrnLine.len = 0;
    expandChIdx  = 0;
    lookAhead    = RdSrc();
    if (tokType == _cmnt)
        return false;
    if (lookAhead >= 0x80) {
        char *p      = getMnemStr(lookAhead);
        PrnLine.len = (uint8_t)strlen(p);
        memcpy(PrnLine.str, p, PrnLine.len);
        lookAhead = RdSrc();
    } else if (!IsLabCh(lookAhead))
        return false;
    while (IsLabel(lookAhead)) {
        if (PrnLine.len >= SymLen - 1)
            return false;
        PrnLine.str[PrnLine.len++] = lookAhead;
        lookAhead                    = RdSrc();
    }
    PrnLine.str[PrnLine.len] = '\0'; // make C string
    return true;
}

bool l16f3() {
    bool result;
    l1613 = curSym;
    SymMArg();
    if (!(result = l1e47()))
        curSym = l1613;
    return result;
}

uint8_t RdChar() {
    uint8_t expandCnt = 0;
    while (1) {
        if (expandCnt++ == 255) { // too many expansions
            SetErr('O');
            PrnLine.len   = 0;
            Stack[0].chPtr = 0; // original was PrnLine
            // here 0 will access mem[0] which is always 0
        }
        if (PrnLine.len) { // pending char
            PrnLine.len--;
            return StChar(PrnLine.str[expandChIdx++]);
        }
        uint8_t ch = lookAhead;
        if (Balance == 0) { // simple read
            if (lookAhead)
                lookAhead = 0;
            else
                ch = RdSrc();
            return StChar(ch);
        } else if (lookAhead) {
            if (lookAhead == '^') {
                if (!GetId() && lookAhead == '&') {
                    PrnLine.len++;
                    PrnLine.str[0] = '&';
                    lookAhead       = 0;
                    return StChar('&');
                }
                return StChar('^');
            } else if (lookAhead == '&') { // 176c
                if (!GetId() || !l16f3())  // 179e
                    return StChar('&');
            } else if (lookAhead != 0x7f) {
                lookAhead = 0;
                return StChar(ch);
            } else if (!GetId() || !l16f3())
                continue;
        } else if (!GetId() || (lookAhead != '&' && tokType == _string) || !l16f3())
            continue;

        Stack[0].curCh = lookAhead == '&' ? 0x7f : lookAhead;
        lookAhead      = 0;

        nest();
        Stack[0].collectMode = 2;
        Stack[0].prevTopPtr  = topPtr;
        Stack[0].chPtr       = GetSymValAddr();
        PrnLine.len         = 0;
        curSym               = l1613;
        GetId();
    }
}

void l17f1() {
    IniEval();
    PrnLine.len = 0;
    lookAhead    = 0;
    curCh        = 0;
    OutLen       = 0;
    prevCh       = '\n';
    emitLine();
    OutLen = _ASCbyt;
}

void IniEval() {
    token.len = 0;
    ValBase   = 0;
}

void addCh() {
    if (token.len >= ArgMax) {
        token.len = 0;
        SetErr('O');
    }
    token.str[token.len++] = curCh;
    token.str[token.len]   = 0; // make sure it's always terminated
}

bool IsLabCh(uint8_t ch) {
    return isalpha(ch) || ch == '?' || ch == '@';
}

bool IsLabel(uint8_t ch) {
    return isalnum(ch) || ch == '?' || ch == '@';
}

uint8_t IsValid(uint8_t ch) {
    if (ch < ' ' && ch != '\t' && ch != '\r' && ch != '\n' && ch != cpmEOF)
        SetErr('I');
    return ch;
}

uint8_t GetStrChr() {
    curCh = IsValid(RdChar());
    if (collectDepth && (collectMode != _macro_ || PassNr == 0))
        PutSym(curCh);
    return curCh;
}

void skipEOL() {
    while (!IsEOS(curCh))
        GetStrChr();
}

bool GetLibName() {
    char name[256];
    uint16_t len = 0;
    tokType      = _string; // make sure chars are not mapped to upper case

    free((void *)libFile); // incase previously used
    libFile = NULL;

    while (curCh == ' ' || curCh == '\t' || curCh == 0)
        GetStrChr();
    bool instring = false;
    if (curCh == '\'') { // collect string to allow any char >= ' '
        instring = true;
        while ((curCh = GetStrChr()) >= ' ') {
            if (curCh == '\'' && (curCh = GetStrChr()) != '\'') {
                instring = false;
                break;
            }
            if (len < 255)
                name[len++] = curCh;
        }
    } else {
        while (curCh > ' ' && curCh != ';') {
            if (len < 255)
                name[len++] = curCh;
            GetStrChr();
        }
    }
    tokType = 0;
    if (instring || len == 0 || len >= 255) {
        SetErr('V');
        return false;
    }

    name[len] = '\0';
    libFile   = makeFilename(name, ".lib", false);
    return true;
}

void GetToken() {
    IniEval();
    for (;;) {
        tokType = 0;
        if (curCh == '\t')
            ;
        else if (curCh == ';') {
            tokType = _cmnt;
            GetStrChr();
            if (collectDepth && (collectMode != _macro_ || PassNr == 0) && curCh == ';') {
                for (symBase -= 2; symBase != startMacroBody; symBase--) {
                    if (BYTE(symBase) == '\n' || BYTE(symBase) >= '!')
                        break;
                }
                if (symBase != startMacroBody && BYTE(symBase) == '\n')
                    symBase -= 2;
                uint8_t tmp  = collectDepth;
                collectDepth = 0;
                skipEOL();
                PutSym(curCh);
                collectDepth = tmp;
            } else
                skipEOL();
            break;
        } else if (curCh != '*' ? (curCh && curCh != ' ') : (prevCh && prevCh != ' '))
            break;
        GetStrChr();
    } // of forever

    tokType = 0;
    if (IsLabCh(curCh))
        tokType = _id;
    else if (isdigit(curCh))
        tokType = _number;
    else if (curCh == '\'') {
        curCh   = 0;
        tokType = _string;
    } else {
        if (curCh == '\n') {
            if (Balance)
                OutLine[5] = '+';
            emitLine();
            OutLine[0] = ' ';
            OutLen     = _ASCbyt;
        }
        tokType = _punct;
    }
    for (;;) {
        if ((prevCh = curCh))
            addCh();
        GetStrChr();
        if (tokType == _punct) {
            if (collectDepth) // Test relation
                return;
            char *rel;
            if (token.str[0] == '=')
                rel = "EQ";
            else if (token.str[0] == '<') {
                if (curCh == '=') {
                    curCh = 0;
                    rel   = "LE";
                } else if (curCh == '>') { // PMO added missing <>
                    curCh = 0;
                    rel   = "NE";
                } else
                    rel = "LT";
            } else if (token.str[0] == '>') {
                if (curCh == '=') {
                    curCh = 0;
                    rel   = "GE";
                } else
                    rel = "GT";
            } else
                return;
            token.len = 2;
            strcpy(token.str, rel);
            tokType = _id;
            return;
        } else if (tokType == _id) {
            if (curCh == '$')
                curCh = 0;
            else if (!IsLabel(curCh))
                return;
        } else if (tokType == _number) {
            if (curCh == '$')
                curCh = 0;
            else if (!isxdigit(curCh)) {
                if (curCh == 'O' || curCh == 'Q' || curCh == 'H') {
                    ValBase = curCh == 'H' ? 16 : 8;
                    curCh   = 0;
                } else if (prevCh == 'B' || prevCh == 'D') {
                    ValBase = prevCh == 'D' ? 10 : 2;
                    token.len--;
                } else
                    ValBase = 10;
                uint32_t val = 0;
                for (uint8_t i = 0; i < token.len; i++) {
                    uint8_t digit = token.str[i];
                    digit -= isdigit(digit) ? '0' : ('A' - 10);
                    if (digit >= ValBase)
                        SetErr('V');
                    if ((val = val * ValBase + digit) >= 0x10000)
                        SetErr('V');
                }
                Value = (uint16_t)val;
                return;
            }
        } else { // string
            if (curCh == '\r') {
                SetErr('O');
                return;
            } else if (curCh == '\'' && GetStrChr() != '\'')
                return;
        }
    }
}

bool IsSpace(uint8_t ch) {
    return ch == '\0' || ch == ' ' || ch == '\t';
}

bool IsDelimiter() {
    return curCh == ',' || curCh == ';' || curCh == '%' || IsEOS(curCh);
}

bool IsEOS(uint8_t ch) {
    return ch == '\r' || ch == cpmEOF || ch == '!';
}

bool IsTerminator(uint8_t ch) {
    return ch == '\r' || ch == '!' || ch == ';';
}

bool IsDelim(uint8_t ch) {
    return ch == ';' || ch == ' ' || ch == '\t' || ch == ',';
}

/****************************************************************************
 * l1afc() - Lexical analyzer/tokenizer
 *
 * Parses input stream and builds tokens, handling:
 * - String literals (delimited by single quotes with '' escaping)
 * - Escape sequences (^ prefix)
 * - Angle bracket delimited sections (< >)
 * - Punctuation tokens
 * - Delimiter detection
 *
 * Token types set:
 * - _string: String literal
 * - _punct: Punctuation/delimiter
 * - _error: Syntax error encountered
 *
 * Errors generated:
 * - 'V': Unterminated string or unmatched angle brackets
 * - 'I': Invalid escape sequence (non-printable after ^)
 ****************************************************************************/
void l1afc() {
    IniEval();
    tokType            = 0;
    uint8_t chevronCnt = 0; // Track nesting level of angle brackets < >
    bool setPrev       = false;

    // Skip leading whitespace
    while (IsSpace(curCh))
        GetStrChr();

    // Handle initial delimiter/punctuation token
    if (IsDelimiter()) {
        tokType = _punct;
        addCh();
        setPrev = true;
    }

    while (1) { // Main tokenization loop
        if (setPrev) {
            prevCh = curCh;
            GetStrChr();
            // If we processed punctuation, return immediately
            if (tokType == _punct)
                return;
        } else
            setPrev = true;

        while (1) { // Character processing loop
            // Check for end of string - error if inside string literal or unmatched brackets
            if (IsEOS(curCh)) {
                if (tokType == _string || chevronCnt != 0)
                    SetErr('V'); // Error 'V': Unterminated string or unmatched angle brackets
                tokType = _error;
                return;
            }

            // Handle string literals delimited by single quotes
            if (tokType == _string) {
                if (curCh == '\'') {
                    addCh();
                    GetStrChr();
                    // Check for escaped quote (two consecutive single quotes)
                    if (curCh == '\'')
                        break;   // It's an escaped quote, continue in string
                    tokType = 0; // End of string literal
                    continue;
                }
            }
            // Start of string literal
            else if (curCh == '\'')
                tokType = _string;

            // Handle escape sequences with caret (^)
            else if (curCh == '^') {
                GetStrChr();
                // Validate escaped character is tab or printable (>= space)
                if (curCh != '\t' && curCh < ' ') {
                    SetErr('I'); // Error 'I': Invalid escape sequence
                    tokType = _error;
                    return;
                }
            }
            // Track angle bracket nesting - opening bracket
            else if (curCh == '<') {
                if (chevronCnt++ == 0)
                    break; // Entering first level of angle brackets
            }
            // Track angle bracket nesting - closing bracket
            else if (curCh == '>') {
                if (chevronCnt && --chevronCnt == 0)
                    break; // Exiting last level of angle brackets
            }
            // Error if delimiter found outside of angle brackets
            else if (chevronCnt == 0 && IsDelim(curCh)) {
                tokType = _error;
                return;
            }

            addCh(); // Add current character to token
            break;
        }
    }
}

uint16_t symHashTab[FLen];
uint8_t HashIdx;
uint16_t *curHashTab = symHashTab;

void IniField_1() {
    memset(symHashTab, 0, sizeof(symHashTab));
    curSym = NIL;
}

void IniField_2() {
    memset(macHashTab, 0, sizeof(macHashTab));
}

void nest() {
    if (Balance >= MaxBal)
        SetErr('B');
    else
        Stack[++Balance] = Stack[0];
}

void unnest() {
    if (Balance == 0)
        SetErr('B');
    else
        Stack[0] = Stack[Balance--];
}

uint8_t GetHashIdx(pstr_t *p) {
    uint8_t hash = 0;
    for (uint8_t i = 0; i < p->len; i++)
        hash += p->str[i];
    return HashIdx = hash & 0x7f;
}

#if 0
void l1e31(uint8_t len) {
    SYMP(curSym)->typesize &= 0xf0;
    SYMP(curSym)->typesize |= len;
}
#endif

uint8_t SymSize() {
    return (SYMP(curSym)->typeSize & 0xf) + 1;
}

bool l1e47() {
    if (curSym == 0)
        return false;
    uint8_t bc;
    if (Stack[0].collectMode != _macro_) {
        for (bc = Balance; bc; bc--) {
            if (Stack[bc].collectMode == _macro_)
                break;
        }
        if (bc == 0)
            return false;
    } else
        bc = 0;
    if (Stack[bc].prevTopPtr > curSym)
        return true;
    curSym = 0;
    return false;
}

bool SymAdr() {
    return curSym != 0;
}

bool SymMArg() {
    if (PrnLine.len > SymLen)
        PrnLine.len = SymLen;
    return lookup((pstr_t *)&PrnLine, macHashTab[GetHashIdx((pstr_t *)&PrnLine) & 0xf]);
}

bool SymSrc() {
    if (token.len > SymLen)
        token.len = SymLen;
    return lookup((pstr_t *)&token, symHashTab[GetHashIdx((pstr_t *)&token)]);
}

bool lookup(pstr_t *ps, uint16_t head) {
    for (curSym = head; curSym; curSym = WORD(curSym)) {
        if (ps->len == SymSize() && memcmp(ps->str, &BYTE(curSym + 3), ps->len) == 0)
            return true;
    }
    return false;
}

void PostSym() {
    curSym = SymTop;
    SymTop += token.len + 5; // packed sizeof(sym_t)
    if (SymTop >= topPtr)
        fatal("Symbol Table Overflow");
    curHashTab = symHashTab;
    l1f31();
    WORD(SymTop - 2) = 0; // clear value
}

void l1f31() {
    sym_t *ps           = SYMP(curSym);

    uint16_t curLink    = curHashTab[HashIdx];
    curHashTab[HashIdx] = curSym;
    ps->link            = curLink;
    uint8_t len         = min(SymLen, token.len);
    ps->typeSize        = len - 1;
    memcpy(ps->name, token.str, len);
}

void alloc(uint16_t bc) {
    curSym = topPtr - (bc + token.len);
    if (curSym < SymTop)
        fatal("Symbol table overflow");
    topPtr = curSym;
}

void l1f87() {
    alloc(1);
    memcpy(&BYTE(topPtr), token.str, token.len);
    BYTE(topPtr + token.len) = 0;
}

void l1fa5() {
    alloc(3);
    curHashTab = macHashTab;
    l1f31();
    SetSymType(HashIdx);
}

void PopParam() {
    while (topPtr < Stack[0].prevTopPtr) {
        curSym                   = topPtr;
        macHashTab[GetSymType()] = SYMP(curSym)->link; // remove from macro hash table
        uint16_t hl              = GetSymValAddr();    // point to param value
        while (BYTE(hl++))                             // skip the parameters
            ;
        topPtr = hl; // update to reflect removal
    }
}

// Byte 0,1  Link address - or zero
// Byte  2   High: Type - Low: Length
// Byte 3..n-1   Symbol
// Byte n,n+1    Symbol value

void SetSymType(uint8_t type) {
    sym_t *ps    = SYMP(curSym);
    ps->typeSize = (ps->typeSize & 0xf) | (type << 4);
}

uint8_t GetSymType() {
    sym_t *ps = SYMP(curSym);
    return ps->typeSize >> 4;
}

uint16_t GetSymValAddr() {
    return curSym + SymSize() + 3;
}

void PutSymVal(uint16_t val) {
    WORD(GetSymValAddr()) = val;
}

uint16_t GetSymVal() {
    return WORD(GetSymValAddr());
}

void SeekMacroParam() {
    symBase = GetSymValAddr() + 2;
}

void setParameterCnt(uint8_t val) {
    SeekMacroParam();
    BYTE(symBase) = val;
}

uint8_t getParamLen() {
    SeekMacroParam();
    return BYTE(symBase);
}

void addParameter() {
    uint8_t type = GetHashIdx((pstr_t *)&token) << 4;
    if (token.len > SymLen)
        token.len = SymLen;
    PutSym(type | (token.len - 1));
    for (uint8_t i = 0; i < token.len; i++)
        PutSym(token.str[i]);
}

void GetSymbol() {
    uint8_t b = SymbolByte();
    HashIdx   = b >> 4;
    token.len = (b & 0xf) + 1;
    for (int i = 0; i < token.len; i++)
        token.str[i] = SymbolByte();
}

uint8_t SymbolByte() {
    return BYTE(++symBase);
}

void PutSym(uint8_t ch) {
    if (symBase + 1 >= topPtr)
        fatal("Symbol Table Overflow");
    else {
        BYTE(++symBase) = ch;
        SymTop         = symBase + 1;
    }
}
