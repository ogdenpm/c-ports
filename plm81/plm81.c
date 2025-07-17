/****************************************************************************
 *  plm81.c: part of the C port of Intel's cross compiler plm81             *
 *  The original application is Copyright Intel                             *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

/*              p a s s - 1    e r r o r   m e s s a g e s

    Error                           Message
    Number
    ------  -------------------------------------------------------------
       1    the symbols printed below have been used in the current block
            but do not appear in a declare statement, or LABEL appears in
            a go to statement but does not appear in the block.

       2    pass-1 compiler symbol table overflow.  Too many symbols in
            the source program.  Either reduce the number of variables in
            the program, or re-compile pass-1 with a larger symbol table.

       3    invalid pl/m statement.  The pair of symbols printed below
            cannot appear together in a valid pl/m statement (this error
            may have been caused be a previous error in the program).

       4    invalid pl/m statement.  The statement is improperly formed--
            the parse to this point follows (this may have occurred be-
            cause of a previous program error).

       5    pass-1 parse stack overflow.  The program statements are
            recursively nested too deeply.  Either simplify the program
            structure, or re-compile pass-1 with a larger parse stack.

       6    number conversion error.  The number either exceeds 65535 or
            contains digits which conflict with the radix indicator.

       7    pass-1 table overflow.  Probable cause is a constant string
            which is too long.  If so, the string should be written as a
            sequence of shorter strings, separated by commas.  Otherwise,
            re-compile pass-1 with a larger varc table.

       8    macro table overflow.  Too many literally declarations.
            either reduce the number of literally declarations, or re-
            compile pass-1 with a larger 'macros' table.

       9    invalid constant in initial, data, or in-line constant.
            Precision of constant exceeds two bytes (may be internal
            pass-1 compiler error).

      10    invalid program.  Program syntax incorrect for termination
            of program.  May be due to previous errors which occurred
            within the program.

      11    invalid placement of a declaration within the pl/m program.
            Procedures and variables may only be declared in the outer
            block (main part of the program) or within do-end groups
            (not iterative do's: case  do-while's: case  or do-case's).

      12    improper use of identifier following an end statement.
            Identifiers can only be used in this way to close a procedure
            definition.

      13    identifier following an end statement does not match the name
            of the procedure which it closes.

      14    duplicate formal parameter name in a procedure heading.

      15    identifier following an end statement cannot be found in the
            program.

      16    duplicate LABEL definition at the same block level.

      17    numeric LABEL exceeds cpu addressing space.

      18    invalid call statement.  The name following the call is not
            a procedure.

      19    invalid destination in a go to.  The value must be a LABEL
            or simple variable.

      20    macro table overflow (see error 8 above).

      21    duplicate variable or LABEL definition.

      22    variable which appears in a data declaration has been pre-
            viously declared in this block

      23    pass-1 symbol table overflow (see error 2 above).

      24    invalid use of an identifier as a variable name.

      25    pass-1 symbol table overflow (see error 2 above).

      26    improperly formed based variable declaration.  The form is
            i based j, where i is an identifier not previously declared
            in this block, and j is an address variable.

      27    symbol table overflow in pass-1 (see error 2 above).

      28    invalid address reference.  The dot operator may only
            precede simple and subscripted variables in this context.

      29    undeclared variable.  The variable must appear in a declare
            statement before its use.  (May also be caused by a recursive
            literally declaration.)

      30    subscripted variable or procedure call references an un-
            declared identifier.  The variable or procedure must be
            declared before it is used.

      31    the identifier is improperly used as a procedure or sub-
            scripted variable.

      32    too many subscripts in a subscripted variable reference.
            pl/m allows only one subscript.

      33    iterative do index is invalid. in the form 'do i = e1 to e2'
            the variable i must be simple (unsubscripted).

      34    attempt to complement a $ control toggle where the toggle
            currently has a value other than 0 or 1.  Use the '= n'
            option following the toggle to avoid this error.

      35    input file number stack overflow.  Re-compile pass-1 with
            a larger instk table.

      36    too many block levels in the pl/m program.  Either simplify
            your program (30 block levels are currently allowed) or
            re-compile pass-1 with a larger block table.

       37   The number of actual parameters in the calling sequence
            is greater than the number of formal parameters declared
            for this procedure.

       38   The number of actual parameters in the calling sequence
            is less than the number of formal parameters declared
            for this procedure.

       39   Invalid interrupt number (must be between 0 and 7)

       40   Duplicate interrupt procedure number.  A procedure
            has been previously specified with an identical
            interrupt attribute.

       41   Procedure appears on left-hand-side of an assignment.

       42   Attempted 'call' of a typed procedure.

       43   Attempted use of an untyped procedure as a function
            or a variable.

       44   This procedure is untyped and should not return a value.

       45   This procedure is typed and should return a value.

       46   'return' is invalid outside a procedure definition.

       47   Illegal use of a LABEL as an identifier.

       48   invalid procedure name (procedure names must be identifiers).

       49   warning - a macro expansion has terminated the scope of
            its own name.  (Probably a bad 'literally' declaration.)

       50   too many chained assignments in a statement.  The maximum
            number of assignments is currently set to 256.  Either
            reduce the number of assignments in the block, or re-compile
            pass-1 with a larger 'assign' table.

       51   missing value on control line
*/

/*
     ------  -------------------------------------------------------------
                i m p l e m e n t a t i o n    n o t e s
                - - - - - - - - - - - - - -    - - - - -



/*************************************************************************/
#include "plm81.h"
#include <stdarg.h>
#include <time.h>
#ifdef _MSC_VER
#include <io.h>
#endif
#include "utility.h"

/* stacks */
#define PSTACK 75
#define MVAR   4094
int sp;
int mp = 0;
// int mpp1 = 1;
int vartop = 1;

struct {
    uint8_t tokId;
    uint16_t loc;
    uint16_t len;
    int fixv;
    int fixc;
} pstack[PSTACK + 1];

// int var[MSTACK + 1];
uint8_t varc[MVAR + 1];

bool failsf = false;
bool compil = true;

/*
 * The original code for inline data stored definitions of online from top of symbol array downwards
 * to allow for symbol to be a structure a separate array is used
 *
 * new format builds up rather than down
 *
 * ((var id) << 15) + (data item cnt)
 * then for each data item
 * ((item id) << 16) + (item symbol index)
 *
 *
 * Note when symbol is made into a structure the format will be modified to use uint16_t
 *
 * var id
 * data item cnt
 * then for each data item
 * symbolIdx
 *
 *
 */
#define MAXINITIAL 32000
uint32_t initialData[MAXINITIAL];
uint16_t initialDataSP;
#define MAXIDLIST 32
uint16_t idList[MAXIDLIST];
uint16_t idListSP;

#define SYMABS 5000
struct _symbol symbol[SYMABS + 1];

int symNext = 1; // 0 is reserved as a sentinal
/*
Symbol table below initial entries are shown below
install loads them into the symbol table
*/
struct {
    char *name;
    uint16_t len;
    uint8_t prec;
    uint8_t type;
    int32_t info;
} builtins[] = {
    { "CARRY", 0, 1, VARB },  { "ZERO", 0, 1, VARB },   { "SIGN", 0, 1, VARB },
    { "PARITY", 0, 1, VARB }, { "MEMORY", 0, 1, VARB }, { "STACKPTR", 0, 2, VARB },
    { "ROL", 2, 1, INTR },    { "ROR", 2, 1, INTR },    { "SHL", 2, 1, INTR },
    { "SHR", 2, 1, INTR },    { "SCL", 2, 1, INTR },    { "SCR", 2, 1, INTR },
    { "TIME", 1, 0, INTR },   { "HIGH", 1, 1, INTR },   { "LOW", 1, 1, INTR },
    { "INPUT", 1, 1, INTR },  { "OUTPUT", 1, 1, INTR }, { "LENGTH", 1, 1, INTR },
    { "LAST", 1, 1, INTR },   { "MOVE", 3, 0, INTR },   { "DOUBLE", 1, 2, INTR },
    { "DEC", 1, 1, INTR }, // { "", -0, 0, VARB } // keeps block tracking aligned with original

};

#define MAXASSIGN 256      /* max number of assignments in a statement */
int assign[MAXASSIGN + 1]; /* assignment symbol indexes */

inline unsigned iabs(int a) {
    return a < 0 ? -a : a;
}

uint16_t symloc;
uint16_t symlen;
int acnt = 0;
// control options 'A'->'Z' - 'A'
int contrl[26];

const int VERS = 40;
/* interrupts */
int intpro[8];

#define MAXBLK 30

int block[MAXBLK] = { 1 };
struct {
    unsigned type : 2;
    unsigned clause : 14;
    unsigned label : 16;
} dopar[MAXBLK + 1];

inline void doHead(int slot, int type, int clause) {
    dopar[slot].type   = type;
    dopar[slot].clause = clause & 0x3fff;
    dopar[slot].label  = clause >> 14;
}

enum { DO_GROUP, DO_ITER, DO_WHILE, DO_CASE };

int macblk[MAXBLK];
int curblk = 1;
int blksym;
int proctp[MAXBLK];
/* hash */
int hentry[128];
int hcode;
int errorCnt;

/*
 file management is changed from the original cross compiler
 The source file is specified on the command line
 its name prefix is used to create other files
 srcFp   - uses specfied file, adding .plm if there is no extent
 polFp   - creates prefix.pol
 symFp   - creates prefix.sym
 lstFp   - creates prefix.lst

 To support include files $I=filename can be used with the previous file being stacked
 Note a .plm extension is used if no extension is given. Filename cannot contain a space
*/

// helper function to set old info word as separate items
void setInfo(int symIdx, uint16_t len, uint8_t prec, uint8_t type) {
    symbol[symIdx].len  = len;
    symbol[symIdx].prec = prec;
    symbol[symIdx].type = type;
}

char const help[] = {
    "Usage : %s [-a] [-b] [-l nn] [-g] [-m] [-p] [-s nn] [-t n] [-u] [-w nn] srcfile\n"
    "Where\n"
    "-a       debug: show production analysis\n"
    "-b       debug: show stack dump\n"
    "-l nn    set line count - default 0 i.e. first line is 1\n"
    "-g       debug: show generated intermediate code\n"
    "-m       turn off dump symbol table\n"
    "-p       turn off source code listing\n"
    "-s n     debug: show symbol information 0 (default) none, 1 symbols, 2 symbols & data\n"
    "-t n     set tab expansion 1 <= n <= 8. Expand tabs with spaces so column is a multiple of n\n"
    "-u       force upper case in strings\n"
    "-w nn    set output line width (min 72) - default 132\n"
    "srcfile  is the source file, of the form prefix.ext e.g. m80.plm\n"
    "         intermediate files prefix.lst, prefix.pol, prefix.sym are created\n"
    "         if the srcfile does not have an extension, .plm is added\n"
};

int main(int argc, char **argv) {

    for (int tokId = 0; tokId < 26; tokId++)
        contrl[tokId] = -1;

    C_ANALYZE  = 0;
    C_BYPASS   = 1;
    C_LINECNT  = 0;
    C_GENERATE = 0;
    C_MEMORY   = 1; // dump symbol table (0 = no, 1 = yes)
    C_PRINT    = 1; // print on 1 else off
    C_SYMBOLS  = 0;
    C_TAB      = 1;
    C_UPPER    = 0;
    C_WIDTH    = 132; // line width for listings

    while (getopt(argc, argv, "abl:gmps:t:uw:") != EOF) {
        switch (optopt) {
        case 'a':
            C_ANALYZE = !C_ANALYZE;
            break;
        case 'b':
            C_BYPASS = !C_BYPASS;
            break;
        case 'l':
            C_LINECNT = atoi(optarg);
            break;
        case 'g':
            C_GENERATE = !C_GENERATE;
            break;
        case 'm':
            C_MEMORY = !C_MEMORY;
            break;
        case 'p':
            C_PRINT = !C_PRINT;
            break;
        case 's':
            C_SYMBOLS = atoi(optarg);
            break;
        case 't':
            if ((C_TAB = atoi(optarg)) < 1 || C_TAB > 8)
                C_TAB = 1;
            break;
        case 'u':
            C_UPPER = !C_UPPER;
            break;
        case 'w':
            if ((C_WIDTH = atoi(optarg)) < 72)
                C_WIDTH = 72;
            break;
        }
    }
    if (optind != argc - 1)
        usage("Expected single source file");

    openfiles(argv[optind]);

    install();

    time_t now;
    time(&now);
    fprintf(lstFp, "         pl/m-8080 pass1 Version 4.0 - %s\n", ctime(&now));

    sp               = 4;
    pstack[sp].tokId = EOFILE;

    scan();         // prime the first token
    cloop();        // parse the program
    emit(NOP, OPR); // mark end

    if (errorCnt) {
        fprintf(lstFp, "\n\n%d PROGRAM ERROR%s\n\n", errorCnt, errorCnt != 1 ? "S" : "");
        fprintf(stdout, "\n\n%d PROGRAM ERROR%s\n\n", errorCnt, errorCnt != 1 ? "S" : "");
    } else {
        fputs("\n\nNO PROGRAM ERRORS\n\n", lstFp);
        fputs("\n\nNO PROGRAM ERRORS\n\n", stdout);
    }

    dumpsy();
    dumpch();
    dumpin();
    return errorCnt;
}

void dumpsym(int idx) {
    static char *types[]  = { "VARB", "INTR", "PROC", "LABEL", "LITER", "NUMBER" };
    static char *labels[] = { "OuterLabel", "LocalLabel", "CompilerLabel" };

    fprintf(stderr, "Symbol %s%d:", symbol[idx].outOfScope ? "-" : "", idx);
    int strId = symbol[idx].strId;
    int len   = symbol[idx].strLen;
    int type  = symbol[idx].type;
    int prec  = symbol[idx].prec;
    int cnt   = 0;
    switch (type) {
    case VARB:
    case LITER:
    case PROC:
    case LABEL:
    case INTR:
        if (strId) {
            fprintf(stderr, " '%.*s', ", len, idToStr(strId));
        } else
            fprintf(stderr, " strId=0, len=%d, ", len);

        if (type != LABEL || prec < OuterLabel || prec > CompilerLabel)
            fprintf(stderr, "type=%s prec=%d ilen=%d", types[type - VARB], prec, symbol[idx].len);
        else
            fprintf(stderr, "type=%s ilen=%d", labels[prec - OuterLabel], symbol[idx].len);
        if (symbol[idx].based)
            fprintf(stderr, ", BASED(%d)", symbol[idx].based);
        break;
    case NUMBER:
        fprintf(stderr, " strId=%d, len=%d, ", strId, len);
        fprintf(stderr, ", type=NUMBER, prec=%d, val=%d", prec, symbol[idx].len);
        break;
    default:
        fprintf(stderr, " strId=%d, len=%d, type=UNKNOWN(%d), prec=%d, ilen=%d", strId, len, type,
                prec, symbol[idx].len);
        break;
    }
    if (symbol[idx].hcode)
        fprintf(stderr, ", hcode=%d, next=%d", symbol[idx].hcode, symbol[idx].next);
    putc('\n', stderr);
}

void exitb() {

    /*     goes through here upon block exit */
    /*      global tables */
    if (curblk > 1) {
        int tokId = block[curblk];
        /* de-allocate those macro definitions whose scope we are leaving,
         * and check if any of these are currently in expansion.
         */
        dropMacro(macblk[curblk--]);

        for (int symIdx = symNext - 1; symIdx >= tokId; symIdx--) {

            if (!symbol[symIdx].outOfScope) { // in scope
                int type = symbol[symIdx].type;

                if (type < LITER) {
                    if (type == VARB || type == LABEL) {
                        if (symbol[symIdx].prec == 0) {
                            if (type == LABEL && curblk > 0) // labels may be non local
                                continue;                    // only fail if not defined at all
                            if (symbol[symIdx].strId)
                                error("1: undefined symbol '%.*s'", symbol[symIdx].strLen,
                                      idToStr(symbol[symIdx].strId));
                            else
                                error("1: undefined symbol ???");
                        }
                    }
                    symbol[symIdx].outOfScope = true; // mark as out of scope
                    /* remove from hash chain if on one*/

                    if (symbol[symIdx].hcode) { // has a hash chain

                        /* find match on the entry */
                        int hcode = symbol[symIdx].hcode;
                        int next  = symbol[symIdx].next;
                        int n     = hentry[hcode];
                        if (n == symIdx) /* this entry is directly connected */
                            hentry[hcode] = next;
                        else {
                            int np;
                            while ((np = symbol[n].next) != symIdx)
                                n = np;
                            symbol[n].next = next;
                        }
                    }
                }
            }
        }
        blksym = block[curblk];
    }
}

int varHash(uint8_t *str, int len) {
    int hval = 0;
    for (int tokId = 0; tokId < len; tokId++)
        hval = (hval << 1) + (hval >> 7) + str[tokId];
    return hval;
}

int lookup(const int iv) {

    symlen = pstack[iv].len;
    symloc = pstack[iv].loc;

    hcode  = (pstack[iv].tokId == NUMBV ? pstack[iv].fixv : varHash(&varc[symloc], symlen)) % 127 +
            1; // hash code

    /*     hcode is in the range 1 to 127 */
    // hentry items point to hash chain, symbol base is +2 from this

    for (int symIdx = hentry[hcode]; symIdx > 0; symIdx = symbol[symIdx].next) {

        if (pstack[iv].tokId == NUMBV) {
            if (symbol[symIdx].type > LITER && symbol[symIdx].len == pstack[iv].fixv)
                return symIdx;
        } else if (!symbol[symIdx].outOfScope && symbol[symIdx].strLen == symlen) {
            if (strequ(symbol[symIdx].strId, &varc[symloc], symlen)) {
                /*     make sure the types match. */
                if ((pstack[iv].tokId == STRV && symbol[symIdx].type == LITER) ||
                    (pstack[iv].tokId == IDENTV && symbol[symIdx].type < LITER))
                    return symIdx;
            }
        }
    }

    return 0;
}

int enter(uint16_t len, uint8_t prec, uint8_t type, bool hasHash) {

    /*      enter assumes a previous call to lookup (either that, or set up */
    /*      the values of symloc and symlen in the varc array). */
    /*         also set-up hash code value (see lookup), if necessary */
    int symIdx = symNext++;

    if (symIdx >= SYMABS)
        fatal("2: pass-1 symbol table overflow");

    if (hasHash && type != NUMBER) {
        symbol[symIdx].strLen = symlen;
        symbol[symIdx].strId  = newString(symlen, &varc[symloc]);
    }
    setInfo(symIdx, len, prec, type);

    if (hasHash) {
        // add to the front of the hash chain
        symbol[symIdx].hcode = hcode;
        symbol[symIdx].next  = hentry[hcode];
        hentry[hcode]        = symIdx;
    }

    return symIdx;
}

void install() {
    symloc = 0;
    for (int tokId = 0; tokId < ASIZE(builtins); tokId++) {
        strcpy(varc, builtins[tokId].name);
        hcode = varHash(varc, symlen = (int)strlen(builtins[tokId].name)) % 127 + 1; // hash code
        enter(builtins[tokId].len, builtins[tokId].prec, builtins[tokId].type, true);
    }
    block[1] = blksym = symNext;
}

void recov() {
    for (;;) {
        /*     find something solid in the text */
        if (token == DECL || token == PROCV || token == ENDV || token == DOV || token == SEMIV ||
            token == EOFILE)
            for (;;) {
                /*     and in the stack */
                int top = pstack[sp].tokId;
                if (failsf && getc1(top, token) != 0) {
                    failsf = false;
                    return;
                }
                if (top == EOFILE && token == EOFILE) {
                    failsf = compil = false;
                    return;
                }
                if (token != EOFILE &&
                    (top == GROUPV || top == SLISTV || top == STMTV || top == DOV || top == PROCV))
                    break;
                /*         but don't go too far */
                if (sp <= 4)
                    break;
                vartop = pstack[sp--].loc;
            }
        scan();
    }
}

// simple compare for bsearch
int intcmp(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

bool stack(/*const int q */) {
    int i;

    for (;;) {
        ;
        switch (getc1(pstack[sp].tokId, token)) {
        case 0:
            /*     illegal symbol pair */
            error("3: invalid PL/M statement. %s & %s not allowed together",
                  tokens[pstack[sp].tokId], tokens[token]);
            sdump();
            recov();
            /*     recover may have set compiling false */
            if (!compil)
                return true;
            break;
        case 1:
            return true;
        case 2:
            return false;
        case 3:
            i = TRI(pstack[sp - 1].tokId, pstack[sp].tokId, token);
            return bsearch(&i, c1tri, ASIZE(c1tri), sizeof(c1tri[0]), intcmp) != NULL;
        }
    }
}

bool prok(const int prd) {
    /*      context check of equal or imbedded right parts */
    if (contc[prd] == 0)
        return true;

    int k         = hdtb[prd] - nt;
    int tokId     = pstack[sp - prlen[prd]].tokId;
    int first     = lefti[k - 1];
    int nextFirst = lefti[k];
    if (first < nextFirst)
        for (int i = first; i < nextFirst; i++)
            if (leftc[i] == tokId)
                return true;
    return false;
}

void reduce() {
    /*     pack stack top */
    int i   = TRI(pstack[sp - 3].tokId, pstack[sp - 2].tokId, pstack[sp - 1].tokId);

    int top = pstack[sp].tokId;

    for (int prd = prind[top - 1]; prd < prind[top]; prd++) {
        int m = i & ((1 << 8 * (prlen[prd] - 1)) - 1); // items to compare
        if (m == prtb[prd] && prok(prd)) {
            mp = sp - prlen[prd] + 1;     // mp -> lowest item on stack
            synth(prdtb[prd], hdtb[prd]); // do the action
            sp               = mp;        // do the reduce
            pstack[sp].tokId = hdtb[prd];
            vartop           = pstack[sp].loc;
            return;
        }
    }
    /*     no applicable production */
    error("4: badly formed PL/M statement");
    failsf = false;
    sdump();
    recov();
    return;
}

void cloop() {
    /*      global tables */
    compil = true;
    while (compil) {
        if (!stack())
            reduce();
        else {
            /*     stack may have set compiling false */
            if (!compil)
                break;
            if (++sp >= PSTACK)
                fatal("5: pass-1 parse stack overflow");
            pstack[sp].tokId = token;
            /*     insert accum into varc here */
            if (token == NUMBV)
                pstack[sp].fixv = accumIVal;

            pstack[sp].loc = vartop;
            for (int i = 0; i < acclen; i++) {
                varc[vartop] = accum[i];
                if (++vartop > MVAR)
                    fatal("7: pass-1 table overflow");
            }
            pstack[sp].len = vartop < pstack[sp].loc ? 1 : vartop - pstack[sp].loc;
            scan();
        }
    }
}

/*
    getc1(stackitem, token)
    returns action code
    0 - illegal combination
    1 - stack
    2 - reduce
    3 - look at top three items is match in c1tri then stack else reduce
*/
int getc1(int stackItem, int token) {
    return (c1[stackItem - 1][token / 4] >> ((token % 4) << 1)) & 3;
}

void procHead(int plist, int rtype) {
    proctp[curblk] = rtype ? 2 : 1; // 1 untyped, 2 typed
    int symIdx     = pstack[mp].fixv;
    setInfo(symIdx, plist, rtype, PROC); // set symbol info
    int node = enter(0, LocalLabel, LABEL, false);
    pstack[mp].fixv += node << 15;
    emit(node, VLU);
    emit(TRA, OPR);
    emit(symIdx, DEF);
    return;
}

void synth(const int prod, const int newId) {
    int tokId, i, k, len, dim, type;
    int symIdx;

    /*    mp == left ,  sp == right */

    if (C_ANALYZE)
        redpr(prod, newId);
    switch (prod) {
    case 1: // <PROGRAM> ::= <STATEMENT LIST>
        if (mp != 5)
            error("10: invalid program. Possibly missing earlier END");
        compil = false;
        exitb();
        return;
    case 2: // <STATEMENT LIST> ::= <STATEMENT>
    case 3: // <STATEMENT LIST> ::= <STATEMENT LIST> <STATEMENT>
    case 4: // <STATEMENT> ::= <BASIC STATEMENT>
    case 5: // <STATEMENT> ::= <IF STATEMENT>
        return;
    case 6: // <BASIC STATEMENT> ::= <ASSIGNMENT> ';'
        while (acnt > 0) {
            symIdx = assign[acnt];
            if (symIdx <= 0)
                emit(XCH, OPR);
            else
                emit(symIdx, ADR);
            if (--acnt > 0)
                emit(STO, OPR);
        }
        emit(STD, OPR);
        return;
    case 7:  // <BASIC STATEMENT> ::= <GROUP> ';'
    case 9:  // <BASIC STATEMENT> ::= <RETURN STATEMENT> ';'
    case 10: // <BASIC STATEMENT> ::= <CALL STATEMENT> ';'
    case 11: // <BASIC STATEMENT> ::= <GO TO STATEMENT> ';'
        return;

    case 8:  // <BASIC STATEMENT> ::= <PROCEDURE DEFINITION> ';'
    case 12: // <BASIC STATEMENT> ::= <DECLARATION STATEMENT> ';'
        if (dopar[curblk].type != DO_GROUP)
            error("11: invalid declaration");
        return;
    case 13: // <BASIC STATEMENT> ::= HALT ';'
        emit(HAL, OPR);
        return;
    case 14: // <BASIC STATEMENT> ::= ENABLE ';'
        emit(ENA, OPR);
        return;
    case 15: // <BASIC STATEMENT> ::= DISABLE ';'
        emit(DIS, OPR);
        return;
    case 16: // <BASIC STATEMENT> ::= ';'
    case 17: // <BASIC STATEMENT> ::= <LABEL DEFINITION> <BASIC STATEMENT>
        return;
    case 18: // <IF STATEMENT> ::= <IF CLAUSE> <STATEMENT>
             // fall through to common code
    case 19: // <IF STATEMENT> ::= <IF CLAUSE> <TRUE PART> <STATEMENT>
        tokId = pstack[mp].fixv;
        emit(tokId, DEF);
        setInfo(tokId, 0, LocalLabel, LABEL); // set symbol info
        return;
    case 20: // <IF STATEMENT> ::= <LABEL DEFINITION> <IF STATEMENT>
        return;
    case 21: // <IF CLAUSE> ::= IF <EXPRESSION> THEN
        tokId = enter(0, 0, LABEL, false);
        emit(tokId, VLU);
        emit(TRC, OPR);
        pstack[mp].fixv = tokId;
        return;
    case 22: // <TRUE PART> ::= <BASIC STATEMENT> ELSE
        tokId = enter(0, 0, LABEL, false);
        emit(tokId, VLU);
        emit(TRA, OPR);
        i                   = pstack[mp - 1].fixv;
        pstack[mp - 1].fixv = tokId;
        tokId               = i;
        emit(tokId, DEF);
        setInfo(tokId, 0, LocalLabel, LABEL); // set symbol info
        return;
    case 23: // <GROUP> ::= <GROUP HEAD> <ENDING>
        if (pstack[sp].fixv > 0)
            error("12: improper use of identifier following END");
        else if (pstack[sp].fixc < 0)
            pstack[mp].fixc = 0;

        switch (dopar[curblk + 1].type) {
        case DO_GROUP:
            emit(END, OPR);
            return;
        case DO_ITER:
            /*     end of iterative statement */
            k = pstack[mp].fixv;
            if (k != 0) {
                /*     otherwise increment variable */
                emit(k, VLU);
                emit(INC, OPR);
                emit(k, ADR);
                emit(STD, OPR);
            }
            break;
        case DO_WHILE:
            break;
        case DO_CASE:
            {
                /*     generate destination of case branch */
                symIdx = dopar[curblk + 1].clause;
                emit(symIdx, DEF);
                int endJmp         = symbol[symIdx].len;
                symbol[symIdx].len = 0;
                /*     m is symbol number of LABEL at end of jump table */
                emit(CSE, OPR);
                /*     define the jump table */
                /*     reverse the LABEL list */
                int pIdx = 0, qIdx;
                for (symIdx = dopar[curblk + 1].label; symIdx; pIdx = symIdx, symIdx = qIdx) {
                    qIdx               = symbol[symIdx].len;
                    symbol[symIdx].len = pIdx;
                }

                do { /* emit list starting at pIdx */
                    qIdx = symbol[pIdx].len;
                    setInfo(pIdx, 0, LocalLabel, LABEL); // set symbol info
                    if (qIdx) {
                        emit(pIdx, VLU);
                        emit(AX2, OPR);
                    }
                } while ((pIdx = qIdx));
                /*     define end of jump table */
                emit(endJmp, DEF);
            }

            return;
        }
        /*     define end of while statement */
        emit(dopar[curblk + 1].label, VLU);
        emit(TRA, OPR);
        emit(dopar[curblk + 1].clause, DEF);
        return;
    case 24: // <GROUP HEAD> ::= DO ';'
        enterb();
        emit(ENB, OPR);
        return;
    case 25: // <GROUP HEAD> ::= DO <STEP DEFINITION> ';'
        enterb();
        doHead(curblk, DO_ITER, pstack[mp + 1].fixv);
        return;
    case 26: // <GROUP HEAD> ::= DO <WHILE CLAUSE> ';'
        enterb();
        doHead(curblk, DO_WHILE, pstack[mp + 1].fixv);
        return;
    case 27: // <GROUP HEAD> ::= DO <CASE SELECTOR> ';'
        enterb();
        k = enter(0, LocalLabel, LABEL, false);
        /*     k is LABEL after case jump table */
        symIdx = enter(k, LocalLabel, LABEL, false);
        emit(symIdx, VLU);
        emit(AX1, OPR);
        doHead(curblk, DO_CASE, symIdx);
        symIdx              = enter(0, LocalLabel, LABEL, false);
        dopar[curblk].label = symIdx;
        emit(symIdx, DEF);
        return;
    case 28:                                 // <GROUP HEAD> ::= <GROUP HEAD> <STATEMENT>
        if (dopar[curblk].type == DO_CASE) { // case stmt
            emit(symbol[dopar[curblk].clause].len, VLU);
            emit(TRA, OPR);
            symIdx              = enter(dopar[curblk].label, LocalLabel, LABEL, false);
            dopar[curblk].label = symIdx;
            emit(symIdx, DEF);
        }
        return;
    case 29: // <STEP DEFINITION> ::= <VARIABLE> <REPLACE> <EXPRESSION> <ITERATION CONTROL>
        i = pstack[mp + 3].fixv;
        /*     place <variable> symbol number into do slot */
        pstack[mp - 1].fixv = i >= 0 ? 0 : pstack[mp].fixv;
        pstack[mp].fixv     = iabs(i);
        return;
    case 30: // <ITERATION CONTROL> ::= <TO> <EXPRESSION>
        emit(LEQ, OPR);
        tokId = enter(0, LocalLabel, LABEL, false);
        emit(tokId, VLU);
        emit(TRC, OPR);
        pstack[mp].fixv = -((pstack[mp].fixv << 14) + tokId);
        return;
    case 31: // <ITERATION CONTROL> ::= <TO> <EXPRESSION> <BY> <EXPRESSION>
        tokId = pstack[mp - 3].fixv;
        /*     i = symbol number of indexing variable */
        emit(tokId, VLU);
        emit(ADD, OPR);
        emit(tokId, ADR);
        emit(STD, OPR);
        /*     branch to compare */
        tokId = pstack[mp + 2].fixv;
        emit((tokId >> 14), VLU);
        emit(TRA, OPR);
        /*     define beginning of statements */
        emit(tokId & 0x3fff, DEF);
        return;
    case 32: // <WHILE CLAUSE> ::= <WHILE> <EXPRESSION>
        tokId           = enter(0, LocalLabel, LABEL, false);
        pstack[mp].fixv = (pstack[mp].fixv << 14) + tokId;
        /*     (back branch number/end loop number) */
        emit(tokId, VLU);
        emit(TRC, OPR);
        return;
    case 33: // <CASE SELECTOR> ::= CASE <EXPRESSION>
        return;
    case 34: // <PROCEDURE DEFINITION> ::= <PROCEDURE HEAD> <STATEMENT LIST> <ENDING>
        k = (pstack[mp].fixv >> 15);
        i = pstack[sp].fixv;
        if (i < 0)
            i = -i + 1;
        if (i != 0 && (pstack[mp].fixv & 0x7fff) != i)
            error("13: identifier following END does not match");
        emit(END, OPR);
        /*     emit a ret just in case he forgot it */
        emit(DRT, OPR);
        emit(k, DEF);
        return;
    case 35: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> ';'
        procHead(0, 0);
        return;
    case 36: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> <TYPE> ';'
        procHead(0, pstack[sp - 1].fixv);
        return;
    case 37: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> <PARAMETER LIST> ';'
        procHead(pstack[mp + 1].fixv, 0);
        return;
    case 38: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> <PARAMETER LIST> <TYPE> ';'
        procHead(pstack[mp + 1].fixv, pstack[sp - 1].fixv);
        return;
    case 39:                      // <PROCEDURE HEAD> ::= <PROCEDURE NAME> INTERRUPT <NUMBER> ';'
        symIdx = pstack[mp].fixv; // procedure symbolId
        i      = pstack[sp - 1].fixv; // interrupt number
        if (i > 7)
            error("39: invalid interrupt number");
        else if (intpro[i] <= 0)
            intpro[i] = symIdx;
        else
            error("40: duplicate interrupt procedure");

        procHead(0, 0);
        return;
    case 40: // <PROCEDURE NAME> ::= <LABEL DEFINITION> PROCEDURE
             /* check for numeric label */
        if (pstack[mp].fixc >= 0) {
            pstack[mp].fixv = pstack[mp].fixc;
            error("48: invalid procedure name");
        }
        enterb();
        emit(ENP, OPR);
        return;
    case 41: // <PARAMETER LIST> ::= <PARAMETER HEAD> <IDENTIFIER> ')'
    case 43: // <PARAMETER HEAD> ::= <PARAMETER HEAD> <IDENTIFIER> ','
        symIdx = lookup(sp - 1);
        if (symIdx >= blksym)
            error("14: duplicate formal parameter name");
        enter(0, 0, VARB, true);
        pstack[mp].fixv++;
        return;
    case 42: // <PARAMETER HEAD> ::= '(' Left context check(<PROCEDURE NAME>)
        pstack[mp].fixv = 0;
        return;
    case 44: // <ENDING> ::= END
        exitb();
        pstack[mp].fixv = 0;
        return;
    case 45: // <ENDING> ::= END <IDENTIFIER>
        exitb();
        if (!(symIdx = lookup(sp)))
            error("15: identifier following END not found");
        pstack[mp].fixv = symIdx;
        return;
    case 46: // <ENDING> ::= <LABEL DEFINITION> <ENDING>
        pstack[mp].fixv = pstack[sp].fixv;
        return;
    case 47: // <LABEL DEFINITION> ::= <IDENTIFIER> ':'
        symIdx = lookup(mp);
        if (symIdx <= blksym)
            symIdx = enter(0, curblk == 1 ? OuterLabel : LocalLabel, LABEL, true);
        else {
            if (symbol[symIdx].prec)
                error("16: duplicate LABEL definition in block");

            symbol[symIdx].prec = curblk == 1 ? OuterLabel : LocalLabel;
        }
        pstack[mp].fixv = symIdx;
        /* indicate that this is an identifier label */
        pstack[mp].fixc = -1;
        if (token != PROCV)
            emit(symIdx, DEF);
        return;
    case 48: // <LABEL DEFINITION> ::= <NUMBER> ':'
        k = pstack[mp].fixv;
        if (k > 65535)
            error("17: numeric LABEL too big");
        else {
            if ((symIdx = lookup(mp)) == 0)
                /*     enter number */
                symIdx = enter(k, k > 255 ? 2 : 1, NUMBER, true);

            /* indicate that this is a numeric label */
            pstack[mp].fixc = symIdx;
            emit(symIdx, VLU);
            emit(ORG, OPR);
        }
        return;
    case 49: // <RETURN STATEMENT> ::= RETURN
        emit(0, LIT);
        if (proctp[curblk] == 2)
            error("45: missing return value for typed procedure");
        else if (proctp[curblk] == 0)
            error("46: return outside procedure definition");
        emit(RET, OPR);
        return;
    case 50: // <RETURN STATEMENT> ::= RETURN <EXPRESSION>
        if (proctp[curblk] == 1)
            error("44: return value invalid for untyped procedure");
        else if (proctp[curblk] == 0)
            error("46: return outside procedure definition");
        emit(RET, OPR);
        return;
    case 51: // <CALL STATEMENT> ::= CALL <VARIABLE>
        if ((symIdx = pstack[sp].fixv) == 0)
            return;
        if (symIdx > 0) {
            int type = symbol[symIdx].type;
            emit(symIdx, ADR);
            if (type == PROC) {
                emit(PRO, OPR);
                return;
            } else if (type == INTR) {
                emit(BIF, OPR);
                return;
            }
        }
        error("18: invalid CALL statement");
        return;
    case 52: // <GO TO STATEMENT> ::= <GO TO> <IDENTIFIER>
        if ((symIdx = lookup(sp)) == 0)
            symIdx = enter(0, 0, LABEL, true);
        i = symbol[symIdx].type;
        if (i == LABEL || i == VARB) {
            if (i == LABEL) // bump the reference counter
                symbol[symIdx].refCnt++;
            emit(symIdx, VLU);
            emit(TRA, OPR);
        } else
            error("19: invalid destination for GOTO");
        return;
    case 53: // <GO TO STATEMENT> ::= <GO TO> <NUMBER>
        k = pstack[sp].fixv;
        if (k > 65535) {
            error("17: numeric LABEL too big");
        } else {
            if ((symIdx = lookup(sp)) == 0) /*     enter number */
                symIdx = enter(k, k > 255 ? 2 : 1, NUMBER, true);
            emit(symIdx, VLU);
            emit(TRA, OPR);
        }
        return;
    case 54: // <GO TO> ::= GO TO
    case 55: // <GO TO> ::= GOTO
    case 56: // <DECLARATION STATEMENT> ::= DECLARE <DECLARATION ELEMENT>
    case 57: // <DECLARATION STATEMENT> ::= <DECLARATION STATEMENT> ',' <DECLARATION
             // ELEMENT>
    case 58: // <DECLARATION ELEMENT> ::= <TYPE DECLARATION>
        return;
    case 59: // <DECLARATION ELEMENT> ::= <IDENTIFIER> LITERALLY <STRING>
        if (!defMacro(pstack[mp].len, &varc[pstack[mp].loc], pstack[sp].len, &varc[pstack[sp].loc]))
            fatal("20: macro table overflow");
        return;
    case 60: // <DECLARATION ELEMENT> ::= <IDENTIFIER> <DATA LIST>
        symIdx = pstack[mp].fixv;
        i     = pstack[mp + 1].fixv;
        symbol[symIdx].len += i & 0xffff;
        emit(DAT, OPR);
        emit((i >> 16), DEF);
        return;
    case 61: // <DATA LIST> ::= <DATA HEAD> <CONSTANT> ')'
    case 63: // <DATA HEAD> ::= <DATA HEAD> <CONSTANT> ','
        pstack[mp].fixv += wrdata(-pstack[mp + 1].fixv);
        return;
    case 62: // <DATA HEAD> ::= DATA '('
        i = enter(0, LocalLabel, LABEL, false);
        emit(i, VLU);
        emit(TRA, OPR);
        pstack[mp].fixv = (i << 16);
        if (lookup(mp - 1) > blksym)
            error("22: duplicate variable declaration");
        /*     set precision of inline data to 3 */
        symIdx              = enter(0, P_INLINE, VARB, true);
        pstack[mp - 1].fixv = symIdx;
        emit(DAT, OPR);
        emit(symIdx, DEF);
        return;
    case 64: // <TYPE DECLARATION> ::= <IDENTIFIER SPECIFICATION> <TYPE>
    case 65: // <TYPE DECLARATION> ::= <BOUND HEAD> <NUMBER> ')' <TYPE>
        dim = prod == 64 ? 1 : pstack[mp + 1].fixv; // dimension
        k   = pstack[sp].fixv;                      // item type
        do {
            int symIdx = idList[--idListSP];
            if (symbol[symIdx].type != VARB || symbol[symIdx].prec || symbol[symIdx].dim)
                error("21: duplicate variable or label definition");
            setInfo(symIdx, dim, k, k == P_LABEL ? LABEL : VARB); // set symbol info
        } while (idListSP);

        pstack[mp].fixv = idList[0];
        return;
    case 66: // <TYPE DECLARATION> ::= <TYPE DECLARATION> <INITIAL LIST>
        return;
    case 67: // <TYPE> ::= BYTE
        pstack[mp].fixv = P_BYTE;
        return;
    case 68: // <TYPE> ::= ADDRESS
        pstack[mp].fixv = P_ADDRESS;
        return;
    case 69: // <TYPE> ::= LABEL
        pstack[mp].fixv = P_LABEL;
        return;
    case 70: // <BOUND HEAD> ::= <IDENTIFIER SPECIFICATION> '('
        return;
    case 71:                                  // <IDENTIFIER SPECIFICATION> ::= <VARIABLE NAME>
        idList[idListSP++] = pstack[mp].fixv; // single so no need to check for overflow
        return;
    case 72: // <IDENTIFIER SPECIFICATION> ::= <IDENTIFIER LIST> <VARIABLE NAME> ')'
    case 74: // <IDENTIFIER LIST> ::= <IDENTIFIER LIST> <VARIABLE NAME> ','
        if (idListSP < MAXIDLIST)
            idList[idListSP++] = pstack[mp + 1].fixv;
        else
            fatal("23: pass-1 factored list overflow");

        return;
    case 73: // <IDENTIFIER LIST> ::= '('
        return;
    case 75: // <VARIABLE NAME> ::= <IDENTIFIER> Left context check(',' | DECLARE |
             // <IDENTIFIER LIST>)
    case 77: // <BASED VARIABLE> ::= <IDENTIFIER> BASED
        symIdx = lookup(mp);
        if (symIdx < blksym)
            symIdx = enter(0, 0, VARB, true);
        else if (symbol[symIdx].type != VARB || symbol[symIdx].prec)
            error("24: invalid variable identifier");
        pstack[mp].fixv = symIdx;
        return;
    case 76: // <VARIABLE NAME> ::= <BASED VARIABLE> <IDENTIFIER>
        if (symNext >= SYMABS)
            fatal("25: pass-1 symbol table overflow");
        else {
            tokId  = pstack[mp].fixv;
            symIdx = lookup(sp);
            if (symIdx == 0)
                symIdx = enter(0, 0, VARB, true);
            else if (symbol[symIdx].type != VARB) {
                error("26: badly formed BASED variable declaration");
                return;
            }

            symbol[tokId].based = symIdx; // stuff the symId of the base var
        }
        return;
        /*
         * Modified from original code
         * separate space for initial data, now built upwards see notes
         * just before definition of initialData variable
         */

    case 79: // <INITIAL HEAD> ::= INITIAL '('
        if (initialDataSP < MAXINITIAL) {
            initialData[initialDataSP] = pstack[mp - 1].fixv << 15;
            pstack[mp].fixv            = initialDataSP++;
        } else
            fatal("23: pass-1 initial data table overflow");
        return;
    case 78: // <INITIAL LIST> ::= <INITIAL HEAD> <CONSTANT> ')'
    case 80: // <INITIAL HEAD> ::= <INITIAL HEAD> <CONSTANT> ','
        if (initialDataSP < MAXINITIAL) {
            initialData[pstack[mp].fixv]++; // bump the count
            initialData[initialDataSP++] =
                (pstack[mp + 1].fixv << 16) + pstack[mp + 1].fixv; // store the value
        } else
            fatal("23: pass-1 initial data table overflow");
        return;

    case 81: // <ASSIGNMENT> ::= <VARIABLE> <REPLACE> <EXPRESSION>
    case 82: // <ASSIGNMENT> ::= <LEFT PART> <ASSIGNMENT>
        if (++acnt > MAXASSIGN)
            fatal("50: too many chained assignments");
        else {
            assign[acnt] = pstack[mp].fixv;
            /*      check for procedure on lhs of assignment. */
            /*     ****note that this is dependent on symbol number of output=17**** */
            if (pstack[mp].fixv == 0 && pstack[mp].fixc != 17)
                error("41: procedure on left-hand side of an assignment");
        }
        return;
    case 83: // <REPLACE> ::= '='
    case 84: // <LEFT PART> ::= <VARIABLE> ','
    case 85: // <EXPRESSION> ::= <LOGICAL EXPRESSION>
        return;
    case 86: // <EXPRESSION> ::= <VARIABLE> ':' '=' <LOGICAL EXPRESSION>
        symIdx = pstack[mp].fixv;
        if (pstack[mp].fixv == 0)
            error("41: procedure on left-hand side of an assignment");
        else if (symIdx < 0) // PMO added 'else' as symIdx = 0 will cause memory error
            emit(XCH, OPR);
        else
            emit(symIdx, ADR);

        emit(STO, OPR);
        return;
    case 87: // <LOGICAL EXPRESSION> ::= <LOGICAL FACTOR>
        return;
    case 88: // <LOGICAL EXPRESSION> ::= <LOGICAL EXPRESSION> OR <LOGICAL FACTOR>
        emit(IOR, OPR);
        return;
    case 89: // <LOGICAL EXPRESSION> ::= <LOGICAL EXPRESSION> XOR <LOGICAL FACTOR>
        emit(XOR, OPR);
        return;
    case 90: // <LOGICAL FACTOR> ::= <LOGICAL SECONDARY>
        return;
    case 91: // <LOGICAL FACTOR> ::= <LOGICAL FACTOR> AND <LOGICAL SECONDARY>
        emit(AND, OPR);
        return;
    case 92: // <LOGICAL SECONDARY> ::= <LOGICAL PRIMARY>
        return;
    case 93: // <LOGICAL SECONDARY> ::= NOT <LOGICAL PRIMARY>
        emit(NOT, OPR);
        return;
    case 94: // <LOGICAL PRIMARY> ::= <ARITHMETIC EXPRESSION>
        return;
    case 95: // <LOGICAL PRIMARY> ::= <ARITHMETIC EXPRESSION> <RELATION> <ARITHMETIC
             // EXPRESSION>
        emit(pstack[mp + 1].fixv, OPR);
        return;
    case 96: // <RELATION> ::= '=' Left context check(<ARITHMETIC EXPRESSION>)
        pstack[mp].fixv = EQL;
        return;
    case 97: // <RELATION> ::= '<'
        pstack[mp].fixv = LSS;
        return;
    case 98: // <RELATION> ::= '>'
        pstack[mp].fixv = GTR;
        return;
    case 99: // <RELATION> ::= '<' '>'
        pstack[mp].fixv = NEQ;
        return;
    case 100: // <RELATION> ::= '<' '='
        pstack[mp].fixv = LEQ;
        return;
    case 101: // <RELATION> ::= '>' '='
        pstack[mp].fixv = GEQ;
        return;
    case 102: // <ARITHMETIC EXPRESSION> ::= <TERM>
        return;
    case 103: // <ARITHMETIC EXPRESSION> ::= <ARITHMETIC EXPRESSION> '+' <TERM>
        emit(ADD, OPR);
        return;
    case 104: // <ARITHMETIC EXPRESSION> ::= <ARITHMETIC EXPRESSION> '-' <TERM>
        emit(SUB, OPR);
        return;
    case 105: // <ARITHMETIC EXPRESSION> ::= <ARITHMETIC EXPRESSION> PLUS <TERM>
        emit(ADC, OPR);
        return;
    case 106: // <ARITHMETIC EXPRESSION> ::= <ARITHMETIC EXPRESSION> MINUS <TERM>
        emit(SBC, OPR);
        return;
    case 107: // <ARITHMETIC EXPRESSION> ::= '-' <TERM>
        emit(0, LIT);
        emit(XCH, OPR);
        emit(SUB, OPR);
        return;
    case 108: // <TERM> ::= <PRIMARY>
        return;
    case 109: // <TERM> ::= <TERM> '*' <PRIMARY>
        emit(MUL, OPR);
        return;
    case 110: // <TERM> ::= <TERM> '/' <PRIMARY>
        emit(DIV, OPR);
        return;
    case 111: // <TERM> ::= <TERM> MOD <PRIMARY>
        emit(REM, OPR);
        return;
    case 112: // <PRIMARY> ::= <CONSTANT>
        symIdx = pstack[mp].fixv;
        emit(symIdx, VLU);
        return;
    case 113: // <PRIMARY> ::= '.' <CONSTANT>
        tokId           = enter(0, LocalLabel, LABEL, false);
        pstack[mp].fixv = tokId;
        emit(tokId, VLU);
        emit(TRA, OPR);
        emit(DAT, OPR);
        emit(0, DEF);
        /*     drop through to next production */
    case 114: // <PRIMARY> ::= <CONSTANT HEAD> <CONSTANT> ')'
        wrdata(-pstack[mp + 1].fixv);
        emit(DAT, OPR);
        emit(pstack[mp].fixv, DEF);
        return;
    case 115: // <PRIMARY> ::= <VARIABLE>
        symIdx = pstack[mp].fixv;
        if (symIdx > 0) {
            /*     simple variable */
            emit(symIdx, VLU);
            int type = symbol[symIdx].type;
            if (type == PROC)
                emit(PRO, OPR);
            else if (type == INTR)
                emit(BIF, OPR);
        } else if (symIdx != 0)
            /*     subscripted variable */
            emit(LOD, OPR);
        return;
    case 116: // <PRIMARY> ::= '.' <VARIABLE>
        symIdx = pstack[sp].fixv;
        if (symIdx > 0) {
            if (symbol[symIdx].type == VARB) {
                emit(symIdx, ADR);
                emit(CVA, OPR); /*     subscripted - change precision to 2 */
                return;
            }
        } else if (symIdx != 0) { /*     subscripted - change precision to 2 */
            emit(CVA, OPR);
            return;
        }
        error("28: invalid address reference");
        return;
    case 117: // <PRIMARY> ::= '(' <EXPRESSION> ')'
        return;
    case 118: // <CONSTANT HEAD> ::= '.' '('
        symIdx          = enter(0, LocalLabel, LABEL, false);
        tokId           = symIdx;
        pstack[mp].fixv = tokId;
        emit(tokId, VLU);
        emit(TRA, OPR);
        emit(DAT, OPR);
        emit(0, DEF);
        return;
    case 119: // <CONSTANT HEAD> ::= <CONSTANT HEAD> <CONSTANT> ','
        wrdata(-pstack[mp + 1].fixv);
        return;
    case 120: // <VARIABLE> ::= <IDENTIFIER>
        if ((symIdx = lookup(mp)) == 0) {
            error("29: undeclared variable");
            symIdx = enter(0, 0, VARB, true);
        }
        pstack[mp].fixv = symIdx;
        i               = symbol[symIdx].type;
        if (i == LABEL)
            error("47: illegal use of a LABEL");
        if (i != PROC && i != INTR)
            return;
        if (symbol[symIdx].len != 0)
            error("38: too few parameters");
        i = symbol[symIdx].prec;
        if (pstack[mp - 1].tokId == CALLV && i != 0)
            error("42: attempted CALL of a typed procedure");
        if (pstack[mp - 1].tokId != CALLV && i == 0)
            error("43: attempted use of untyped procedure as a function or variable");
        tokId           = symIdx;
        tokId           = ((tokId << 15) + tokId + 1);
        pstack[mp].fixc = 0;
        emit(tokId >> 15, VLU);
        pstack[mp].fixc = tokId >> 15;
        pstack[mp].fixv = 0;
        emit(PRO, OPR);
        return;
    case 121: // <VARIABLE> ::= <SUBSCRIPT HEAD> <EXPRESSION> ')'
        tokId = pstack[mp].fixv;
        if (tokId >= 0) {
            pstack[mp].fixv = -tokId;
            emit(INX, OPR);
            return;
        } else {
            tokId = -tokId;
            emit(tokId & 0x7fff, ADR);
            if (pstack[mp].fixc != 1)
                emit(STD, OPR);
            if (pstack[mp].fixc == 0)
                error("37: too many actual parameters");
            else if (iabs(pstack[mp].fixc) > 1)
                error("38: too few parameters");
        }
        emit(tokId >> 15, VLU);
        pstack[mp].fixc = tokId >> 15;
        pstack[mp].fixv = 0;
        emit(PRO, OPR);
        return;
    case 122: // <SUBSCRIPT HEAD> ::= <IDENTIFIER> '('
        if ((symIdx = lookup(mp)) == 0) {
            error("30: subscripted variable or procedure call references undeclared "
                  "identifier");
            symIdx = enter(0, 0, VARB, true);
        }
        type = symbol[symIdx].type;
        if (type != VARB) {
            if (type != PROC && type != INTR)
                error("31: Identifier is improperly used as a procedure or subscripted "
                      "variable");
            else {
                pstack[mp].fixc = symbol[symIdx].len;
                if (type == INTR)
                    pstack[mp].fixc = -pstack[mp].fixc;
                int rtype = symbol[symIdx].prec;
                if (pstack[mp - 1].tokId == CALLV && rtype != 0)
                    error("42: attempted CALL of a typed procedure");
                if (pstack[mp - 1].tokId != CALLV && rtype == 0)
                    error("43: attempted use of untyped procedure as a function or variable");
                pstack[mp].fixv = -((symIdx << 15) + symIdx + 1);
                return;
            }
        }
        pstack[mp].fixv = symIdx;
        emit(symIdx, ADR);
        return;
    case 123: // <SUBSCRIPT HEAD> ::= <SUBSCRIPT HEAD> <EXPRESSION> ','
        tokId = -pstack[mp].fixv;
        if (tokId <= 0)
            error("32: too many subscripts. Only one allowed");
        else {
            pstack[mp].fixv = -(tokId + 1);
            i               = tokId & 0x7fff;
            emit(i, ADR);
            if (pstack[mp].fixc == 0)
                error("37: too many actual parameters");
            else {
                if (pstack[mp].fixc != 2)
                    emit(STD, OPR);
                pstack[mp].fixc += (pstack[mp].fixc < 0) ? 1 : -1;
            }
        }
        return;
    case 124: // <CONSTANT> ::= <STRING>
        /*     may wish to treat this string as a constant later */
        len = pstack[sp].len;
        if (0 < len && len <= 2) {
            i = pstack[sp].loc;
            k = varc[i];
            if (len == 2)
                k = k * 256 + varc[i + 1];
        } else {
            len = 3;
            k   = 0;
        }
        if ((symIdx = lookup(sp)) == 0)
            symIdx = enter(k, len, LITER, true);
        pstack[mp].fixv = symIdx;
        return;
    case 125: // <CONSTANT> ::= <NUMBER>
        if ((symIdx = lookup(sp)) == 0) {
            /*     enter number into symbol table */
            tokId  = pstack[mp].fixv;
            symIdx = enter(tokId, tokId > 255 ? 2 : 1, NUMBER, true);
        }
        pstack[mp].fixv = symIdx;
        return;
    case 126: // <TO> ::= TO
        symIdx = pstack[mp - 3].fixv;
        if (symIdx <= 0) {
            error("33: interative DO index is invalid");
            pstack[mp].fixv = 1;
        } else {
            tokId               = symIdx;
            pstack[mp - 3].fixv = tokId;
            emit(tokId, ADR);
            emit(STD, OPR);
            i = enter(0, LocalLabel, LABEL, false);
            emit(i, DEF);
            pstack[mp].fixv = i;
            emit(tokId, VLU);
        }
        return;
    case 127: // <BY> ::= BY
        emit(LEQ, OPR);
        tokId = enter(0, LocalLabel, LABEL, false);
        /*     save symbol number at <to> (end loop number) */
        i                   = pstack[mp - 2].fixv;
        pstack[mp - 2].fixv = tokId;
        emit(tokId, VLU);
        emit(TRC, OPR);
        tokId           = enter(0, LocalLabel, LABEL, false);
        pstack[mp].fixv = (i << 14) + tokId;
        /*     <by> is (to number/statement number) */
        emit(tokId, VLU);
        emit(TRA, OPR);
        /*     now define by LABEL */
        tokId = enter(0, LocalLabel, LABEL, false);
        /*     save by LABEL in <to> as branch back number */
        pstack[mp - 2].fixv += tokId << 14;
        emit(tokId, DEF);
        return;
    case 128: // <WHILE> ::= WHILE
        tokId = enter(0, LocalLabel, LABEL, false);
        emit(tokId, DEF);
        pstack[mp].fixv = tokId;
        return;
    }
    error("Internal error in synth. Unknown prod=%d", prod);
}

void enterb() {
    /*     entry to block goes through here */
    curblk++;
    proctp[curblk] = proctp[curblk - 1];
    if (curblk >= MAXBLK)
        fatal("36: too many block levels");
    block[curblk] = symNext;
    doHead(curblk, DO_GROUP, 0);
    /*     save the state of the macro definition table */
    macblk[curblk] = macdefSP;

    blksym         = symNext;
    return;
}

_Noreturn void fatal(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(lstFp, "Fatal Error: ");
    vfprintf(lstFp, fmt, args);
    fprintf(lstFp, "\nCompilation Terminated\n");
    fprintf(stderr, "Fatal Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\nCompilation Terminated\n");
    va_end(args);
    exit(++errorCnt);
}

void error(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(lstFp, "(%05d) Error: ", ++errorCnt);
    vfprintf(lstFp, fmt, args);
    fprintf(lstFp, "  near %.*s\n", acclen, accum);
    if (errorCnt == 100)
        fatal("Too many errors");
    va_end(args);
}

void showTopTokens(int start, int col) {
    for (int tokId = start; tokId <= sp; tokId++) {
        char const *token = tokens[pstack[tokId].tokId];
        if (col + strlen(token) + (*token == '<' && token[1] ? 1 : 3) > C_WIDTH)
            col = fprintf(lstFp, "\n%*s", 8, "") - 1;
        if (*token == '<')
            col += fprintf(lstFp, " %s", token);
        else
            col += fprintf(lstFp, " '%s'", token);
    }
    putc('\n', lstFp);
}

void sdump() {
    /*     check for stack dump bypass */
    if (C_BYPASS == 0)
        showTopTokens(5, fprintf(lstFp, "PARSE STACK:"));
}

void redpr(const int prod, const int tokId) {
    if (prod > 128)
        error("Unknown production %d\n", prod);
    else
        showTopTokens(mp, fprintf(lstFp, "    %03d %s ::=", prod, tokens[tokId]));
}
