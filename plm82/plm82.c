/****************************************************************************
 *  plm82.c: part of the C port of Intel's cross compiler plm82             *
 *  The original application is Copyright Intel                             *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// end of change
/*************************************************************************/

/*         8 0 8 0   p l / m   c o m p i l e r ,   p a s s - 2          */
/*                                 plm82                                */
/*                              version 2.0/4.0 hybrid                  */
/*                             january/November, 1975                   */

/*                          copyright (c) 1975                          */
/*                          intel corporation                           */
/*                          3065 bowers avenue                          */
/*                          santa clara, california 95051               */

/*  modifyed by jeff ogden (um), december 1977.                         */

/*************************************************************************/

/*         p a s s - 2     e r r o r    m e s s a g e s*/

/*  error                     message*/
/*  number*/
/*  ------  --- -------------------------------------------------------*/

/*   101     reference to storage locations outside the virtual memory*/
/*           of pass-2.  RE-compile pass-2 with larger 'memory' array.*/

/*   102         "*/

/*   103     virtual memory overflow.  program is too large to compile*/
/*           with present size of 'memory.'  either shorten program OR*/
/*           recompile pass-2 with a larger virtual memory.*/

/*   104     (same as 103).*/

/*   105     $toggle used improperly IN pass-2.  attempt to complement*/
/*           a toggle which has a value other than 0 OR 1.*/

/*   106     register allocation table underflow.  may be due to a pre-*/

/*   107     register allocation error.  no registers available.  may*/
/*           be caused by a previous error, OR pass-2 compiler error.*/

/*   108     pass-2 symbol table overflow.  reduce number of*/
/*           symbols, OR RE-compile pass-2 with larger symbol table.*/

/*   109     symbol table overflow (see error 108).*/

/*   110     memory allocation error.  too much storage specified IN*/
/*           the source program (16k max).  reduce source program*/
/*           memory requirements.*/

/*   111     inline data format error.  may be due to improper*/
/*           record size IN symbol table file passed to pass-2.*/

/*   112     (same as error 107).*/

/*   113     register allocation stack overflow.  either simplify the*/
/*           program OR increase the size of the allocation stacks.*/

/*   114     pass-2 compiler error IN 'litadd' -- may be due to a*/
/*           previous error.*/

/*   115     (same as 114).*/

/*   116     (same as 114).*/

/*   117     line width set too narrow for code dump (use $width=n)*/

/*   118     (same as 107).*/

/*   119     (same as 110).*/

/*   120     (same as 110, but may be a pass-2 compiler error).*/

/*   121     (same as 108).*/

/*   122     program requires too much program AND variable storage.*/
/*           (program AND variables exceed 16k).*/

/*   123     initialized storage overlaps previously initialized storage.*/

/*   124     initialization table format error.  (see error 111).*/

/*   125     inline data error.  may have been caused by previous error.*/
//           This can also be caused by insufficient symbol space when
//           an inline constant is being defined (See card 44130)

/*   126     built-IN function improperly called.*/

/*   127     invalid intermediate language format. (see error 111).*/

/*   128     (same as error 113).*/

/*   129     invalid use of built-IN function IN an assignment.*/

/*   130     pass-2 compiler error.  invalid variable precision (NOT */
/*           single byte OR double byte).  may be due to previous error.*/

/*   131     LABEL resolution error IN pass-2 (may be compiler error).*/

/*   132     (same as 108).*/

/*   133     (same as 113).*/

/*   134     invalid program transfer (only computed jumps are allowed */
/*           with a 'go to').*/

/*   135     (same as 134).*/

/*   136     error IN built-IN function call. */

/*   137     (NOT used)*/

/*   138     (same as 107). */

/*   139     error IN changing variable to address reference. may*/
/*           be a pass-2 compiler error, OR may be caused by pre- */
/*           vous error.*/

/*   140     (same as 107).*/

/*   141     invalid origin.  code has already been generated IN the*/
/*           specified locations.*/

/*   142     a symbol table dump has been specified (using the $memory */
/*           toggle IN pass-1), but no file has been specified to RE-*/
/*           ceive the bnpf tape (use the $bnpf=n control).*/

/*   143     invalid format for the simulator symbol table dump (see*/
/*           error 111). */

/*   144     stack NOT empty at END of compilation.  possibly caused */
/*           by previous compilation error.*/

/*   145     procedures nested too deeply (hl optimization) */
/*           simplify nesting, OR RE-compile with larger pstack */

/*   146     procedure optimization stack underflow.  may be a */
/*           return IN outer block. */

/*   147     stack NOT empty at END of compilation. register */
/*           stack order is invalid.  may be due to previous error. */

/*   148     pass-2 compiler error.  attempt to unstack too*/
/*           many values.  may be due to previous error.*/

/*   149     pass-2 compiler error. attempt to convert invalid */
/*           value to address type.  may be due to previous error.*/

/*   150     (same as 147) */

/*   151     pass-2 compiler error. unbalanced  execution stack*/
/*           at block END.  may be due to a previous error.*/

/*    152    invalid stack order IN apply.  may be due to previous*/
/*           error.*/

//    153    Illegal Control Record: First nonblank character was
//           other than a dollar sign

//    154    Bad code origin from pass-1 (e.g. Pass-1 error 48)

/*              i m p l e m e n t a t i o n    n o t e s */
/*              - - - - - - - - - - - - - -    - - - - -*/

#include "plm82.h"

char const help[] =
    "Usage: %s [-a n] [-d nn] [-e nn] [-f] [-g n] [-i nn] [-l nn] [-m] [-n] [-O] [-s n] [-v nn] "
    "[-x]"
    "plmfile\n"
    "Where\n"
    "-a n       debug - set analysis level != 0 show state, >= 2 show registers\n"
    "-e nn      set the entry point - default first org or post interrupt vectors. -e 0 simulates "
    "V2.0 code\n"
    "-f         disable code dump at finish\n"
    "-g n       trace - 0 no trace, 1 lines vs locs, 2 full interlist- default 1\n"
    "-i [+]nn   stack initialisation. No '+' 0 system determined, 1 user specified, > 1 stack "
    "location\n"
    "           with '+' as per system determined but reserve an additional nn words\n"
    "-l nn      start machine code generation at location nn\n"
    "-m         turn off symbol map\n"
    "-n         write emitter trace\n"
    "-O         enable the additional V4 load and arith immediate optimisations\n"
    "-s n       debug - write symbol info. 0 none, != 0 address , >= 2 detailed info\n"
    "-t nn      set the scaler for time function 1-256, default 12 for 2MHz 8080\n"
    "-v nn      set first page of RAM\n"
    "-w nn      set output width, min 72 default 120\n"
    "-x         disable hex file\n"
    "plmfile    is the same source file name used in plm81 of the form prefix.ext\n"
    "           intermediate files prefix.lst, prefix.pol and prefix.sym are used\n"
    "           optionally prefix.cfg can be used to hold plm82 configuration flags\n"
    "           prefix.lst is updated with pass2 output added and prefix.hex is created\n"
    "           Note the .pol and .sym files are deleted if pass 2 is successful\n"
    "Numerical arguments terminated with h or H are processed as hexadecimal numbers\n";

/* types */
int casjmp = 0;

//                             A  B  C  D  E  H  L     M
uint8_t const regmap[9 + 1] = { 0, 7, 0, 1, 2, 3, 4, 5, 6, 6 };

/* ... plm2 version ...*/
#define VERSION "4.0"

/* symbl */
int symbol[MAXSYM + 1];
int symax  = MAXSYM;
int sytop  = 0;
int syinfo = MAXSYM;
int lmem;
int extraStack = 0; // additional stack space requested by user

/* xfropt is used in branch optimiztion*/
int xfrloc = -1;
int xfrsym = 0;
int tstloc = -1;
int conloc = -1;
int defsym = 0;
int defrh  = -1;
int defrl  = -1;

/* inter */

uint16_t intpro[8];

// clang-format on

/* code */
int codloc = 0;
bool alter;

// clang-format on
bool errflg = false;

/* peep is used in peephole optimization (see emit)*/
/* lapol is a one element polish look-ahead*/
/* lastld is codloc of last register to memory store*/
/* lastrg is the effected register*/
/* lastin is the codloc of the last increment*/
/* (used in do-loop index increment)*/
/* lastex is location of last XCHG operator*/
/* lastir is the codloc of the last register increment*/
/* (used in apply and gensto to gen inr memory)*/
typedef struct {
    int8_t typ;
    uint16_t val;
} pol_t;

int lapol       = -1;
int lastLoad    = 0;
int lastReg     = 0;
int lastin      = 0;
int lastex      = 0;
int lastIncReg  = 0;
int lastLoadImm = 0; // location of last load reg immediate
int lastRegImm  = 0;

/* pstack is the procedure stack used in hl optimization*/
int prstk[15 + 1];
int maxdep[16];
int curdep[16];
int prsmax     = 15;
int prsp       = 0;
int lxis       = 0;

int timeScaler = 12;

// clang-format on
/* memory */
uint8_t mem[MAXMEM]; // upto max memory of 8080
int codeBase = 0;
int entry    = -1; // entry point for hex trailer
int preamb;

// for all registers
// regValue in range 0-255 is the current value, -1 is unknown
// for RH and RL the additional options are
// RH     RL
// -255   -255 nil
// -254   -254 not changed
// -3     -symId
// -4     -symId

struct {
    uint8_t regSt;
    int16_t regValue;
    bool lock;
} state[1 + RL] = { { 0, -1, false }, { 0, -1, false }, { 0, -1, false }, { 0, -1, false },
                    { 0, -1, false }, { 0, -1, false }, { 0, -1, false }, { 0, -1, false } };

regAlloc_t parseStk[16 + 1];

int8_t sp = 0;
enum { maxsp = 16 };

/* intbas is the largest intrinsic symbol number including guard symbol */
int intbas = S_DEC + 1;

int errorCnt;
int C_ANALYSIS;
int C_COUNT;           // line counter
bool C_FINISH  = true; // dump code at finish
int C_GENERATE = 1;
int C_LOAD     = -1; // load address of program
bool C_MAP     = true;
bool C_NUMERIC = false;
bool v4Opt     = false;
int C_SYMBOLS;
int C_VARIABLES;
int C_WIDTH    = 120;
bool C_HEXFILE = true;
int C_STACKHANDLING; // 0->compiler handles stack pointer
                     // 1 - programmer handles stack pointer
                     // > 1 n is stack size

// helper function - most likely inlined
regAlloc_t stkItem(uint16_t assignment, uint8_t prec, int16_t st, int32_t litv) {
    regAlloc_t r = { .assignment = assignment, .prec = prec, .st = st, .litv = litv };
    return r;
}

int parseNum(char const *s) {
    char const *t;

    for (t = s; isxdigit(*t); t++)
        ;

    uint8_t radix = toupper(*t) == 'H' ? 16 : 10;
    if (t == s || t[radix == 16])
        usage("Invalid number %s\n", s);

    int32_t val = 0;

    for (t = s; isxdigit(*t); t++) {
        uint8_t digit = isdigit(*t) ? *t - '0' : toupper(*t) - 'A' + 10;
        if (digit >= radix)
            usage("Invalid digit %c in decimal number %s\n", *t, s);
        val = val * radix + digit;
        if (val >= 0x10000)
            usage("Number %s is too big\n", s);
    }
    return val;
}

int main(int argc, char **argv) {

    while (getOpt(argc, argv, "a:e:fg:i:l:mn:Os:t:v:w:x") != EOF) {
        switch (optOpt) {
        case 'a':
            C_ANALYSIS = parseNum(optArg);
            break;
        case 'e':
            if ((entry = parseNum(optArg)) > 0xffff)
                usage("Entry point %s out of range", optArg);
            break;
        case 'w':
            if ((C_WIDTH = parseNum(optArg)) < 72)
                C_WIDTH = 72;
            else if (C_WIDTH > 132)
                C_WIDTH = 132;
            break;
        case 'f':
            C_FINISH = !C_FINISH;
            break;
        case 'g':
            C_GENERATE = parseNum(optArg);
            break;
        case 'i':
            if (optArg[0] == '+') {
                if ((extraStack = parseNum(optArg + 1)) >= 0x8000)
                    usage("Extra stack size %s too large", optArg + 1);
            } else if ((C_STACKHANDLING = parseNum(optArg)) >= 0x10000)
                usage("Stack address %s out of range", optArg);
            break;
        case 'l':
            if ((C_LOAD = parseNum(optArg)) > 0xffff)
                usage("Load address %s out of range", optArg);
            break;
        case 'm':
            C_MAP = !C_MAP;
            break;
        case 'n':
            C_NUMERIC = !C_NUMERIC;
            break;
        case 'O':
            v4Opt = true;
            break;
        case 's':
            C_SYMBOLS = parseNum(optArg);
            break;
        case 't':
            timeScaler = parseNum(optArg);
            if (timeScaler < 1 || timeScaler > 256)
                usage("Time scaler %s out of range", optArg);
            timeScaler %= 256; // ensure it is in range 0-255
            break;
        case 'V':
            if ((C_VARIABLES = parseNum(optArg)) >= 0x100)
                usage("Variables page address %s out of range", optArg);
            break;
        case 'x':
            C_HEXFILE = !C_HEXFILE;
            break;
        }
    }
    if (optInd != argc - 1)
        usage("Expected single file name");

    openfiles(argv[optInd]);

    time_t now;
    time(&now);
    fprintf(lstFp, "         pl/m-8080 pass2 Version " VERSION " - %s\n", ctime(&now));

    codloc = C_LOAD >= 0 ? C_LOAD : 0;
    loadsy();
    readcd();
    if (!errflg) {
        /* make sure compiler stack is empty */
        if (sp != 0)
            error("144: stack not empty at end of compilation");

        /* make sure execution stack is empty */
        if (curdep[0] != 0)
            error("150: stack not empty at end of compilation. Register stack order is invalid");
        reloc();

        /* may want a symbol table for the simulator */
        sydump();
        if (C_FINISH) {
            /* dump the preamble */
            for (int ii = 0; ii < 64; ii += 8)
                if (intpro[ii / 8])
                    dump(ii, ii + 2, true);

            /* dump the symbol table by segments until codloc-1 */
            int loc = codeBase;

            do {
                int dataAddr = 0x10000;
                int dataLen  = 0;

                /* locate next inline data at or above loc */
                int symNum = 0;

                for (int n = intbas; n <= sytop; n++) {
                    int attrib = symAttrib(n);
                    if (attrib >= 0 && INFO_TYPE(attrib) == VARB) {
                        int j = LOWWORD(abs(symAddr(n)));
                        if (j <= dataAddr && j >= loc &&
                            INFO_PREC(attrib)) { // check candidate at j
                            dataAddr = j;
                            symNum   = n;
                            dataLen  = INFO_ECNT(attrib) * (INFO_PREC(attrib) == 2 ? 2 : 1);
                        }
                    }
                }

                /* dataAddr is base address of next data stmt, dataLen is length IN bytes */
                if (loc < dataAddr) /* code is printed below */
                    dump(loc, min(dataAddr - 1, codloc - 1), true);

                if (dataAddr < codloc) { /* then the data segments */
                    if (C_SYMBOLS)
                        fprintf(lstFp, "S%05d\n", symNum);
                    dump(dataAddr, dataAddr + dataLen - 1, false);
                }
                loc = dataAddr + dataLen;
            } while (loc < codloc);
        }

        int isave = codloc;
        int kval  = loadin();

        if (codloc != isave && C_FINISH)
            dump(kval, codloc - 1, false); /* dump the initialized variables */

        if (C_HEXFILE) {
            /* punch deck */

            for (int ii = 0; ii < 64; ii += 8)
                if (intpro[ii / 8])
                    puncod(ii, ii + 2);

            if (codloc != isave) {
                puncod(codeBase, isave - 1);
                puncod(kval, codloc - 1);
            } else
                puncod(codeBase, codloc - 1);
            puntrailer();
        }
        /* write error count */
        if (errorCnt == 0) {
            fputs("\nNO PROGRAM ERRORS\n\n", lstFp);
            fputs("\nNO PROGRAM ERRORS\n\n", stdout);
        } else {
            fprintf(lstFp, "%d PROGRAM ERROR%s\n\n", errorCnt, errorCnt != 1 ? "S" : "");
            fprintf(stdout, "%d PROGRAM ERROR%s\n\n", errorCnt, errorCnt != 1 ? "S" : "");
        }
    }
    closefiles();
    cmpuse();
    return errorCnt;
}

uint8_t get(uint32_t ip) {
    if (ip >= MAXMEM) {
        Fatal("101: pass-2 address outside address space");
        return 0;
    }
    return mem[ip];
}

uint16_t getword(int ip) {
    return get(ip) + get(ip + 1) * 256;
}

void put(uint32_t ip, uint8_t val) {
    if (ip >= MAXMEM)
        Fatal("102: pass-2 address outside address space");
    else
        mem[ip] = val;
}

void putword(uint32_t ip, uint16_t val) {
    put(ip, val % 256);
    put(ip + 1, val / 256);
}

int right(int reg, int j) {
    return reg % (1 << j);
}

void pop(int n) { /* pop the top n elements from the stack */

    while (n-- > 0) {
        if (sp <= 0) {
            error("106: register allocation table underflow");
            return;
        }
        uint8_t reg;
        if ((reg = REGHIGH(parseStk[sp].assignment))) {
            if (state[RA].regSt == reg)
                state[RA].regSt = 0;
            state[reg].lock  = false;
            state[reg].regSt = 0;
        }
        if ((reg = REGLOW(parseStk[sp].assignment))) {
            if (state[RA].regSt == reg)
                state[RA].regSt = 0;
            state[reg].lock  = false;
            state[reg].regSt = 0;
        }

        sp--;
    }
}

void apply(int op, int op2, bool com, bool cyflag) {
    int ia, ib, lp, jp, k;

    /* apply op to top elements of stack */
    /* use op2 for high order bytes if double byte operation */
    /* com = true if commutative operator, false otherwise */
    /* cyflag = true if the CARRY is involved in the operation */

    /* may want to clear the CARRY for this operation */

    /* check for one of the operands in the stack (only one can be there) */
    for (uint8_t findCnt = 0, j = sp - 1; j <= sp; j++) {
        if (parseStk[j].st == 0 && parseStk[j].assignment == 0 &&
            parseStk[j].litv < 0) { /* operand is stacked */
            uint16_t regPair             = genReg(-2);
            state[REGLOW(regPair)].regSt = j;
            if (findCnt++) //
                error("152: Invalid stack order in 'apply'");
            if (parseStk[j].prec > 1) { /* double byte operand */
                state[REGHIGH(regPair)].regSt = j;
                parseStk[j].assignment        = regPair;
            } else /* single precision result */
                parseStk[j].assignment = REGLOW(regPair);
            emit(POP, REGHIGH(regPair), 0);
            ustack();
        }
    }
    /* make a quick check for possible accumulator match */
    /* with the second operand */
    if (COND((ia = parseStk[sp].assignment)))
        cvcond(sp);
    if (COND((ib = parseStk[sp - 1].assignment)))
        cvcond(sp - 1);
    int regA = state[RA].regSt;
    if (ia && ib && regA && com &&
        regA == REGLOW(ia)) /* commutative operator, one may be in the accumulator */
        exch();             /* second operand in gpr's, regA.o. byte in accumulator */

    bool storeSetup = false; // replaces use of goto

    for (;; exch()) {
        if (!parseStk[sp - 1].assignment) { /* is op1 in gpr's */
            if (parseStk[sp].assignment) {  /* is op2 in gpr's */
                                            /* yes - can we exchange and try again */
                parseStk[sp].litv = -1;     /* after insuring that a literal has no regs assigned */
                if (com)
                    continue;
            }
        } else if ((ia = REGLOW(parseStk[sp - 1].assignment)) == 0) {
            Fatal("107: register allocation error. No registers available");
            return;
        } else { /* reg assigned, lock regs containing var */
            state[ia].lock = true;
            if ((ib = REGHIGH(parseStk[sp - 1].assignment)))
                state[ib].lock = true;

            /* may have to generate one free reg */
            if (parseStk[sp - 1].prec < parseStk[sp].prec)
                ib = ia - 1;

            /* check for pending register store */
            if ((jp = state[RA].regSt) != ia) {
                if (jp)
                    emit(LD, jp, RA);
                state[RA].regSt = ia;
                emit(LD, RA, ia);
            }
        }

        /* op2 NOT IN gpr's OR op is NOT commutative */
        /* check for literal value - is op2 literal */
        if ((k = parseStk[sp].litv) >= 0) {
            if (parseStk[sp].prec <= 1 && parseStk[sp - 1].prec <= 1 &&
                k == 1 /* make special check for possible increment OR decrement */
                && (op == AD || op == SU)        /* must be ADD OR subtract without CARRY */
                && parseStk[sp - 1].prec == 1) { /* first operand must be single byte variable */
                if (ia <= 1) {                   /* op1 must be IN memory, so load into gpr */
                    loadv(sp - 1, 0);
                    if ((ia = parseStk[sp - 1].assignment & 0xf) == 0) {
                        Fatal("107: register allocation error. No registers available");
                        return;
                    }
                    lastIncReg = codloc; /* ...may change to inr memory if STD to op1 follows... */
                }
                emit(op == AD ? IN : DC, state[RA].regSt == ia ? RA : ia, 0);
                storeSetup = true;
            }
            break;
        }

        /* op1 not a literal,  check for literal op2 */
        if (parseStk[sp - 1].litv < 0 || !com)
            break;
    }

    if (!storeSetup) {
        /* generate registers to hold results in loadv */
        /* (loadv will load the low order byte into the ACC) */
        loadv(sp - 1, 1);
        if ((ia = REGLOW(parseStk[sp - 1].assignment)) == 0) {
            Fatal("107: register allocation error. No registers available");
            return;
        }
        state[ia].lock = true;
        ib             = parseStk[sp - 1].assignment / 16;

        /* is this a single byte / double byte operation */
        if (ib <= 0 && parseStk[sp].prec != 1) {
            /* get a spare register */
            if ((ib = ia - 1) == 0) { // ia was A reg so can't double byte
                Fatal("107: register allocation error. No registers available");
                return;
            }
            state[ib].lock = true;
        } else
            ib &= 7;

        /* now ready to perform operation */
        /* l.o. byte is in AC, h.o. byte is in ib. */
        /* result goes to ia (l.o.) AND ib (h.o.) */

        /* is op2 IN gpr's */
        k = -1;
        if ((lp = parseStk[sp].assignment) > 0) /* perform ACC-reg operation */
            emit(op, REGLOW(lp), 0);
        else if ((k = parseStk[sp].litv) < 0) { /* is op2 a literal */
            loadv(sp, 2);                       /* perform operation with low order byte */
            emit(op, ME, 0);
        } else if (op == XR && k % 256 == 255) /* use CMA if op is XR AND op2 is LIT 255 */
            emit(CMA, 0, 0);
        else /* perform ACC-immediate operation */
            emit(op, -k % 256, 0);

        /* set up a pending register store */
        /* if this is NOT a compare */
        if (op != CP)
            state[RA].regSt = ia;
        if (parseStk[sp].prec == 2) {
            if (k < 0 && lp <= 0) { /* is h.o. byte of op2 IN memory */
                emit(IN, RL, 0);    /* point to h.o. byte with h AND l */
                state[RL].regValue++;
            }

            /* do we need to pad with h.o. zero for op1 */
            if ((jp = state[RA].regSt)) {
                if (jp != ib) { // is store pending
                    emit(LD, jp, RA);
                    state[RA].regSt = 0;
                    if (parseStk[sp - 1].prec > 1)
                        emit(LD, RA, ib);
                    else
                        emit(cyflag ? LD : XR, RA, 0);
                }
            } else if (parseStk[sp - 1].prec > 1)
                emit(LD, RA, ib);
            else
                emit(cyflag ? LD : XR, RA, 0);

            if (lp) /* op2 in gpr's - perform ACC-register operation */
                emit(op2, REGHIGH(lp), 0);
            else if (k < 0) /* perform ACC-memory operation */
                emit(op2, ME, 0);
            else if (op2 != XR || k != 0xffff) /* yes - perform ACC-immediate operation */
                emit(op2, -HIGH(k), 0);
            else /* use CMA if op1 is XR AND op2 is 0xffff */
                emit(CMA, 0, 0);
        } else if (parseStk[sp - 1].prec >= 2) { /* second operand is single byte */
            /* may not need to perform operations for certain operators, but ... */
            /* perform operation with h.o. byte of op1 */
            /* op1 must be in the gpr's - perform dummy operation with ZERO */
            if ((jp = state[RA].regSt)) {
                if (jp != ib) {
                    emit(LD, jp, RA);
                    state[RA].regSt = 0;
                    emit(LD, RA, ib);
                }
            } else
                emit(LD, RA, ib);
            emit(op2, 0, 0);
        } else
            storeSetup = true;
    }
    /* set up pending register store */
    if (!storeSetup)
        state[RA].regSt = ib;

    /* save the pending accumulator - register store */
    jp = state[RA].regSt;
    pop(2);
    state[RA].regSt    = jp;
    parseStk[++sp]     = stkItem(REGPAIR(ib, ia), 1, 0, -1);
    state[ia].regSt    = sp;
    state[ia].regValue = -1;
    if (ib > 0) {
        parseStk[sp].prec  = 2;
        state[ib].regSt    = sp;
        state[ib].lock     = false;
        state[ib].regValue = -1;
    }
}

uint16_t genReg(int8_t np) {
    int ia, ib;
    genreg(np, &ia, &ib);
    return REGPAIR(ib, ia);
}

void genreg(int np, int *ia, int *ib) {
    uint8_t freeReg;

    /* generate abs(np) free registers for subsequent operation */
    // Note -2 <= np <= 2 np negative if no pushing allowed

    if (ib)
        *ib = 0;
    *ia = 0;

    /* look for free RC or RE AND allocate in pairs (RC/RB,RE/RD) */
    while (state[freeReg = RC].regSt && state[freeReg = RE].regSt) {
        uint8_t candidate = 0;
        if (np >= 0 && sp > 0) {
            /* generate temporaries in the stack and re-try */
            /* search for lowest register pair assignment in stack */
            for (int reg = 1; reg <= sp; reg++) {
                uint16_t assign = parseStk[reg].assignment;
                if (assign == 0) { // not assigned
                    // reset search if already stacked
                    if (parseStk[reg].st == 0 && parseStk[reg].litv < 0)
                        candidate = 0;
                } else if (!COND(assign) && candidate == 0) {
                    // don't look at cond or if already have candidate
                    int lowReg  = REGLOW(assign);
                    int highReg = REGHIGH(assign);
                    // is single reg usage or pair and both not locked
                    if (!state[lowReg].lock &&
                        (highReg == 0 || (highReg == lowReg - 1 && !state[highReg].lock)))
                        candidate = reg; // have candidate
                }
            }
        }
        if (!candidate) { // didn't find a candidate
            saver();
            return;
        }
        /* found entry to push at ip */
        int lowReg          = REGLOW(parseStk[candidate].assignment);
        int highReg         = REGHIGH(parseStk[candidate].assignment);

        state[lowReg].regSt = 0; // mark as freed
        if (highReg > 0)         // part of a pair
            state[highReg].regSt = 0;
        int raRegs = state[RA].regSt;
        /* check pending register store */
        if (raRegs && (raRegs == lowReg || raRegs == highReg)) {
            emit(LD, raRegs == lowReg ? lowReg : highReg, RA);
            state[RA].regSt = 0;
        }

        /* free the register for allocation */
        stack(1);
        emit(PUSH, lowReg - 1, 0);

        /* mark element as stacked (st=0, assignment=0) */
        parseStk[candidate].st         = 0;
        parseStk[candidate].assignment = 0;
        parseStk[candidate].litv       = -1;
    }
    *ia = freeReg;
    if (abs(np) > 1 && ib)
        *ib = freeReg - 1;
    return;
}

void loadv(int s, int typ) {
    uint16_t regPair = 0;

    /* load value to register if not a literal */
    /* typ = 0, load directly into registers */
    /* typ = 1 if call from 'apply' in which case the l.o. byte is */
    /* loaded into the accumulator instead of a gpr. */
    /* if typ = 2, the address is loaded, but the variable is not. */
    /* if typ = 3, a double byte (address) fetch is forced. */
    /* if typ = 4 then do a quick load into h and l */
    /* if typ = 5, a double byte quick load into h and l is forced */
    uint8_t prec = 0;
    if (typ != 2) {
        if (COND(parseStk[s].assignment)) // convert conditional register to normal
            cvcond(s);
        if (typ == 4 || typ == 5) {
            int litv = parseStk[s].litv;
            int st   = parseStk[s].st;
            if (parseStk[s].assignment) { /* registers are assigned */
                int regsA       = state[RA].regSt;
                uint8_t regLow  = REGLOW(parseStk[s].assignment);
                uint8_t regHigh = REGHIGH(parseStk[s].assignment);
                if (regsA && regsA == regHigh)
                    regHigh = RA;
                if (regsA && regsA == regLow)
                    regLow = RA;
                if (regLow == RE && regHigh == RD)
                    emit(XCHG, 0, 0);
                else {
                    emit(LD, RL, regLow);  /* not in d AND e, so use two byte move */
                    emit(LD, RH, regHigh); /* may be a lhi 0 */
                }
            } else {
                if (st) {
                    /* variable , literal  OR address reference */
                    if (st <= 0)
                        litadd(sp); /* ADR ref - set h AND l with litadd */
                    else if (litv < 0) {
                        /* simple variable or literal ref, may use LHLD */
                        /* may want to check for possible INX OR DCX, but now... */
                        int regH = state[RH].regValue;
                        int regL = state[RL].regValue;
                        if (regH != -3 || -regL != st) {
                            if (regH == -4 && -regL == st)
                                emit(DCX, RH, 0);
                            else
                                emit(LHLD, chain(st, codloc + 1), 0);
                        }
                        state[RH].regValue = -1;
                        state[RL].regValue = -1;
                        if (parseStk[s].prec <= 1 && typ != 5)
                            /* this is a single byte value */
                            emit(LD, RH, 0);
                        else {
                            state[RH].regValue = -3;
                            state[RL].regValue = -st;
                        }
                    } else {
                        /* literal value to h l */
                        emit(LXI, RH, litv);
                        state[RH].regValue = HIGH(litv);
                        state[RL].regValue = LOW(litv);
                        return;
                    }
                } else if (litv < 0) {
                    /* value stacked, so... */
                    ustack();
                    emit(POP, RH, 0);
                    if (parseStk[s].prec < 2)
                        emit(LD, RH, 0);
                    state[RH].regValue = -1;
                    state[RL].regValue = -1;
                } else {
                    /* literal value to h l */
                    emit(LXI, RH, litv);
                    state[RH].regValue = litv / 256;
                    state[RL].regValue = litv % 256;
                    return;
                }
            }
            if (parseStk[s].assignment == 0)
                parseStk[s].assignment = REGPAIR(RH, RL);
            return;
        } else if (parseStk[s].assignment > 0)
            return;
        else { /* check for previously stacked value */
            if (parseStk[s].st != 0 || parseStk[s].litv >= 0) {
                /* no registers assigned.  allocate registers AND load value. */
                prec = parseStk[s].prec;
                if (typ == 3) { /* force a double byte load */
                    prec = 2;
                    typ  = 0;
                }

                if ((regPair = genReg(prec)) == 0) {
                    Fatal("112: register allocation error");
                    return;
                }
            } else {
                regPair = genReg(2);
                /* check to ensure the stack is in good shape */
                for (int reg = s + 1; reg <= sp; reg++) {
                    if (parseStk[reg].st == 0 && parseStk[reg].assignment == 0 &&
                        parseStk[reg].litv < 0)
                        /* found another stacked value */
                        error("147: stack not empty at end of compilation. Register stack "
                              "order is invalid");
                }
                /* available cpu register is based at ia */
                emit(POP, REGHIGH(regPair), 0);
                state[REGLOW(regPair)].regSt = s;
                if (parseStk[sp].prec >= 2) {
                    state[REGHIGH(regPair)].regSt = s;
                    parseStk[s].assignment        = regPair;
                } else
                    parseStk[s].assignment = REGLOW(regPair);
                if (typ == 1) {
                    if (state[RA].regSt)
                        emit(LD, state[RA].regSt, RA);
                    emit(LD, RA, REGLOW(regPair));
                }

                ustack(); /* decrement the stack count for this level */
                return;
            }
        }
    }

    /* check for literal value (in arith exp) */
    // guard typ == 2 only ever has -ve lit, so doesn't need ia or ib
    int litv = parseStk[s].litv;
    if (typ != 2 && 0 <= litv && litv <= 0xffff) {
        parseStk[s].litv                = -1;
        state[REGLOW(regPair)].regSt    = s;
        state[REGLOW(regPair)].regValue = LOW(litv);
        if (typ == 1) {            /* check for pending register store */
            if (state[RA].regSt) { /* store ACC into register before continuing */
                emit(LD, state[RA].regSt, RA);
                state[RA].regSt = 0;
            }
            if (LOW(litv) == 0)
                emit(XR, RA, 0);
            else
                emit(LD, RA, -LOW(litv));
            if (REGHIGH(regPair))
                emit(LD, REGHIGH(regPair), -HIGH(litv));
        } else {                  /* typ = 0, load directly into registers */
            if (REGHIGH(regPair)) // reg pair so use LXI
                emit(LXI, REGHIGH(regPair), litv);
            else
                emit(LD, REGLOW(regPair), -LOW(litv));
        }
        if (REGHIGH(regPair)) {
            state[REGHIGH(regPair)].regSt    = s;
            state[REGHIGH(regPair)].regValue = -litv;
        }
        parseStk[s].assignment = regPair;
    } else { /* otherwise fetch from memory */
        sp++;
        setadr(parseStk[s].st);
        litadd(sp);

        /* address of varREGLOW(regPair)ble is in h and l */
        if (typ == 0) /* call from gensto (typ = 0) */
            emit(LD, REGLOW(regPair), ME);
        else if (typ == 1) { /* call from apply to load value of varREGLOW(regPair)ble */
            /* check for pending register store */
            if (state[RA].regSt) { /* have to store ACC into register before reloading */
                emit(LD, state[RA].regSt, RA);
                state[RA].regSt = 0;
            }
            emit(LD, RA, ME);
        }

        /* check for double byte varREGLOW(regPair)ble */
        if (typ != 2 && prec > 1) { /* load high order byte */
            emit(IN, RL, 0);
            state[RL].regValue++;
            emit(LD, REGHIGH(regPair), ME);
        }

        /* value is now loaded */
        pop(1);
        if (typ != 2) {
            parseStk[s].assignment          = REGPAIR(REGHIGH(regPair), REGLOW(regPair));
            state[REGLOW(regPair)].regSt    = s;
            state[REGLOW(regPair)].regValue = -1;
            if (REGHIGH(regPair)) {
                state[REGHIGH(regPair)].regSt    = s;
                state[REGHIGH(regPair)].regValue = -1;
            }
        }
    }
}

void setadr(int val) { // set top of stack to address reference
    alter = true;
    if (sp > maxsp) {
        Fatal("113: register allocation stack overflow.");
        sp = 1;
    } else { // mark as address reference
        int attrib   = symAttrib(val);
        int addr     = symAddr(val);
        parseStk[sp] = stkItem(0, INFO_PREC(attrib), -val,
                               addr < 0 ? (-addr & 0xffff) : 0x10000 + (addr & 0xffff));
    }
}

void ustack() {
    /* decrement curdep and check for underflow */
    if (--curdep[prsp] < 0) {
        curdep[prsp] = 0;
        error("148: pass-2 compiler error. Attempt to unstack too many values");
    }
}

int chain(uint16_t sy, int loc) {
    /* chain in double-byte refs to symbol sy, if necessary */
    if (symAddr(sy) < 0)
        return -symAddr(sy) & 0xffff; // absolute address already assigned

    int next   = symRef(sy);
    symRef(sy) = loc;
    return next;
}

void genStore(bool keep) {
    uint16_t reg;
    int j;
    uint16_t iq = 0; // init to avoid compiler warning

    /* keep = false if STD, keep = true if STO (value retained) */
    /* generate a store into the address at stack top */
    /* load value if not literal */
    int l = parseStk[sp - 1].litv;
    if (l < 0)
        loadv(sp - 1, 0);

    uint8_t regLow  = REGLOW(parseStk[sp - 1].assignment);
    uint8_t regHigh = REGHIGH(parseStk[sp - 1].assignment);

    /* check for pending register store */
    int16_t regA = state[RA].regSt;
    if (regA) {
        if (regA == regHigh)
            regHigh = RA;
        if (regA == regLow)
            regLow = RA;
    }

    if (-parseStk[sp].st != S_STACKPTR) {
        bool chkInr = true;
        if (parseStk[sp].litv < 0) {
            bool releaseHL = true; // used to eliminate goto L220
            reg            = parseStk[sp].assignment;
            if (reg <= 0 && parseStk[sp].st == 0) { /* registers NOT allocated  */
                emit(POP, RH, 0);                   // address is stacked so POP to h AND l
                ustack();
            } else if ((reg = parseStk[sp].st) >= intbas) {
                /* may be able to simplify (OR eliminate) the LHLD */
                int regH = state[RH].regValue;
                int regL = state[RL].regValue;
                if (regH != -3 || -regL != reg) {
                    if (regH == -4 && -regL == reg) {
                        emit(DCX, RH, 0);
                        state[RH].regValue = -3;
                    } else {
                        j = chain(reg, codloc + 1);
                        emit(LHLD, j, 0);
                        state[RH].regValue = -3;
                        state[RL].regValue = -reg;
                    }
                }
                releaseHL = false;
            } else { /* check for ref to simple based variable */
                if (regLow)
                    state[regLow].lock = true;
                if (regHigh)
                    state[regHigh].lock = true;

                loadv(sp, 3); /* force a double byte fetch into gprs */
                reg  = parseStk[sp].assignment;

                regA = state[RA].regSt;
                j    = REGLOW(reg);
                reg /= 16; // PMO - might be able to use REGHIGH
                if (regLow == 0 || reg != j - 1) {
                    if (j == regA)
                        j = RA;
                    if (reg == regA)
                        reg = RA;
                    if (reg != RD || j != RE) {
                        emit(LD, RL, j);
                        emit(LD, RH, reg);
                    } else
                        emit(XCHG, 0, 0);
                } else if (reg != RD || lastex != codloc - 1) { /* if prevous syllable is XCHG then
                                                                   do another - peep will fix it */
                    if (regLow != RA) {                         /* use STAX - set up accumulator */
                        if (regA != 0)
                            emit(LD, regA, RA);
                        if (regHigh == RA)
                            regHigh = (uint8_t)regA;
                        emit(LD, RA, regLow);
                        state[RA].regSt = 0;
                    }
                    emit(STAX, reg, 0);
                    if (parseStk[sp].prec >= 2) { /* if byte dest we are done */
                        emit(INCX, reg, 0);
                        if (regHigh != 0) { /* store high order byte */
                            if (regLow == 1 && keep) {
                                emit(LD, REGLOW(parseStk[sp - 1].assignment), RA);
                                state[RA].regSt = 0;
                            }
                            emit(LD, RA, regHigh);
                            emit(STAX, reg, 0);
                        } else {                     /* store high order ZERO */
                            if (regLow == 1 && keep) // V4 fix
                                emit(LD, REGLOW(parseStk[sp - 1].assignment), RA);
                            state[RA].regSt = 0;
                            emit(XR, RA, 0);
                            emit(STAX, reg, 0);
                        }
                    }
                    pop(1);
                    return;
                }
            }

            if (releaseHL) {
                if (regHigh)
                    state[regHigh].lock = false;
                if (regLow)
                    state[regLow].lock = false;
                state[RH].regValue = state[RL].regValue = -1;
            }
        } else if (regHigh != RD || regLow != RE || lastex != codloc - 1 ||
                   parseStk[sp].prec != 2) /* otherwise this is a literal address */
            litadd(sp);                    /* if possible, generate a SHLD */
        else {
            emit(XCHG, 0, 0);
            int16_t symId = abs(parseStk[sp].st);

            emit(SHLD, chain(symId, codloc + 1), 0);
            state[RH].regValue = -3;
            state[RL].regValue = -symId;
            if (keep)
                emit(XCHG, 0, 0);
            chkInr = false;
        }

        if (chkInr) {

            /* we may change mov r,m inr r mov m,r to inr m. */
            /* if so, AND this is a non-destructive store, the register */
            /* assignment must be released. */
            iq = lastIncReg;

            /* generate low order byte store */
            emit(LD, ME, regLow != 0 ? regLow : -abs(l) % 256); /* check for immediate store */
            if (parseStk[sp].prec != 1) { /* now store high order byte (if any) */
                emit(INCX, RH, 0);        /* store second byte */
                /* regv(RH) = -3 then LHLD has occurred on symbol -regv(RL) */
                /* regv(RH) = -4 then LHLD AND INCX h has occurred */
                if (state[RH].regValue >= 0)
                    state[RL].regValue++;
                else if (state[RH].regValue == -3)
                    state[RH].regValue = -4;
                else /* RH AND RL have unknown values */
                    state[RH].regValue = state[RL].regValue = -1;

                if (parseStk[sp - 1].prec >= 2) {
                    if (regHigh == 0) /* second byte is literal */
                        emit(LD, ME, -abs(l / 256));
                    else /* LD memory from register */
                        emit(LD, ME, regHigh);
                } else
                    emit(LD, ME, 0);
            }
        }
    } else if (regLow == 0) /* load sp immediate */
        emit(LXI, RSP, l);
    else {
        emit(LD, RL, regLow);
        emit(LD, RH, regHigh);
        emit(SPHL, 0, 0);
        state[RH].regValue = state[RL].regValue = -1;
    }

    /* now release register containing address */
    /* release register assignment for value */
    /* if mov r,m inr r mov m,r was changed to inr m. */
    if (iq != codloc)
        pop(1);
    else {
        int16_t symId = -parseStk[sp].st;
        pop(2);
        parseStk[++sp] = stkItem(0, 1, symId, -1);
    }
}

void litadd(int s) {
    int assignment, k, j;
    /* load h AND l with the address of the variable at s in */
    /* the stack */

    if (parseStk[s].litv < 0)
        error("114: pass-2 compiler error in 'litadd'");
    else if ((assignment = parseStk[s].assignment) != REGPAIR(RH, RL)) { /* deassign registers */
        int regSt = state[RA].regSt;
        if ((k = REGLOW(assignment))) {
            if (k == regSt)
                state[RA].regSt = 0;
            state[k].regSt    = 0;
            state[k].lock     = false;
            state[k].regValue = -1;
        }
        if ((k = REGHIGH(assignment))) {
            if (k == regSt)
                state[RA].regSt = 0;
            state[k].regSt    = 0;
            state[k].lock     = false;
            state[k].regValue = -1;
        }
        parseStk[s].assignment = 0;
        int il                 = parseStk[s].litv % 256;
        int ih                 = parseStk[s].litv / 256;
        int lp;
        int it;
        for (int ir = RH; ir <= RL; ir++) {
            int regVal = ir == RH ? ih : il;
            if ((j = state[ir].regSt)) {
                if (REGLOW(parseStk[j].assignment) == ir)
                    parseStk[j].assignment &= ~0xf;
                if (REGHIGH(parseStk[j].assignment) == ir)
                    parseStk[j].assignment &= ~0xf0;
            }
            if ((lp = state[ir].regValue) != regVal) {
                if (lp == regVal + 1)
                    emit(DC, ir, 0); // dcr reg to make it match
                else if (lp == regVal - 1 && regVal != 0) {
                    emit(IN, ir, 0); // inc reg to make it match
                } else if (ir == RH && il != state[RL].regValue) {
                    // no inc/dec possible, see if l does NOT match
                    state[RL].regValue = il;
                    if (regVal <= 255) /* otherwise this is a real address */
                        emit(LXI, RH, il + ih * 256);
                    else { /* the LXI must be backstuffed later */
                        it = parseStk[s].st;
                        if (it >= 0) {
                            error("115: pass-2 compiler error in 'litadd'");
                            return;
                        }
                        it = -it;
                        emit(LXI, RH, symRef(it)); /* place reference into chain */
                        symRef(it) = codloc - 2;
                    }
                } else if (regVal <= 255)
                    emit(LD, ir, -regVal);
                else if ((it = parseStk[s].st) >= 0) { /* the address must be backstuffed later */
                    error("115: pass-2 compiler error in 'litadd'");
                    return;
                } else {
                    it = -it;
                    if (symAddr(it) <= 0) {
                        error("116: pass-2 compiler error in 'litadd'");
                        break;
                    }
                    /* place link into code */
                    int target  = HIGHWORD(symAddr(it));
                    symAddr(it) = ((codloc + 1) << 16) + LOWWORD(symAddr(it));
                    emit(0, LOW(target), 0);
                    emit(0, HIGH(target), 0);
                }
            }
            /* fix values IN stack AND reg */
            if (ir == RL)
                parseStk[s].assignment = REGPAIR(RH, RL);

            state[ir].regSt    = s;
            state[ir].regValue = regVal;
        }
    }
}

uint8_t const cbits[43 + 1] = { 0,   0x40, 4,   5,   0x80, 136, 144, 152, 160, 168, 176,
                                184, 7,    195, 194, 205,  196, 201, 192, 199, 219, 211,
                                118, 50,   58,  235, 249,  233, 47,  55,  63,  39,  34,
                                42,  251,  243, 1,   197,  193, 9,   2,   10,  3,   11 };
uint8_t const cc[4]         = { 0x10, 0x00, 0x30, 0x20 }; // false conditional code bits

/* the following comments are sample calls to the emit */
/* routine.  note that emit requires three argument at all times */
/* (the unused arguments are ZERO). */

/* emit(LD,RA,RB) */
/* emit(LD,RC,-34) */
/* emit(LD,RD,ME) */
/* emit(LD,ME,RE) */
/* emit(IN,RH,0) */
/* emit(DC,RL,0) */
/* emit(AD,RB,0) */
/* emit(AD,ME,0) */
/* emit(AD,-5,0) */
/* emit(SU,RB,0) */
/* emit(SB,ME,0) */
/* emit(ND,-5,0) */
/* emit(XR,0,0) */
/* emit(OR,RB,0) */
/* emit(CP,RH,0) */
/* emit(ROT,ACC,LFT) */
/* emit(ROT,CY,LFT) */
/* emit(ROT,CY,RGT) */
/* emit(JMP,148,0) */
/* emit(JMC,TRU+ZERO,148) */
/* emit(CAL,1048,0) */
/* emit(CLC,FAL+PARITY,148) */
/* emit(RTN,0,0) */
/* emit(RTC,FAL*32+CARRY,255) */
/* emit(RST,3,0) */
/* emit(INP,6,0) */
/* emit(OUT,10,0) */
/* emit(HALT,0,0) */
/* emit a literal between 0 AND 255 */
/* emit(0,44,0) */
/* emit(STA,300,0) */
/* emit(LDA,300,0) */
/* emit(XCHG,0,0) */
/* emit(SPHL,0,0) */
/* emit(PCHL,0,0) */
/* emit(CMA,0,0) */
/* emit(STC,0,0) */
/* emit(CMC,0,0) */
/* emit(DAA,0,0) */
/* emit(SHLD,300,0) */
/* emit(LHLD,300,0) */
/* emit(EI,0,0) */
/* emit(DI,0,0) */
/* emit(LXI,(RB,RD,RH,RSP),300) */
/* emit(PUSH,(RB,RD,RH,RA),0) */
/* emit(POP,(RB,RD,RH,RA),0) */
/* emit(DAD,(RB,RD,RH,RSP),0) */
/* emit(STAX,(RB,RD),0) */
/* emit(LDAX,(RB,RD),0) */
/* emit(INX,(RB,RD,RH,RSP),0) */
/* emit(DCX,(RB,RD,RH,RSP),0) */

void emit(int opr, int opa, int opb) {
    uint8_t opcode;
    uint16_t operand;

    uint8_t byteCnt = 1;
    if (C_NUMERIC) /* write emitter trace */
        fprintf(lstFp, "E(%d,%d,%d)\n", opr, opa, opb);

    if (opr <= 0)
        opcode = opa;

    else {
        opcode = cbits[opr];
        switch (opr) {
        case LD:            /* mov group */
            if (opb <= 0) { /* lri operation */
                if (v4Opt && lastLoadImm == codloc - 2 && lastRegImm / 2 == opa / 2 &&
                    RB <= lastRegImm && lastRegImm <= RL) {
                    // consecutive mvi r,imm, to same register pair or register
                    if (lastRegImm == opa) {   // mvi r,imm2 followed by mvi r,imm2
                        put(codloc - 1, -opb); // optimised to single mvi r,imm2
                        return;
                    }
                    // mvi r,imm followed by mvi r,imm to other register of pair
                    opcode = 1 + ((opa / 2 - 1) << 4); // optimise to lxi r,imm16
                    put(codloc - 2, opcode);           // replace op with lxi r,
                    byteCnt     = 1;                   // lxi is 3 bytes, already have 2
                    lastLoadImm = 0;                   // can't optimise again
                    if (opa % 2 == 0)                  // now have high byte so append
                        opcode = -opb;
                    else {                        // now have low byte so swap
                        opcode = get(codloc - 1); // will append high byte
                        put(codloc - 1, -opb);    // insert low byte
                    }
                } else {                  // couldn't optimise
                    lastLoadImm = codloc; // update for further optimisations
                    lastRegImm  = opa;
                    byteCnt     = 2;                        /* 2 byte instruction */
                    opcode      = 0x6 + (regmap[opa] << 3); /* gen mvi instruction */
                    operand     = -opb;                     /* and the value */
                }
            } else { // check for possible load register elimination
                // if mov m,r followed by mov r,m then optimise 2nd instruction away
                if (opa != ME) {
                    if (opb == ME && lastLoad == codloc - 1 && lastReg == opa)
                        return;

                    // may change a mov r,m inr/dcr r mov m,r to inr/dcr m
                } else if (lastIncReg == codloc - 1) {
                    // other code has flagged this is inc of same register
                    opcode = (get(codloc - 1) & 7) + 0x30; // generate the inr/dcr

                    /* the register load may have been eliminated... */
                    if (lastLoad != codloc - 2 || opb != lastReg)
                        codloc--;
                    put(codloc - 1, opcode);
                    lastIncReg = 0; // prevent further attempts to optimise
                    lastReg    = 0;
                    lastLoad   = 0;
                    if (lastin == codloc || lastin == codloc + 1)
                        lastin = codloc - 1;
                    return;
                } else {
                    lastLoad = codloc;
                    lastReg  = opb;
                }
                // this is a load memory from register operation - save
                opcode += (regmap[opa] << 3) + regmap[opb];
            }
            break;
        case IN:
        case DC:
            opcode += regmap[opa] << 3; /* gen inr / dcr */
            break;
            /* Arith group */
        case AD:
        case AC:
        case SU:
        case SB:
        case ND:
        case XR:
        case OR:
        case CP:
            if (opa > 0)
                opcode += regmap[opa];
            else {           /* immediate operand */
                byteCnt = 2; // normally 2 byte code
                if (v4Opt && opa == 0) {
                    switch (opr) {
                    case AD:
                    case SU:
                    case XR:
                    case OR: // replace adi,0 sui,0 xri,0 or ori 0 with ora,a
                             // case CP: // would also work, but not used in V4
                        opcode  = cbits[OR] + regmap[RA];
                        byteCnt = 1;
                        break;
                    case ND: // replace ani,0 with xra,a
                        opcode  = cbits[XR] + regmap[RA];
                        byteCnt = 1;
                        break;
                    }
                }
                if (byteCnt == 2) { // make the 2 byte code
                    opcode += 0x46; /* gen imm arith instruction */
                    operand = -opa; /* the immediate value*/
                }
            }
            break;
        case ROT: /* rotate group */
            opcode += opa + opb;
            break;

        case JMC:
        case CLC: /* conditional jmp & call */
            byteCnt = 3;
            operand = opb;
            // fallthrough
        case RTC: /* conditional return */
            opcode += cc[(opa - CARRY) & 3] + (opa & TRU);
            break;
        case RST: /* rst xx */
            opcode += (opa & 7) << 3;
            break;
        case INP:
        case OUT: /* in & out */
            byteCnt = 2;
            operand = opa;
            break;
        case JMP:
        case CAL: /* jmp & call */
        case STA:
        case LDA:
        case SHLD:
        case LHLD: /* STA LDA SHLD LHLD */
            byteCnt = 3;
            operand = opa;
            break;
        case XCHG:
            if (lastex == codloc - 1) { /* remove double xchg*/
                codloc--;
                lastex = 0;
                return;
            }
            lastex = codloc; /* mark this xchg */
            break;
        case RTN:
        case HALT:
        case SPHL:
        case PCHL:
        case CMA:
        case STC:
        case CMC:
        case DAA:
        case EI:
        case DI:
            break;
        case LXI:                       /* LXI (get immediate part) */
            byteCnt = 3;                /* 3 byte instruction */
            opcode += regmap[opa] << 3; /* merge in the register */
            operand = opb;              /* and the location */
            break;
        case PUSH:
            /* check for XCHG PUSH d combination. change to PUSH h */
            // PMO, relies on no code being generated that uses existing DE or HL after this
            if (lastex == codloc - 1 && opa == RD) {
                codloc--;
                lastex = 0;
                opcode += regmap[RH] << 3;
                break;
            }
            // fallthrough
        case POP:
            if (opa == RA) {
                opcode += 6 << 3; /* push/pop psw */
                break;
            }
            // fallthrough
        case DAD:
        case STAX:
        case LDAX:
        case INCX:
        case DCX:
            opcode += regmap[opa] << 3;
            break;
        }
    }
    put(codloc++, opcode);
    if (byteCnt > 1) {
        put(codloc++, operand % 256);
        if (byteCnt > 2)
            put(codloc++, operand / 256);
    }
}

void cvcond(int s) {

    /* convert the condition code at s in the stack to a boolean value */
    int assign = LOW(parseStk[s].assignment);
    int ia     = assign & 0xf;
    int cond   = COND(parseStk[s].assignment);

    /* test = 1 if CARRY, 2 if ZERO, 3 if SIGN, AND 4 if PARITY */

    /* we may generate a short sequence */
    if ((cond & ~TRU) <= ZERO && ia && state[RA].regSt == ia) {
        if ((cond & ~TRU) == CARRY) { /* short conversion for true or false CARRY */
            emit(SB, RA, 0);
            if (!(cond & TRU))
                emit(CMA, 0, 0);
        } else {            /* short conversion for true or false ZERO */
            if (cond & TRU) // true if true/ZERO
                emit(SU, -1, 0);
            else
                emit(AD, -255, 0);
            emit(SB, RA, 0);
        }
    } else { /* do we have to assign a register */
        if (ia == 0) {
            ia = genReg(1);
            if (ia) {
                state[ia].regSt = sp;
                assign          = ia;
            } else {
                Fatal("118: register allocation error");
                return;
            }
        }

        /* check pending register store */
        int jp = state[RA].regSt;
        if (jp && jp != ia) {
            emit(LD, jp, RA);
            state[RA].regSt = 0;
        }
        emit(LD, RA, -255);
        emit(JMC, cond, codloc + 4);
        emit(XR, RA, 0);
    }

    /* set up pending register store */
    state[RA].regSt        = ia;
    parseStk[s].assignment = assign;
}

void saver() {
    /* save the active registers and reset tables */
    /* first determine the stack elements which must be saved */

    if (sp != 0) {
        int16_t wordChain = 0, byteChain = 0;
        uint8_t wordCnt = 0, byteCnt = 0;
        for (int j = 1; j <= sp; j++) {
            uint16_t regPair = parseStk[j].assignment;
            if (COND(regPair)) {
                cvcond(j);
                regPair = parseStk[j].assignment;
            }
            if (regPair >= 16) { /* double byte */
                if (!(state[REGLOW(regPair)].lock || state[REGHIGH(regPair)].lock)) {
                    parseStk[j].st = wordChain;
                    wordChain      = j;
                    wordCnt++;
                }
            } else if (regPair > 0 && !state[REGLOW(regPair)].lock) { /* single byte */
                parseStk[j].st = byteChain;
                byteChain      = j;
                byteCnt++;
            }
        }
        lmem -= byteCnt + wordCnt * 2;
        if (byteCnt == 0 && wordCnt != 0 && lmem % 2)
            lmem--;

        /* lmem is now properly aligned. */
        if (lmem < 0) {
            error("119: memory allocation error");
            return;
        } else {
            int loc = lmem;
            int regSt;
            while (byteChain + wordChain) {
                if ((loc & 1) || wordChain == 0) /* single byte */
                    byteChain = parseStk[regSt = byteChain].st;
                else /* even byte boundary with double bytes to store */
                    wordChain = parseStk[regSt = wordChain].st;

                if (regSt <= 0) {
                    error("120: memory allocation error");
                    return;
                }

                /* place temporary into symbol table */
                parseStk[regSt].st = ++sytop;
                symbol[sytop]      = syinfo;

                uint8_t prec       = REGHIGH(parseStk[regSt].assignment) ? 2 : 1;

                symbol[syinfo]     = loc;
                loc += prec;
                symbol[--syinfo] = PACK_ATTRIB(1, prec, VARB);

                /* leave room for LXI chain */
                symbol[--syinfo] = 0;
                if (sytop > --syinfo) {
                    Fatal("121: pass-2 symbol table overflow");
                    return;
                }

                /* store into memory */
                int regPair                = parseStk[regSt].assignment;
                parseStk[regSt].assignment = 0;
                sp++;
                setadr(sytop);
                litadd(sp);
                while (regPair) {
                    uint8_t reg = REGLOW(regPair);
                    if (reg == state[RA].regSt) {
                        reg                = RA;
                        state[RA].regSt    = 0;
                        state[RA].regValue = -1;
                    }
                    emit(LD, ME, reg);
                    if ((regPair = REGHIGH(regPair))) { /* double byte store */
                        emit(IN, RL, 0);
                        state[RL].regValue++;
                    }
                }
                pop(1);
            }
        }
    }
    for (int reg = RB; reg <= RL; reg++)
        if (!state[reg].lock) {
            state[reg].regSt    = 0;
            state[reg].regValue = -1;
        }
    return;
}

void reloc() {
    if (C_SYMBOLS >= 2) {
        for (int reg = 1; reg <= sytop; reg++)
            fprintf(lstFp, "%4d = %6d\n", reg, symbol[reg]);
        for (int reg = syinfo; reg <= symax; reg++)
            fprintf(lstFp, "%5d = %c%08XH\n", reg, (symbol[reg] < 0) ? '-' : ' ', abs(symbol[reg]));
    }

    /* compute max stack depth required for correct execution */
    uint16_t stsize = maxdep[0] + extraStack;
    for (int n = 0; n < 8; n++) {
        if (intpro[n]) /* get interrupt procedure depth */
            /* note that i exceeds depth by 1 since RET may be pending */
            stsize += symIProcDepth(intpro[n]) + 1;
    }
    stsize *= 2;
    uint16_t n = C_STACKHANDLING == 0 ? stsize : 0;

    /* align to even boundary, if necessary */
    if (n != 0 && lmem % 2 == 1)
        lmem--;
    int stloc = lmem;
    lmem -= n;

    /* stsize is number of bytes reqd for stack, stloc is addr */

    /* compute page to start variables */
    int page = (codloc % 256 > lmem % 256 ? 1 : 0) + codloc / 256;
    if (C_VARIABLES > page)
        page = C_VARIABLES;

    /* compute first relative address page */
    int relPage = lmem / 256 - page;
    if (relPage < 0)
        error("122: program requires too much program and variable storage");

    else {
        for (int symId = 1; symId <= sytop; symId++) {
            int symAddr = symAddr(symId);
            if (symAddr >= 0) {                           /* now fix page number */
                int actualPage = HIGH(symAddr) - relPage; /* l is relocated page number */
                symAddr(symId) = (actualPage << 8) + LOW(symAddr);

                /* backstuff lhi l into location target-1 */
                for (int next, target = HIGHWORD(symAddr); target; target = next) {
                    next = get(target - 1) * 256 + get(target);
                    put(target - 1, 0x26);
                    put(target, actualPage);
                }
                /* backstuff LXI references to this variable */
                int target = symAddr(symId);
                for (int next, link = symRef(symId); link; link = next) {
                    next = getword(link);
                    putword(link, target);
                }
            }
        }
        if (C_MAP)
            putc('\n', lstFp);

        /* relocate AND backstuff the stack top references */
        stloc -= relPage * 256;
        while (lxis != 0) {
            int fixup = lxis;
            lxis      = getword(fixup);
            putword(fixup, stloc);
        }
        if (C_STACKHANDLING == 1)
            fprintf(lstFp, "STACK SIZE OVERRIDDEN\n");
        else
            fprintf(lstFp, "STACK SIZE = %d BYTES\n", stsize);

        /* now backstuff all other TRC, TRA, AND PRO addresses */
        for (int symId = 1; symId <= sytop; symId++) {

            uint8_t type = INFO_TYPE(symAttrib(symId));
            if (type == LABEL || type == PROC) {
                int symFix  = -symAddr(symId);
                int addr    = symFix / 4; // does right thing if k -1 .. -3
                int backref = symRef(symId);
                while (addr) { // V4
                    int next = getword(addr);
                    putword(addr, backref);
                    addr = next;
                }
                symAddr(symId) = symFix % 4;
            }
        }
        if (preamb > 0) {
            for (int intId = 0; intId < 8; intId++)
                if (intpro[intId]) {
                    put(intId * 8, 0xc3);
                    putword(intId * 8 + 1, symRef(intpro[intId]));
                }
            if (intpro[0] == 0 && codeBase == preamb && C_STACKHANDLING != 1) {
                put(0, 0xc3);
                putword(1, codeBase);
            } // V4
        }
    }
}

void put2(int ch1, int ch2, int prec) {
    if (prec == 2) {
        put(codloc++, ch2);
        put(codloc++, ch1);
    } else {
        put(codloc++, ch1);
        put(codloc++, ch2);
    }
}

void put1(int ch, int prec) {
    put(codloc++, ch);
    if (prec == 2)
        put(codloc++, 0);
}

void unary(int val) {
    uint8_t reg, regSt, ia, ib;
    bool singleReg;
    uint16_t regpair;

    /* 'val' is an integer corresponding to the operations-- */
    /* RTL(0) RTR(1) SFL(2) SFR(3) SCL(4) SCR(5) HIV(6) LOV(7) */
    if (COND(parseStk[sp].assignment))
        cvcond(sp);
    uint8_t prec = parseStk[sp].prec;
    switch (val) {
    case B_ROL:
    case B_ROR:
        if (prec <= 1) {
            if (parseStk[sp].assignment == 0) {
                loadv(sp, 1);
                state[RA].regSt = REGLOW(parseStk[sp].assignment);
            }
            reg   = REGLOW(parseStk[sp].assignment);
            regSt = state[RA].regSt;
            if (regSt != 0) {
                if (regSt != reg) {
                    emit(LD, regSt, RA);
                    emit(LD, RA, reg);
                    state[RA].regSt = reg;
                }
            } else {
                emit(LD, RA, reg);
                state[RA].regSt = reg;
            }

            emit(ROT, CY, val == B_ROR ? RGT : LFT);
            return;
        }
        break;
    case B_SHL:
    case B_SHR:
    case B_SCL: // SCL
    case B_SCR: // SCR

        singleReg = (val != B_SHR && val != B_SCR) || prec == 1;

        regpair   = parseStk[sp].assignment;
        if (regpair <= 0) { /* load from memory */
            loadv(sp, singleReg);
            regpair = parseStk[sp].assignment;
            if (singleReg)
                state[RA].regSt = REGLOW(regpair);
        }

        /* may have to store the accumulator */
        ia    = REGLOW(regpair);
        ib    = REGHIGH(regpair);
        reg   = singleReg ? ia : ib;

        regSt = state[RA].regSt;

        /* we want register k to be IN the accumulator */
        if (regSt != reg) {
            if (regSt)
                emit(LD, regSt, RA);
            emit(LD, RA, reg);
        }
        state[RA].regSt = reg;

        /* SFL AND SFR take separate paths now... */
        if (val == B_SHR || val == B_SCR) {
            if (val == B_SHR)
                emit(OR, RA, 0);
            emit(ROT, ACC, RGT);
            if (prec >= 2) {
                emit(LD, ib, RA);
                emit(LD, RA, ia);
                emit(ROT, ACC, RGT);
                state[RA].regSt = ia;
            }
        } else {
            /* SFL - clear CARRY AND shift */
            if (val == B_SHL)
                emit(AD, RA, RA);
            else if (val == B_SCL)
                emit(ROT, ACC, LFT);
            if (prec >= 2) {
                emit(LD, ia, RA);
                emit(LD, RA, ib);
                emit(ROT, ACC, LFT);
                state[RA].regSt = ib;
            }
        }
        return;
    case B_TIME:
        break;
    case B_HIGH:
        if (prec >= 2) {
            if (parseStk[sp].assignment <= 0)
                loadv(sp, 0);
            int regHigh = REGHIGH(parseStk[sp].assignment);
            int regLow  = REGLOW(parseStk[sp].assignment);
            if (state[RA].regSt == regLow)
                state[RA].regSt = 0;
            state[regHigh].regSt    = 0;
            state[regHigh].regValue = -1;
            parseStk[sp].assignment = regLow;
            parseStk[sp].prec       = 1;
            if (state[RA].regSt != regHigh)
                emit(LD, regLow, regHigh);
            else
                state[RA].regSt = regLow;
            return;
        }
        break;
    case B_LOW:
        parseStk[sp].prec = 1;
        /* may have to release register */
        if ((reg = REGHIGH(parseStk[sp].assignment))) {
            state[reg].regSt    = 0;
            state[reg].regValue = -1;
            if (state[RA].regSt == reg)
                state[RA].regSt = 0;
        }
        parseStk[sp].assignment = REGLOW(parseStk[sp].assignment);
        return;
    }
    error("126: built-in function improperly called");
    return;
}

void exch() {

    /* exchange the top two elements of the stack - check if both pushed */
    if (parseStk[sp - 1].st == 0 && parseStk[sp - 1].assignment == 0 && parseStk[sp - 1].litv < 0 &&
        parseStk[sp].st == 0 && parseStk[sp].assignment == 0 && parseStk[sp].litv < 0) {
        /* both are pushed */

        for (uint8_t j = 0; j <= 1; j++) {
            /* POP element */
            uint16_t regpair = genReg(-1);
            if (regpair == 0) {
                Fatal("107: register allocation error. No registers available");
                break;
            } else {
                emit(POP, regpair - 1, 0);
                ustack();
                state[regpair].regSt = sp - j;
                if (parseStk[j].prec > 1) {
                    state[regpair - 1].regSt = sp - j;
                    regpair                  = REGPAIR(regpair - 1, regpair);
                }
                parseStk[sp - j].assignment = regpair;
            }
        }
    }

    for (int reg = RB; reg <= RL; reg++) {
        if (state[reg].regSt == sp)
            state[reg].regSt = sp - 1;
        else if (state[reg].regSt == sp - 1)
            state[reg].regSt = sp;
    }
    /* now swap the top two elements */
    regAlloc_t tmp   = parseStk[sp];
    parseStk[sp]     = parseStk[sp - 1];
    parseStk[sp - 1] = tmp;
    return;
}

void stack(const int n) {
    /* ADD n to current depth, test for stacksize exceeding maxdepth */
    curdep[prsp] += n;
    if (curdep[prsp] > maxdep[prsp])
        maxdep[prsp] = curdep[prsp];
    return;
}

void readcd() {
    int reg, j, k;
    int ip;
    char *rmap       = "-ABCDEHL";
    char polchr[][4] = { "OPR", "ADR", "VLU", "DEF", "LIT", "LIN" };
    char opcval[][4] = { "NOP", "ADD", "ADC", "SUB", "SBC", "MUL", "DIV", "MDF", "NEG",
                         "AND", "IOR", "XOR", "NOT", "EQL", "LSS", "GTR", "NEQ", "LEQ",
                         "GEQ", "INX", "TRA", "TRC", "PRO", "RET", "STO", "STD", "XCH",
                         "DEL", "DAT", "LOD", "BIF", "INC", "CSE", "END", "ENB", "ENP",
                         "HAL", "RTL", "RTR", "SFL", "SFR", "HIV", "LOV", "CVA", "ORG",
                         "DRT", "ENA", "DIS", "AX1", "AX2", "AX3" };

    C_COUNT          = 1;
    int lline        = 0;
    int lloc         = 0;
    int lcnt         = -1;
    alter            = false;

    /* reserve space for interrupt locations */
    preamb = 0;
    for (int rst = 7; rst >= 0; rst--) {
        if (intpro[rst] != 0) {
            preamb = rst * 8 + 3;
            break;
        }
    }

    /* adjust codloc to account for preamble */
    if (codloc < preamb)
        codloc = preamb;

    /* allocate 'preamble' cells at start of code */
    codeBase = codloc;

    /* set stack pointer upon program entry */
    if (C_STACKHANDLING != 1) {   /* 1 -> user handled */
        if (C_STACKHANDLING == 0) /* 0 -> system handled else stack size */

            /* start chain of lxis */
            lxis = codloc + 1;
        emit(LXI, RSP, C_STACKHANDLING);
    }
    while (!errflg) {
        uint8_t ibase = 0;

        /* may have been stack overflow so... */
        if (sp < 0)
            sp = 0;
        if (C_ANALYSIS && alter && sp > 0) {
            /* write stack */
            fprintf(lstFp, "   PR   ST   RASN  LITV\n");
            for (int ip = sp; ip > 0; ip--) {
                fprintf(lstFp, "%02d %d ", ip, parseStk[ip].prec);
                if (parseStk[ip].st)
                    fprintf(lstFp, " %c%05d", parseStk[ip].st < 0 ? 'A' : 'S',
                            abs(parseStk[ip].st));
                else
                    fputs("       ", lstFp);

                fprintf(lstFp, "  %c %c", rmap[REGHIGH(parseStk[ip].assignment)],
                        rmap[REGLOW(parseStk[ip].assignment)]);
                if (parseStk[ip].litv >= 0)
                    fprintf(lstFp, " %c%05d", HIGHWORD(parseStk[ip].litv) ? 'R' : ' ',
                            LOWWORD(parseStk[ip].litv));
                putc('\n', lstFp);
            }

            /* write registers */
            if (C_ANALYSIS >= 2) {
                for (int reg = RA; reg <= RL; reg++) {
                    ip     = state[reg].regSt;
                    int kp = state[reg].lock;
                    int lp = state[reg].regValue;
                    if (kp + ip + lp >= 0) {
                        fprintf(lstFp, " %c(%c,", rmap[reg], kp == 1 ? 'L' : 'U');
                        if (ip == 0)
                            putc('*', lstFp);

                        else
                            fprintf(lstFp, "%02d", ip);
                        putc(',', lstFp);
                        if (lp < 0)
                            putc('*', lstFp);

                        else
                            fprintf(lstFp, "%04XH", lp);
                        putc(')', lstFp);
                    }
                }
                putc('\n', lstFp);
            }
        }

        do {
            /* copy the elt just read to the polish look-ahead, AND */
            /* interpret the previous elt */
            k = lapol;
            if (lapol != FIN && (lapol = getNextPol()) == EOF) {
                lapol = FIN;
                Fatal("127: invalid intermediate language format");
                return;
            }
        } while (k < 0);

        /* check for END of code */
        if (k == FIN)
            break;
        uint8_t typ  = k & 7;
        uint16_t val = k >> 3;

        /* $g=0 for no trace, $g=1 gives lines vs locs, */
        /* $g=2 yields full interlist of i.l. */
        if (C_GENERATE != 0) {
            if (C_GENERATE > 1) {
                /* otherwise interlist the i.l. */
#if 0
                FILE *tmp = lstFp;
                lstFp     = stdout;
#endif
                fprintf(lstFp, "%05d %04XH %d %s ", codloc, codloc, polCnt - 1, polchr[typ]);
                switch (typ) {
                case OPR:
                    fprintf(lstFp, "%s", opcval[val]);
                    break;
                case ADR:
                case VLU:
                case DEF:
                    fprintf(lstFp, "S%05d", val);
                    break;
                case LIT:
                case LIN:;
                    fprintf(lstFp, " %05d", val);
                }
                putc('\n', lstFp);
#if 0
                lstFp = tmp;
#endif
            } else
                /* print line number = code location, if altered */
                if (lline != C_COUNT && lloc != codloc) {
                    /* changed completely, so print it */
                    if (lline == 0)
                        lcnt = C_WIDTH / 12;
                    else if (lcnt <= 0) {
                        lcnt = C_WIDTH / 12;
                        putc('\n', lstFp);
                    } else
                        fputs("  ", lstFp);
                    lcnt--;
                    lline = C_COUNT;
                    lloc  = codloc;

                    fprintf(lstFp, "%4d=%04XH", lline, lloc);
                }
        }
        if (++sp > maxsp) { /* stack overflow */
            Fatal("128: register allocation stack overflow");
            sp = 1;
        }
        parseStk[sp] = stkItem(0, 0, 0, -1);

        alter        = false;
        switch (typ) {
        case OPR: /* operator */
            sp--;
            alter |= operat(val);
            continue;
        case ADR:
            /* check for active condition code which must be changed to boolean */
            if (sp > 1 && COND(parseStk[sp - 1].assignment))
                cvcond(sp - 1);

            if (symAttrib(val) >= 0) {
                setadr(val);
                continue;
            } else {
                /* load address of based variable.  change to */
                /* load value of the base, using the variable's precision */
                ibase = INFO_PREC(-symAttrib(val));
                val   = symRef(val);
            }
            break;
        case VLU:
            break;
        case DEF: /* mark last register load nil */
            lastReg     = 0;
            lastex      = 0;
            lastin      = 0;
            lastIncReg  = 0;
            lastLoadImm = 0; // only used in V4
            sp--;

            /* save registers if this is a PROC or a LABEL which was */
            /* referenced in a go-to statement or was compiler-generated. */
            ip  = symbol[val];
            reg = abs(symAttrib(val));

            /* save this DEF symbol number and the literal values of the */
            /* h AND l registers for possible TRA chain straightening. */
            if (INFO_TYPE(reg) == LABEL) {
                defsym = val;
                defrh  = state[RH].regValue;
                defrl  = state[RL].regValue;
            }

            /* we may convert the sequence */

            /* TRC l, TRA/PRO/RET, DEF l */
            /* to an equivalent conditional TRA/PRO/RET... */
            if (INFO_ECNT(reg) == 1 && tstloc == codloc && conloc == xfrloc - 3) {
                int addr = -symAddr(val);

                if ((addr >> 2) == conloc + 1) {
                    /* adjust backstuffing chain for JMP OR call */
                    if (xfrsym > 0) /* decrement backstuff location by 3 - note -ve */
                        symAddr(xfrsym) += 3 << 2;

                    /* arrive here with the configuration TRC...DEF */
                    symAddr(val) = -(addr & 3);
                    if (symAttrib(val) < 0)
                        symAttrib(val) = -(abs(symAttrib(val)) & 0xff);
                    else
                        symAttrib(val) &= 0xff;
                    // invert the conditional + merge with the TRA/PRO/RET
                    int j = ((get(conloc) ^ 8) & ~7) + (get(xfrloc) & 6);
                    do {
                        put(conloc, j);
                        conloc++;
                        xfrloc++;
                        j = get(xfrloc);
                    } while (xfrloc != codloc);
                    codloc = conloc;
                    conloc = xfrloc = tstloc = -1;
                    /* notice that defrh & defrl are now incorrect */
                    /* defsym=0 prevents use of these variables... */
                    /* ... if a TRA immediately follows */
                    defsym = 0;
                }
            }

            switch (INFO_TYPE(reg)) {
            case LABEL:
                if (INFO_ECNT(reg) == 1) { /* single reference, so no conflict with h and l */
                    int trackHL = symTrackHL(val);
                    /* check for previous reference  forward */
                    if (trackHL && trackHL != -1) {
                        int16_t regL   = (trackHL & LVALID) ? trackHL & 0xff : -1;
                        int16_t regH   = (trackHL & HVALID) ? (trackHL >> 8) & 0x1ff : -1;
                        state[RH].lock = true; // don't spill H or L
                        state[RL].lock = true;
                        saver();

                        state[RH].lock = false; // remove lock
                        state[RL].lock = false;
                        // check for changes
                        state[RH].regValue =
                            (state[RH].regValue == -255 || state[RH].regValue == regH) ? regH : -1;
                        state[RL].regValue =
                            (state[RL].regValue == -255 || state[RL].regValue == regL) ? regL : -1;
                    } else
                        saver();
                } else if (INFO_ECNT(reg))
                    saver();
                k = codloc;
                break;
            case PROC: /* set up procedure stack for procedure entry */
                if (++prsp <= prsmax) {
                    prstk[prsp] = TrackHL(ip); // index of symTrackHL

                    /* mark h AND l as unaltered initially */
                    /* /  1b  /  1b  /  1b  /  1b  /  9b  /  8b  / */
                    /* /h unal/l unal/h vald/l vald/h valu/l valu/ */
                    /* ------------------------------------------- */
                    symbol[TrackHL(ip)] = HNOTSET | LNOTSET;
                    saver();
                    state[RH].regValue = -254;
                    state[RL].regValue = -254;
                    k                  = codloc;

                    /* set up stack depth counters */
                    maxdep[prsp] = 0;
                    curdep[prsp] = 0;
                    for (int reg = 0; reg < 8; reg++)
                        if (val == intpro[reg]) {
                            /* interrupt procedure is marked with ho 1 */
                            prstk[prsp] += 0x10000;
                            emit(PUSH, RH, 0);
                            emit(PUSH, RD, 0);
                            emit(PUSH, RB, 0);
                            emit(PUSH, RA, 0);
                            stack(4);
                        }
                } else {
                    Fatal("146: too many procedures nested");
                    saver();
                    k = codloc;
                }
                break;
            default:
                saver();

                k = codloc;
                break;
            }

            /* LABEL is resolved.  last two bits of entry must be 01 */
            if ((-symAddr(val) & 3) != 1)
                error("131: label resolution error in pass-2");

            symAddr(val) = -(-symAddr(val) | 3);
            symRef(val)  = k;
            /* now check for procedure entry point */
            reg = symAttrib(val);
            if (INFO_TYPE(reg) == PROC) {
                uint8_t paramCnt = INFO_ECNT(reg);

                /* build receiving sequence for register parameters */
                if (paramCnt >= 1) {
                    uint8_t k = paramCnt < 2 ? 1 : paramCnt - 1;
                    if (paramCnt > 2)
                        paramCnt = 2;
                    for (int j = 0; j < paramCnt; j++) {
                        if (++sp > maxsp) {
                            Fatal("113: register allocation stack overflow");
                            sp = 1;
                        }

                        parseStk[sp] =
                            stkItem(j == 0 ? REGPAIR(RB, RC) : REGPAIR(RD, RE), 2, 0, -1);
                        if (++sp > maxsp) {
                            Fatal("113: register allocation stack overflow");
                            sp = 1;
                        }
                        parseStk[sp].assignment = 0;
                        parseStk[sp].litv       = -1;
                        setadr(val + k + j);
                        alter |= operat(STD);
                    }
                }
            }
            continue;
        case LIT:
            /* check for active condition code which must be changed to boolean */
            if (sp > 1 && COND(parseStk[sp - 1].assignment))
                cvcond(sp - 1);
            alter             = true;
            parseStk[sp].litv = val;
            parseStk[sp].prec = parseStk[sp].litv > 255 ? 2 : 1;
            continue;
        case LIN:
            /* line number */
            C_COUNT = val;
            sp--;
            continue;
        }
        reg = symbol[val];
        j   = symAttrib(val);
        if (sp > 1)
            /* allow only a LABEL variable to be stacked */
            if (abs(j) % 16 != LABEL) {
                /* check for active condition code which must be changed to boolean */
                if (COND(parseStk[sp - 1].assignment))
                    cvcond(sp - 1);
            }
        /* check for condition codes */
        if (val < intbas) {
            if (val <= S_PARITY) { /* CARRY ZERO MINUS PARITY */
                parseStk[sp].assignment = ASSIGN(TRU + val, 0, 0);
                parseStk[sp].st         = 0;
                parseStk[sp].prec       = 1;
                alter                   = true;
                continue;
            } else if (val == S_STACKPTR) {
                /* load value of stack pointer to registers immediately */
                uint16_t regPair = genReg(2);
                if (!regPair)
                    Fatal("107: register allocation error. No registers available");
                else {
                    parseStk[sp]                  = stkItem(regPair, 2, 0, -1);
                    state[REGHIGH(regPair)].regSt = state[REGLOW(regPair)].regSt = sp;

                    emit(LXI, RH, 0);
                    emit(DAD, RSP, 0);
                    emit(LD, REGLOW(regPair), RL);
                    emit(LD, REGHIGH(regPair), RH);
                    state[RH].regValue = -1;
                    state[RL].regValue = -1;
                    alter              = true;
                }
                continue;
            }
        }
        if (j < 0)
            /* value reference to based variable. first ensure that this */
            /* is not a length attribute reference, (i.e., the variable is */
            /* not an actual parameter for a call on length or last) by */
            /* insuring that the next polish elt is not an address */
            /* reference to symbol (length+1) OR (last+1) */
            if (lapol != ((S_LAST + 1) << 3) + ADR && lapol != ((S_LENGTH + 1) << 3) + ADR) {
                /* load value of base variable.  change to load */
                /* value of base, followed by a LOD op. */
                ibase = 16 + ((-j >> 4) & 0xf);
                val   = symRef(val);
                j     = symAttrib(val);
            }
        alter = true;

        /* examine attributes */
        parseStk[sp].st = val;
        k = parseStk[sp].prec = ibase > 0 ? ibase & 0xf : INFO_PREC(j);
        if (INFO_TYPE(j) >= LITER) {
            if (k == 1 || k == 2) // byte, address
                parseStk[sp].litv = INFO_ECNT(j);
            else {
                error("130: pass-2 compiler error. Invalid variable precision");
                continue;
            }
        }

        if (ibase >= 16) { /* check for base address which must be loaded */
            /* low bits base precision, higher bits based variable precision */
            parseStk[sp].prec = (parseStk[sp].prec << 2) + 2;
            alter |= operat(LOD);
        }
    }
    emit(EI, 0, 0);
    emit(HALT, 0, 0);

    /* may be line/loc's left IN output buffer */
    if (C_GENERATE != 0)
        putc('\n', lstFp);
    ;
}

void doPRO(int16_t symId) {
    /* pass the last two (at most) parameters in the registers */
    uint8_t regParamCnt = min(INFO_ECNT(symAttrib(symId)), 2);
    if (regParamCnt == 0) { // no parameters
        state[RH].lock = state[RL].lock = true;
        saver();
        state[RH].lock = state[RL].lock = false;
    } else {
        uint8_t firstSp = sp - 2 * regParamCnt;
        for (int stkIdx = firstSp; stkIdx < sp; stkIdx += 2) {
            uint8_t regLow  = REGLOW(parseStk[stkIdx].assignment);
            uint8_t regHigh = REGHIGH(parseStk[stkIdx].assignment);
            if (regLow)
                state[regLow].lock = true;
            if (regHigh)
                state[regHigh].lock = true;
            parseStk[stkIdx].prec = min(parseStk[stkIdx].prec, parseStk[stkIdx + 1].prec);
            if (parseStk[stkIdx].prec <= 1 && regHigh) {
                // clear pending store if passing address var to byte
                if (state[RA].regSt == regHigh)
                    state[RA].regSt = 0;
                state[regHigh].regSt = 0;
                state[regHigh].lock  = false;
                regHigh              = 0;
                // test appears to be odd but retained for now
                if (state[RA].regSt == regLow || state[RA].regSt == 0)
                    state[RA].lock = true;
            }
            parseStk[stkIdx].assignment = REGPAIR(regHigh, regLow);
        }

        /* stack any stuff which does not go to the procedure */
        bool pushed = false;

        for (int stkIdx = 1; stkIdx <= sp; stkIdx++) { /* check for value to PUSH */
            if (parseStk[stkIdx].assignment == 0) {
                /* registers NOT assigned - check for stacked value */
                if (parseStk[stkIdx].st == 0 && parseStk[stkIdx].litv < 0 && pushed)
                    error("150: pass-2 compiler error in 'loadv'");
            } else if (stkIdx < firstSp) { /* possible PUSH if not a parameter */
                                      /* registers must be pushed */
                uint8_t regHigh = REGHIGH(parseStk[stkIdx].assignment);
                uint8_t regSt   = state[RA].regSt;
                uint8_t regLow  = REGLOW(parseStk[stkIdx].assignment);
                if (regSt) {                /* pending ACC store, check ho and lo registers */
                    if (regSt == regHigh) { /* pending ho byte store */
                        emit(LD, regHigh, RA);
                        state[RA].regSt = 0;
                    }
                    if (regSt == regLow) { /* check lo byte */
                        emit(LD, regLow, RA);
                        state[RA].regSt = 0;
                    }
                }
                emit(PUSH, regLow - 1, 0);
                stack(1);
                parseStk[stkIdx].st = 0;
                if (regLow)
                    state[regLow].regSt = 0;
                if (regHigh)    
                    state[regHigh].regSt = 0;

                parseStk[stkIdx].assignment = 0;
                parseStk[stkIdx].litv       = -1;
                pushed = true;
            }
        }
        uint8_t it     = RH;
        for (int stkIdx = firstSp; stkIdx < sp; stkIdx += 2) {
            int8_t paramRegHigh = REGHIGH(parseStk[stkIdx].assignment);
            int8_t paramRegLow  = REGLOW(parseStk[stkIdx].assignment);
            for (uint8_t regId = stkIdx == firstSp ? RC : RE; paramRegLow > 0; regId--) {
                if (paramRegLow != regId) {
                    if (state[regId].regSt) {
                        uint8_t regSt   = state[regId].regSt;
                        uint8_t regHigh = REGHIGH(parseStk[regSt].assignment);
                        uint8_t regLow  = REGLOW(parseStk[regSt].assignment);
                        if (regLow == regId)
                            regLow = it;
                        if (regHigh == regId)
                            regHigh = it;
                        // CLEAR PENDING STORE WHEN REG PAIRS ARE TO BE EXCHANGED ***
                        if (state[RA].regSt == regId) {
                            emit(LD, it, RA);
                            state[RA].regSt = 0;
                            state[RA].lock  = false;
                        } else
                            emit(LD, it, regId);
                        state[it].regSt            = regSt;
                        parseStk[regSt].assignment = REGPAIR(regHigh, regLow);
                        it++;
                    }
                    state[paramRegLow].regSt = 0;
                    state[paramRegLow].lock  = false;
                    if (state[RA].regSt == paramRegLow) {
                        paramRegLow     = RA;
                        state[RA].regSt = 0;
                        state[RA].lock  = false;
                    }
                    emit(LD, regId, paramRegLow);
                    state[regId].regSt = stkIdx;
                }
                state[regId].lock = true;
                paramRegLow       = paramRegHigh;
                paramRegHigh      = -1;
            }
        }
        uint8_t ip = RB;
        for (int stkIdx = firstSp; stkIdx < sp; stkIdx += 2) {
            if (parseStk[stkIdx].assignment == 0)
                loadv(stkIdx, 0);
            state[ip].regSt = stkIdx;
            state[ip].lock  = true;
            if (parseStk[stkIdx + 1].prec == 2 && parseStk[stkIdx].prec == 1)
                emit(LD, ip, 0);
            ip = RD;
        }
        if (state[RA].regSt)
            emit(LD, state[RA].regSt, RA);
        for (int reg = RA; reg <= RL; reg++) {
            state[reg].regSt    = 0;
            state[reg].lock     = false;
            state[reg].regValue = -1;
        }
        for (int k = 0; k < 2 * regParamCnt; k++) {
            exch();
            if (parseStk[sp].st == 0 && parseStk[sp].assignment == 0 && parseStk[sp].litv < 0) {
                emit(POP, RH, 0);
                ustack();
                state[RH].regValue = state[RL].regValue = -1;
            }
            pop(1);
        }
    }
}

bool doBuiltin(int val) {
    int reg, j, k, m;
    int port;
    int ip;
    int ia, ib;

    pop(1);
    switch (val) {
    case B_ROL:
    case B_ROR:
    case B_SHL:
    case B_SHR:
    case B_SCL:
    case B_SCR:
    case B_HIGH:
    case B_LOW:
        if (val > B_SCR) /* ** note that this also assumes only 6 such bifs */
            unary(val);
        else {
            int32_t cnt = parseStk[sp].litv;
            if (cnt > 0) { /* generate in-line code for shift counts of */
                /* 1 or 2 for address values */
                /* 1 to 3 for shr of byte values */
                /* 1 to 6 for all other shift functions on byte values */
                if (cnt <= (parseStk[sp - 1].prec != 1 ? 2 : val == B_SHR ? 3 : 6)) {
                    pop(1);
                    for (int j = 0; j < cnt; j++)
                        unary(val);
                    return true;
                }
            }
            exch();
            /* load the value to decrement */
            loadv(sp - 1, 0);
            j = REGLOW(parseStk[sp - 1].assignment);
            if (state[RA].regSt == j) {
                emit(LD, j, RA);
                state[RA].regSt = 0;
            }
            state[j].lock = true;

            /* load the value which is to be operated upon */
            uint8_t prec   = parseStk[sp].prec;
            bool singleReg = prec <= 1;
            if (parseStk[sp].assignment == 0) {
                loadv(sp, singleReg);
                if (singleReg)
                    state[RA].regSt = REGLOW(parseStk[sp].assignment);
            }
            m = REGLOW(parseStk[sp].assignment);
            if (!singleReg || state[RA].regSt != m) {
                if (state[RA].regSt) {
                    emit(LD, state[RA].regSt, RA);
                    state[RA].regSt = 0;
                }
                if (singleReg) {
                    emit(LD, RA, m);
                    state[RA].regSt = m;
                }
            }
            int pc = codloc;
            unary(val);
            if (prec != 1) {
                if (state[RA].regSt)
                    emit(LD, state[RA].regSt, RA);
                state[RA].regSt = 0;
            }
            emit(DC, j, 0);
            emit(JMC, FAL + ZERO, pc);

            /* END up here after operation completed */
            exch();
            state[j].lock = false;
            pop(1);
        }
        return true;
    case B_TIME:
        if (COND(parseStk[sp].assignment))
            cvcond(sp);
        /* emit the following code sequence for 100 usec per loop */
        /* 8080 cpu only */
        /* (get time parameter into the accumulator) */
        /*          mvi   b,12   (7 CY overhead) */
        /* start    mov   c,b    (5 CY * .5 usec = 2.5 usec) */
        /* -------------------- */
        /* tim180   dcr   c      (5 CY * .5 usec = 2.5 usec) */
        /*          jnz   tim180 (10 CY* .5 usec = 5.0 usec) */
        /* -------------------- */
        /*              12 *     (15 CY* .5 usec = 7.5 usec) */
        /*              =        (180 CY* .5 usec = 90 usec) */
        /*          dcr   a      (5 CY * .5 usec = 2.5 usec) */
        /*          jnz   start  (10 CY* .5 usec = 5.0 usec) */

        /*          total time   (200 CY*.5 usec = 100 usec/loop) */
        /*
          If N is the value assigned in MVI B, and T is the value in A and F is the
          CPU clock frequency in MHz, then times are
          8080  (N * 15 + 20) * T / F and overhead is 7 / F
          8085  (N * 14 + 15) * T / F and overhead is 4 / F

          i.e.
          8080 N = (100 * F) / 15
          8085 N = (100 * F - 15) / 14

          8080 2MHz:      use 12 as above
          8080 3.125MHz:  use 19 instead of 12 (97.6uS per loop)
          8080 2.63MhZ:   use 16 instead of 12 (98.86uS per loop)
          8085 3MHz:      use 20 instead of 12 (98.33uS per loop)
          8085 5MHz:      use 35 instead of 12 (101uS per loop)
          8085 6MHz:      use 42 instead of 12 (100.5uS per loop)
        */
        j   = state[RA].regSt;
        ip  = REGHIGH(parseStk[sp].assignment);
        reg = REGLOW(parseStk[sp].assignment);
        if (!j || j != reg) {
            /* get time parameter into the accumulator */
            if (j != 0 && j != ip)
                emit(LD, j, RA);
            state[RA].regSt = 0;
            if (reg == 0)
                loadv(sp, 1);
            reg = REGLOW(parseStk[sp].assignment);
            if (j)
                emit(LD, RA, reg);
        }
        state[RA].regSt = 0;
        emit(LD, reg - 1, -timeScaler);
        emit(LD, reg, reg - 1);
        emit(DC, reg, 0);
        emit(JMC, FAL + ZERO, codloc - 1);
        emit(DC, RA, 0);
        emit(JMC, FAL + ZERO, codloc - 6);
        pop(1);
        return true;
    case B_INPUT:
        /* input function. get input port number */
        port = parseStk[sp].litv;
        if (0 <= port && port <= 255) {
            pop(1);
            sp++;
            uint16_t reg = genReg(1);
            if (reg) {
                k = state[RA].regSt;
                if (k)
                    emit(LD, k, RA);
                state[RA].regSt  = (uint8_t)reg;
                parseStk[sp]     = stkItem(reg, 1, 0, -1);
                state[reg].regSt = sp;
                emit(INP, port, 0);
                return true;
            }
        }
        break;
    case B_OUTPUT:
        /* check for proper output port number */
        port = parseStk[sp].litv;
        if (0 <= port && port <= 255) {
            pop(1);
            /* now build an entry which can be recognized by operat. */
            parseStk[++sp] = stkItem(0, 1, S_OUTPUT, port);
            return true;
        }
        break;
    case B_LENGTH:
    case B_LAST:
        j = parseStk[sp].st;
        if (j > 0) {
            int rval = INFO_ECNT(symAttrib(j)) + B_LENGTH - val;
            pop(1);
            parseStk[++sp] = stkItem(0, rval > 255 ? 2 : 1, 0, rval);
            return true;
        }
        break;
    case B_MOVE: // move is explicitly expanded in pass 1
        break;
    case B_DOUBLE:
        if (parseStk[sp].prec > 1) // already double
            return false;
        if (parseStk[sp].assignment == 0) {
            if (parseStk[sp].litv < 0) {
                loadv(sp, 1); // load value to accumulator and get a register
                state[RA].regSt = REGLOW(parseStk[sp].assignment);
            } else {
                parseStk[sp].prec = 2; // convert to 2 byte literal
                parseStk[sp].st   = 0;
                return true;
            }
        }
        parseStk[sp].prec = 2;
        parseStk[sp].st   = 0;
        if (REGHIGH(parseStk[sp].assignment) == 0) { // only low register
            ia = parseStk[sp].assignment;
            ib = ia - 1;
            if (ib == 0)
                Fatal("133: register allocation stack overflow");
            else {
                state[ib].regSt         = sp;
                state[ia].lock          = false;
                parseStk[sp].assignment = REGPAIR(ib, ia);
                emit(LD, ib, 0); /* ZERO the register */
            }
        }
        return true;
    case B_DEC:
        j = REGLOW(parseStk[sp].assignment);
        if (j != 0)
            if (parseStk[sp].prec == 1) {
                reg = state[RA].regSt;
                if (reg != j) { /* may be a pending register store */
                    if (reg != 0)
                        emit(LD, reg, RA);
                    emit(LD, RA, j);
                    state[RA].regSt = j;
                }
                emit(DAA, 0, 0);
                return true;
            }
        break;
    }

    /* built IN function error */
    error("136: error in built-in function call");
    return false;
}

bool operat(int val) {
    static int it;
    int reg, j, k, m, l, iop = 0, iop2 = 0, jp; // assign to appease GCC
    int ia, ib, il, lp;
    int16_t symId;
    bool skip;
    uint8_t prec;

    /* ADD ADC SUB SBC MUL div mod NEG AND IOR */
    /* XOR NOT EQL LSS GTR NEQ LEQ GEQ INX TRA */
    /* TRC PRO RET STO STD XCH DEL cat LOD BIF */
    /* INC CSE END ENB ENP HAL RTL RTR SFL SFR */
    /* HIV LOV CVA ORG AX1 AX2 AX3 */

    switch (val) {
    case INC:
        parseStk[++sp].litv = 1; /* place a literal 1 at stack top and apply ADD operator */
        /* check for single byte increment, may be comparing with 255 */
        if (parseStk[sp - 1].prec == 1) {
            apply(AD, AC, true, true);
            lastin = codloc;
            return true;
        }
        // fallthrough
    case ADD:
        /* may do the ADD in h and l (using INX operator) */
        if (parseStk[sp].prec != 1)
            exch();
        if (parseStk[sp - 1].prec != 1)
            indexOp(1); // prec=1 for inx
        else {
            exch();
            apply(AD, AC, true, true);
        }
        return true;
    case ADC:
        apply(AC, AC, true, true);
        return true;
    case SUB: /* change address value - 1 to address value + 0xffff and apply ADD */
        if (parseStk[sp - 1].prec != 1 && parseStk[sp].litv == 1) {
            parseStk[sp].litv = 0xffff;
            parseStk[sp].prec = 2;
            indexOp(1);
        } else
            apply(SU, SB, false, true);
        return true;
    case SBC:
        apply(SB, SB, false, true);
        return true;
    case MUL:
        builtin(0, RD);
        return true;
    case DIV:
        builtin(1, RB);
        return true;
    case MDF: // MOD
        builtin(1, RD);
        return true;
    case NEG: // not used
    case BIF:
    case END:
    case ENB:
    case ENP:
    case AX3: // not used
        return false;
    case AND:
        apply(ND, ND, true, false);
        return true;
    case IOR:
        apply(OR, OR, true, false);
        return true;
    case XOR:
        apply(XR, XR, true, false);
        return true;
    case NOT:
        if (COND(parseStk[sp].assignment)) /* condition code - change PARITY */
            parseStk[sp].assignment ^= (TRU << 8);
        else { /* perform XOR with 0xff OR 0xffff (byte or address) */
            uint8_t prec        = parseStk[sp].prec;
            parseStk[++sp].litv = prec == 1 ? 0xff : 0xffff;
            parseStk[sp].prec   = prec;
            apply(XR, XR, true, false);
        }
        return true;
    case EQL: /* equal test */
        if (parseStk[sp].prec + parseStk[sp - 1].prec <= 2) {
            apply(SU, 0, true, false);
            parseStk[sp].assignment += ZFLAG; /* mark as true/ZERO (1*16+2) */
        } else
            compare16(true, ZFLAG, true);
        return true;
    case GTR: /* GTR - change to LSS */
        exch();
        // FALLTHROUGH
    case LSS:                                                 /* LSS set to true/CARRY */
        if (parseStk[sp].prec + parseStk[sp - 1].prec <= 2) { // both bytes
            apply(parseStk[sp].litv != 1 ? SU : CP, 0, false, false);
            parseStk[sp].assignment += CFLAG; /* mark as true/CARRY code */
        } else
            compare16(false, CFLAG, false);
        return true;
    case NEQ:
        if (parseStk[sp].prec + parseStk[sp - 1].prec <= 2) { // both bytes
            apply(SU, 0, true, false);
            parseStk[sp].assignment += NZFLAG; /* mark as false/ZERO */
        } else
            compare16(true, NZFLAG, true);
        return true;
    case LEQ: /* LEQ - change to GEQ */
        exch();
        // fallthrough
    case GEQ:
        if (parseStk[sp].prec + parseStk[sp - 1].prec <= 2) { // both bytes
            apply(parseStk[sp].litv != 1 ? SU : CP, 0, false, false);
            parseStk[sp].assignment += NCFLAG; /* mark as false/CARRY code */
        } else
            compare16(false, NCFLAG, false);
        return true;
    case INX:
        indexOp(parseStk[sp - 1].prec);
        return true;
    case TRC:
        if (parseStk[sp - 1].litv > 0 && (parseStk[sp - 1].litv & 1)) {
            /* this is a do forever (OR something similar) so ignore the jump */
            pop(2);
            return true;
        }
        iop = 2; /* NOT a literal '1' */
        /* check for condition code */
        if (COND(parseStk[sp - 1].assignment)) { /* active condition code, construct mask for JMC */
            iop2 = COND(parseStk[sp - 1].assignment) ^ TRU;
        } else {
            if (parseStk[sp - 1].assignment == 0) { /* load value to accumulator */
                parseStk[sp - 1].prec = 1;
                loadv(sp - 1, 1);
            } else if (REGLOW(parseStk[sp - 1].assignment) !=
                       state[RA].regSt) { /* value already loaded */
                if (state[RA].regSt)
                    emit(LD, state[RA].regSt, RA);
                emit(LD, RA, REGLOW(parseStk[sp - 1].assignment));
            }
            state[RA].regSt = 0;
            emit(ROT, CY, RGT);
            iop2 = FAL + CARRY;
        }
        break;
    case PRO:
        symId = parseStk[sp].st;
        if (symId >= intbas)
            doPRO(symId);
        else                                 /* this is a built-IN function. */
            return doBuiltin(symId - S_ROL); // pass function number
        iop = 3;
        break;
    case RET:
        if (prsp <= 0) {
            error("146: procedure optimization stack underflow");
            state[RH].regValue = state[RL].regValue = -255; /* mark as nil */
            return true;
        }
        // V4 /* check for type AND precision of procedure */
        l    = LOWWORD(prstk[prsp]) + 2;
        prec = INFO_PREC(symbol[l]);

        /* prec is the precision of the procedure */
        if (prec != 0) {
            uint16_t assignment = parseStk[sp].assignment;
            if (assignment == 0)
                loadv(sp, 1);
            else if (assignment >= 256)
                cvcond(sp);
            uint8_t regSt   = state[RA].regSt;
            uint8_t regLow  = REGLOW(parseStk[sp].assignment);
            uint8_t regHigh = REGHIGH(parseStk[sp].assignment);
            if (assignment &&
                regLow != regSt) { /* have to load the accumulator.  may have h.o. byte. */
                if (regSt && regSt == regHigh)
                    emit(LD, regHigh, RA);
                emit(LD, RA, regLow);
            }
            if (regHigh && regHigh != RB)
                emit(LD, RB, regHigh);
            else if (prec > parseStk[sp].prec) /* compare precision of procedure with stack */
                emit(LD, RB, 0);
        }
        pop(1);
        if (prstk[prsp] > 65535) { /* interrupt procedure - use the DRT code below */
            emit(POP, RA, 0);
            emit(POP, RB, 0);
            emit(POP, RD, 0);
            emit(POP, RH, 0);
            emit(EI, 0, 0);
            emit(RTN, 0, 0);
            if (prsp <= 0) {
                error("146: procedure optimization stack underflow");
                state[RH].regValue = state[RL].regValue = -255; /* mark as nil */
                return true;
            }
        } else
            emit(RTN, 0, 0);
        /* merge values of h AND l for this procedure */
        updateHL(prsp);
        return true;
    case STO:
    case STD:
        /* STO and STD */
        symId = parseStk[sp].st;

        /* check for output function */
        if (symId != S_OUTPUT) { /* check for computed address OR saved address */
            if (symId < 0) {     /* check for address reference outside intrinsic range */
                symId = -symId;
                if (S_STACKPTR < symId &&
                    symId <= intbas) { /* check for 'memory' address reference */
                    if (val == STD)
                        pop(1);
                    return true;
                }
            }
            genStore(val != STD);
        } else {
            uint32_t port       = parseStk[sp].litv;
            uint16_t assignment = parseStk[sp - 1].assignment;
            if (assignment <= 0 || assignment >= 256) { /* load value to ACC */
                assignment = state[RA].regSt;
                if (assignment > 0)
                    emit(LD, assignment, RA);
                loadv(sp - 1, 1);
                assignment = parseStk[sp - 1].assignment;
            } else { /* operand is IN the gprs */
                assignment = LOWNIBBLE(assignment);
                if ((k = state[RA].regSt) != assignment) {
                    if (k > 0)
                        emit(LD, k, RA);
                    else
                        emit(LD, RA, assignment);
                }
            }
            /* now mark ACC active IN case subsequent STO operator */
            state[RA].regSt = LOWNIBBLE(assignment);
            emit(OUT, port, 0);
            pop(1);
        }
        /* * check for STD * */
        if (val == STD)
            pop(1);
        return true;
    case XCH:
        exch();
        return true;
    case DEL:
        if (parseStk[sp].st == 0 && parseStk[sp].assignment == 0 && parseStk[sp].litv < 0) {
            /* value is stacked, so get rid of it */
            emit(POP, RH, 0);
            state[RH].regValue = state[RL].regValue = -1;
            ustack();
        }
        pop(1);
        return true;
    case DAT:
        inldat();
        return false;
    case LOD:
        skip = false;
        k    = parseStk[sp].prec >> 2; // precision of the based variable if applicable

        /* mask off any based variable precision */
        parseStk[sp].prec &= 3;
        ia = parseStk[sp].assignment;
        if (ia <= 0) {
            /* check for simple based variable case */
            symId = parseStk[sp].st;
            if (symId > 0) { /* reserve registers for the result */
                genreg(2, &ia, &ib);
                state[ia].regSt         = sp;
                state[ib].regSt         = sp;
                parseStk[sp].assignment = REGPAIR(ib, ia);
                /* may be able to simplify LHLD */
                lp = state[RH].regValue;
                l  = state[RL].regValue;
                if (lp != -3 || -l != symId) {
                    if (lp == -4 && -l == symId) {
                        emit(DCX, RH, 0);
                        state[RH].regValue = -3;
                    } else {
                        j = chain(symId, codloc + 1);
                        emit(LHLD, j, 0);
                        state[RH].regValue = -3;
                        state[RL].regValue = -symId;
                    }
                }
                skip = true;
            } else if (parseStk[sp].st == 0) { /* first check for an address reference */
                loadv(sp, 0);
                ia = parseStk[sp].assignment;
            } else { /* change the address reference to a value reference */
                parseStk[sp].st   = -parseStk[sp].st;
                parseStk[sp].litv = -1;
                return true;
            }
        }
        il = 0;
        if (!skip) {
            ib  = REGHIGH(ia);
            ia  = REGLOW(ia);
            reg = state[RA].regSt;
            if (ia == reg)
                ia = RA;
            if (ib == reg)
                ib = RA;
            if (ib == ia - 1)
                il = ib;
            if (ia * ib == 0) {
                Fatal("138: register allocation error");
                return true;
            }
            /* may be possible to use LDAX OR XCHG */
            if (il != RD) {
                emit(LD, RL, ia);
                emit(LD, RH, ib);
                il                 = 0;
                state[RH].regValue = state[RL].regValue = -1;
            } else if (lastex == codloc - 1 || parseStk[sp].prec == 2) {
                /* double XCHG or double byte load with addr in d and e */
                emit(XCHG, 0, 0);
                il                 = 0;
                state[RH].regValue = state[RL].regValue = -1;
            }
        }
        // if based convert to precision of the based variable
        if (k)
            parseStk[sp].prec = k;

        /* recover the register assignment from rasn */
        ia = REGLOW(parseStk[sp].assignment);
        ib = REGHIGH(parseStk[sp].assignment);
        j  = state[RA].regSt;

        /* skip if j=0, ia, OR ib */
        if (j != 0 && j != ia && j != ib)
            emit(LD, j, RA);

        /* may be able to change register assignment to bc */
        if (ia == RE)
            if (state[RB].regSt == 0 && state[RC].regSt == 0) { /* bc available, so RE-assign */
                state[ia].regSt = state[ib].regSt = 0;
                state[RB].regSt = state[RC].regSt = sp;
                ia                                = RC;
                ib                                = RB;
                parseStk[sp].assignment           = REGPAIR(RB, RC);
            }
        state[RA].regSt = ia;
        if (il)
            emit(LDAX, il, 0);
        else
            emit(LD, RA, ME);

        if (parseStk[sp].prec > 1) {
            emit(INCX, RH, 0);
            /* may have done a prevous LHLD, if so mark INCX h */
            if (state[RH].regValue == -3)
                state[RH].regValue = -4;
            emit(LD, ib, ME);
        } else { /* single byte load - release h.o. register */
            ib = REGHIGH(parseStk[sp].assignment);
            if (ib == state[RA].regSt)
                state[RA].regSt = 0;
            state[ib].regSt         = 0;
            state[ib].regValue      = -1;
            parseStk[sp].assignment = REGLOW(parseStk[sp].assignment);
        }
        parseStk[sp].st = state[RH].regSt = state[RL].regSt = 0;
        return true;
    case CSE:

        /* let x be the value of the stack top */
        /* compute 2*x + codloc, fetch to hl, AND jump with PCHL */
        /* reserve registers for the jump table base */
        genreg(2, &ia, &ib);
        state[ia].lock = true;
        state[ib].lock = true;

        /* index is in h AND l, so double it */
        emit(DAD, RH, 0);

        /* now load the value of table base, depending upon 9 bytes */
        /* LXI r,$+9 ! DAD r ! MOV E,M ! INX H ! MOV D,M ! XCHG ! PCHL */
        emit(LXI, ib, codloc + 9);
        emit(DAD, ib, 0);
        emit(LD, RE, ME);
        emit(INCX, RH, 0);
        emit(LD, RD, ME);
        emit(XCHG, 0, 0);
        emit(PCHL, 0, 0);

        /* phony entry in symbol table to keep code dump clean */
        symbol[++sytop]  = syinfo;
        symbol[syinfo--] = -codloc;

        /* set entry to len=0/prec=2/type=VARB/ */
        symbol[syinfo] = PACK_ATTRIB(0, 2, VARB);
        casjmp         = syinfo;

        /* casjmp will be used to update the length field */
        if (--syinfo <= sytop)
            Fatal("108: pass-2 symbol table overflow");
        state[ib].lock     = false;
        state[RH].regValue = state[RL].regValue = -255; /* mark as nil */
        return true;
    case HAL:
        emit(EI, 0, 0);
        emit(HALT, 0, 0);
        return true;
    case RTL: // these IR codes are not used
    case RTR:
    case SFL:
    case SFR:
    case HIV:
    case LOV:
        unary(val - RTL);
        return true;
    case CVA:
        /* CVA must be immediately preceded by an INX OR ADR ref */
        parseStk[sp].prec = 2;

        /* if the address is already IN the gpr's then nothing to do */
        if (parseStk[sp].assignment > 0)
            return true;
        if (parseStk[sp].st < 0) { /* check for address ref to data IN rom. */
            jp = parseStk[sp].litv;
            if (jp <= 0xffff) {
                if (jp < 0)
                    error("149: pass-2 compiler error. Attempt to convert invalid value to "
                          "address type");
                /* leave literal value */
                parseStk[sp].st = 0;
                return true;
            } /* do LXI r with the address */
            genreg(2, &ia, &ib);
            if (ia <= 0) {
                Fatal("140: register allocation error");
                return false;
            } else {
                emit(LXI, ib, chain(-parseStk[sp].st, codloc + 1));
                parseStk[sp].st         = 0;
                parseStk[sp].assignment = REGPAIR(ib, ia);
                state[ia].regSt         = sp;
                state[ib].regSt         = sp;
                return true;
            }
        } else if (parseStk[sp].st > 0) {
            /* load value of base for address ref to a based variable */
            loadv(sp, 3);
            return true;
        } else {
            error("139: error in changing variable to address reference");
            return false;
        }
        break; // not needed no path to here
    case ORG:
        reg = parseStk[sp].litv;
        // V4
        if (reg < 0)
            error("154: bad code origin from pass-1");
        else {
            if (codloc > reg)
                error("141: invalid origin");
            j = C_STACKHANDLING;
            k = j == 1 ? 0 : 3; // asssume lxi sp, if not user allocated
            if (C_LOAD < 0 && codloc == codeBase + k) {
                /* no real code generated to set new codeBase */
                codeBase = codloc = reg;
                if (j != 1) {
                    if (j == 0)
                        lxis = codloc + 1; /* linkage chain for stack */
                    emit(LXI, RSP, j);     /* reserve space for stack */
                }
            } else {
                codloc = reg; // new code point. Don't need to explicitly zero

                if (j != 1) {     // not user handled
                    if (j == 0) { // chain system allocated stack
                        j    = lxis;
                        lxis = codloc + 1;
                    }
                    emit(LXI, RSP, j);
                }
            }
        }

        sp--;
        return true;
    case DRT:
        if (prstk[prsp] > 0xffff) /* this is the end of an interrupt procedure */
            curdep[prsp] -= 4;

        /* get stack depth for symbol table */
        if (prsp > 0) {
            if (curdep[prsp] != 0)
                error("150: stack not empty at end of compilation");
            l = (prstk[prsp] & 0xffff) - 1;

            /*l is symbol table count entry - set max stack depth*/
            symbol[l] = maxdep[prsp];
        }
        if ((jp = prsp) > 0)
            prsp--;

        k = state[RH].regValue;
        l = state[RL].regValue;
        if (k == -255 && l == -255)
            return false;
        if (prstk[jp] > 0xffff) { /* POP interrupted registers and enable interrupts */
            emit(POP, RA, 0);
            emit(POP, RB, 0);
            emit(POP, RD, 0);
            emit(POP, RH, 0);
            emit(EI, 0, 0);
        }
        emit(RTN, 0, 0);
        if (k != -254 || l != -254) {
            if (jp > 0) {
                updateHL(jp);
                return true;
            } else
                error("146: procedure optimization stack underflow");
        }
        state[RH].regValue = state[RL].regValue = -255; /* mark as nil */
        return true;
    case ENA:
        emit(EI, 0, 0);
        return false;
    case DIS:
        emit(DI, 0, 0);
        return false;
    case AX1:
        /* load case number to h AND l */
        exch();
        loadv(sp, 4);
        pop(1);
        state[RH].regValue = -1;
        state[RL].regValue = -1;
        // fallthrough

    case TRA: /* TRA -   check stack for simple LABEL variable */
        iop            = 1;
        state[RH].lock = state[RL].lock = true; /* in case there are any pending values ... */
        saver();
        state[RH].lock = state[RL].lock = false;
        m                               = parseStk[sp].litv;
        if (m >= 0) { /* absolute jump - probably to assembly language subrtne... */

            state[RH].regValue = state[RL].regValue = -1; /* ...so make h AND l registers unknown */
            emit(JMP, m, 0);
            pop(1);
            return true;
        }
        break;
    case AX2: /* may not be omitted even though no obvious path exists). */
        iop = 4;
        /* casjmp points to symbol table attributes - INC len field */
        symbol[casjmp] += (1 << 8);
        break;
    }

    symId = parseStk[sp].st;
    if (symId > 0) {
        j = INFO_TYPE(symAttrib(symId));
        /* may be a simple variable */
        if (iop != 1 || j != VARB) {
            if ((iop != 3 || j != PROC) && j != LABEL) {
                error("135: invalid program transfer (only computed jumps are allowed with a "
                      "'go to')");
                sp--;
                return true;
            } else {
                j = -symAddr(symId);
                m = symRef(symId);

                if (iop == 1) {
                    it = INFO_PREC(abs(symAttrib(symId)));
                    /* it is type of LABEL... */
                    if (it == CompilerLabel && defsym > 0 &&
                        INFO_PREC(symAttrib(defsym)) == CompilerLabel) {
                        /* this TRA is one of a chain of compiler generated */
                        /* TRA's - straighten the chain if no code has been */
                        /* generated since the previous DEF. */

                        l  = -symAddr(defsym);
                        jp = symRef(defsym);
                        if (jp == codloc) {
                            /* adjust the reference counts and optimization */
                            /* information for both DEF's. */
                            uint16_t refCnt = INFO_ECNT(abs(symAttrib(defsym)));
                            int trackHL     = refCnt == 1 ? symTrackHL(defsym) : 0;

                            if (defrh == -255)
                                refCnt--;
                            symAttrib(defsym) = (CompilerLabel << 4) + LABEL;

                            /* i.e., zero references to compiler generated LABEL */
                            if (INFO_ECNT(abs(symAttrib(symId))) == 1)
                                symTrackHL(symId) = trackHL;

                            symAttrib(symId) += refCnt << 8;
                            /* corrected reference count for object of the DEF */
                            /* merge the backstuffing chains */
                            uint16_t memAddr;
                            while ((memAddr = l >> 2)) {
                                l               = (getword(memAddr) << 2) + (l & 3);
                                symAddr(defsym) = -l;
                                putword(memAddr, j >> 2);
                                j              = ((memAddr << 2) + (j & 3));
                                symAddr(symId) = -j;
                            }

                            /* equate the defs */
                            for (uint16_t i = 1; i <= sytop; i++)
                                if (symbol[i] == symbol[defsym])
                                    symbol[i] = symbol[symId];
                            /* omit the TRA if no path to it */
                            state[RH].regValue = defrh;
                            state[RL].regValue = defrl;
                        }
                        if (state[RH].regValue == -255) {
                            pop(1);
                            return true;
                        }
                    }
                }

                if (it == OuterLabel && iop == 1) { /* we have a TRA to the outer block... */
                    uint16_t stk = C_STACKHANDLING;
                    if (prsp != 0 && stk != 1) {
                        if (stk == 0) {
                            stk  = lxis;
                            lxis = codloc + 1;
                        }
                        emit(LXI, RSP, stk);
                    }
                }
                j = -symAddr(symId);
                m = j / 4;

                /* connect entry into chain */
                k = codloc + 1;
                if (iop == 4) /* iop = 4 if we arrived here from case table JMP */
                    k = codloc;

                symAddr(symId) = -((k << 2) + right(j, 2));

                /* check for single reference */
                if (INFO_ECNT(abs(symAttrib(symId))) == 1) {
                    //        note in a do case block, an implicit goto is
                    //        generated at the end of each selective statement
                    //        in order to branch out to the next statement
                    //        at the end of block. all these gotos are
                    //        considered a single forward reference for the
                    //        the convienance of the h/l optimization
                    //        ************************************************
                    //        make sure if saved h/l is already marked no good
                    //           1b   |   1b    |   9b    |   8b
                    //        h valid | l valid | h value | l value
                    //

                    int lsym = symTrackHL(symId);

                    /* PMO simplified the code to propagate HL info also fixed a bug in the
                       original Fortran code which could incorrectly subtract the valid flag
                       value from ktotal even if it hadn't been set.
                    */
                    if (lsym != -1) {
                        int ktotal = 0;
                        if (0 <= state[RL].regValue && state[RL].regValue < 256) {
                            ktotal = state[RL].regValue;
                            if (lsym == 0 ||
                                ((lsym & LVALID) && state[RL].regValue == (lsym & 0xff)))
                                ktotal |= LVALID;
                        }
                        if (0 <= state[RH].regValue && state[RH].regValue < 512) {
                            ktotal += state[RH].regValue << 8;
                            if (lsym == 0 ||
                                ((lsym & HVALID) && state[RH].regValue == ((lsym >> 8) & 0x1ff)))
                                ktotal |= HVALID;
                        }
                        symTrackHL(symId) = ktotal & (HVALID | LVALID) ? ktotal : -1;
                    }
                }

                /* TRA, TRC, PRO, AX2 (case TRA) */
                switch (iop) {
                case 1: /* may be INC TRA combination in do-loop */
                    if (lastin + 1 == codloc)
                        emit(JMC, FAL + ZERO, m); /* change to jfz to top of loop */
                    else {
                        xfrloc = codloc;
                        xfrsym = parseStk[sp].st;
                        tstloc = codloc + 3;
                        emit(JMP, m, 0);
                        state[RH].regValue = state[RL].regValue = -255; /* mark as nil */
                    }
                    pop(1);
                    return true;
                case 2:
                    conloc = codloc;
                    emit(JMC, iop2, m);
                    pop(2);
                    return true;
                    ;
                case 3:
                    xfrloc = codloc;
                    xfrsym = parseStk[sp].st;
                    tstloc = codloc + 3;
                    emit(CAL, m, 0);

                    /* adjust the maxdepth, if necessary */
                    // V4
                    j = symIProcDepth(symId) + 1;
                    /* j is number of double-byte stack elements reqd */
                    stack(j);

                    /* now returned from call so... */
                    curdep[prsp] -= j;

                    /* now fix the h AND l values upon return */
                    // V4
                    j = symTrackHL(symId);
                    if (j < 0)
                        state[RH].regValue = state[RL].regValue = -1;
                    else if ((j & (LNOTSET | HNOTSET)) != (LNOTSET | HNOTSET)) {
                        /* compare values */
                        state[RL].regValue = (j & LVALID) ? j & 0xff : -1;
                        state[RH].regValue = (j & HVALID) ? (j >> 8) & 0x1ff : -1;
                    }
                    pop(1);

                    /* may have to construct a returned value at the stack top */
                    j = INFO_PREC(symAttrib(symId));
                    if (j > 0) { /* set stack top to precision of procedure */
                        parseStk[++sp]  = stkItem(j > 1 ? REGPAIR(RB, RC) : RC, j, 0, -1);
                        state[RA].regSt = RC;
                        state[RC].regSt = sp;
                        if (j > 1)
                            state[RB].regSt = sp;
                    }
                    return true;
                case 4:
                    /* came from a case vector */
                    emit(0, m % 256, 0);
                    emit(0, m / 256, 0);
                    pop(1);
                    return true;
                }
            }
        }
    } else if (iop != 1 || symId != 0) { /* could be a computed address */
        error("134: invalid program transfer (only computed jumps are allowed with a 'go to')");
        sp--;
        return true;
    }

    /* jump to computed location */
    loadv(sp, 4);
    pop(1);
    emit(PCHL, 0, 0);

    /* pc has been moved, so mark h AND l unknown */
    // V4
    state[RH].regValue = state[RL].regValue = -1;
    return true;
}

/*
    symbol[i] format
    0-7     RL val
    8-16    RH val  (note additional bit)
    17      RL valid
    18      RH valid
    19+     tbd if 3 then merge values of h and l
*/
void updateHL(int jp) {
    xfrloc   = codloc - 1;
    xfrsym   = 0;
    tstloc   = codloc;
    int reg  = prstk[jp] & 0xffff;
    int dsym = state[RH].regValue;
    int l    = state[RL].regValue;
    int j    = symbol[reg]; // masking not needed as implicit in code below
    int lp, kp;

    alter = true; // hoisted here to simplify return
    if (j < 0)
        j = 0;
    if ((j & (LNOTSET | HNOTSET)) != (LNOTSET | HNOTSET)) {
        /* otherwise merge values of h AND l */
        lp = (j & LVALID) ? j & 0xff : -1;
        kp = (j & HVALID) ? (j >> 8) & 0x1ff : -1;
    } else if (dsym == -254 && l == -254)
        return;
    else { /* h AND l have been altered IN the procedure */
        kp = dsym;
        lp = l;
    }

    /* compare k with kp AND l with lp */
    j = (l >= 0 && lp == l) ? LVALID + l : 0;
    if (dsym >= 0 && kp == dsym)
        j += HVALID + (dsym << 8);
    symbol[reg]        = j;
    state[RH].regValue = state[RL].regValue = -255;
}

// update condition code for 16 bit
void compare16(bool icom, int flag, bool zeroTest) {
    apply(SU, SB, icom, true);
    int regLow  = REGLOW(parseStk[sp].assignment);
    int regHigh = REGHIGH(parseStk[sp].assignment);
    if (zeroTest)
        emit(OR, regLow, 0);

    /* get rid of high order register in the result */
    state[RA].regSt = regLow;
    parseStk[sp]    = stkItem(flag + regLow, 1, 0, -1);

    if (regHigh) { // free high register if previously used
        state[regHigh].lock     = false;
        state[regHigh].regSt    = 0;
        state[regHigh].regValue = -1;
    }
    return;
}

// genrate call to builtin function bf, result determines result reg pair to use
// modified to use bf = 0->mutliply 1->divide/mod
void builtin(int bf, int targetReg) {
    /* clear condition code */
    if (COND(parseStk[sp].assignment))
        cvcond(sp);

    /* clear pending store */
    if (state[RA].regSt != 0) {
        emit(LD, state[RA].regSt, RA);
        state[RA].regSt = 0;
    }

    /* lock any correctly assigned registers */
    /* ....AND store the remaining registers. */
    if (REGLOW(parseStk[sp].assignment) == RE)
        state[RE].lock = true;
    if (REGHIGH(parseStk[sp].assignment) == RD)
        state[RD].lock = true;
    if (REGLOW(parseStk[sp - 1].assignment) == RC)
        state[RC].lock = true;
    if (REGHIGH(parseStk[sp - 1].assignment) == RB)
        state[RB].lock = true;
    saver();

    /* mark register c used. */
    if (state[RC].regSt == 0)
        state[RC].regSt = 255;

    /* load top of stack into registers d and e. */
    loadv(sp, 0);
    if (parseStk[sp].prec == 1)
        emit(LD, RD, 0);

    /* now deassign register c unless correctly loaded. */
    if (state[RC].regSt == 255)
        state[RC].regSt = 0;

    /* load t.o.s. - 1 into registers b AND c. */
    loadv(sp - 1, 0);
    if (parseStk[sp - 1].prec == 1)
        emit(LD, RB, 0);
    pop(2);

    /* call the built-IN function */
    emitbf(bf);

    /* requires 2 levels in stack for BIF (call AND temp.) */
    stack(2);
    ustack();
    ustack();

    /* AND then retrieve results */
    for (int dsym = 1; dsym <= 7; dsym++)
        state[dsym].lock = false;

    /* cannot predict where registers h AND l will END up */
    state[RH].regValue = state[RL].regValue = -1;
    // save the result in the target register pair
    parseStk[++sp]         = stkItem(REGPAIR(targetReg, targetReg + 1), 2, 0, -1);
    state[targetReg].regSt = state[targetReg + 1].regSt = sp;
}

void indexOp(int prec) {

    if (parseStk[sp].litv == 0) /* zero index just pop it and ignore the INX operator */
        pop(1);
    else {
        if (COND(parseStk[sp].assignment))
            cvcond(sp);
        int j         = state[RA].regSt;
        int indexLow  = REGLOW(parseStk[sp].assignment);
        int indexHigh = REGHIGH(parseStk[sp].assignment);
        int baseLow   = REGLOW(parseStk[sp - 1].assignment);
        int baseHigh  = REGHIGH(parseStk[sp - 1].assignment);

        /* check for pending store to base or index */
        if (j && (j == baseHigh || j == baseLow || j == indexHigh || j == indexLow)) {
            emit(LD, j, RA);
            state[RA].regSt = 0;
        }

        /* make sure that d and e are available */
        if (state[RE].regSt != 0 || state[RD].regSt != 0)
            if (indexLow != RE && baseLow != RE) { /* mark all registers free */
                int ia, ic;
                if (indexLow)
                    state[indexLow].regSt = 0;
                if (baseLow)
                    state[baseLow].regSt = 0;
                genreg(2, &ia, NULL);
                state[ia].regSt = 1; // mark as not free so as to not reallocate
                genreg(2, &ic, NULL);
                state[ia].regSt = 0; // back to free

                /* all regs are cleared except base and index, if allocated. */
                if (indexLow)
                    state[indexLow].regSt = sp;
                if (baseLow)
                    state[baseLow].regSt = sp - 1;
            }

        /* if not literal 1 or -1, use DAD else use INX or DCX */
        int op;
        if (parseStk[sp].litv != 1 && parseStk[sp].litv != 0xffff) {
            /* if the index is constant, and the base an address variable, */
            /* double the literal value at compile time */
            if (parseStk[sp].litv >= 0 && prec != 1) {
                parseStk[sp].litv *= 2;
                prec = 1; // pretend byte variable (saves a DAD)
            }
            loadv(sp, parseStk[sp].litv >= 0 ? 3 : 0);

            //  if the index was already in the registers, may have to extend precision to address.
            indexLow  = REGLOW(parseStk[sp].assignment);
            indexHigh = REGHIGH(parseStk[sp].assignment);
            if (indexLow != 0 && indexHigh == 0) {
                indexHigh = indexLow - 1;
                emit(LD, indexHigh, 0);
            }
            op = DAD;
        } else {
            op        = parseStk[sp].litv == 1 ? INCX : DCX;
            indexHigh = RH;
        }

        pop(1);                 /* pop the index.  base is now at top */
        loadv(sp, 5);           /* load the base into the h and l registers */
        emit(op, indexHigh, 0); /* add the base and index */

        if (prec != 1)
            emit(op, indexHigh, 0); /* and add index again if base is an address variable. */
        emit(XCHG, 0, 0);           /* XCHG here and remove with peephole optimization later */
        prec = parseStk[sp].prec;   // get back precision of base
        pop(1);
        parseStk[++sp]     = stkItem(REGPAIR(RD, RE), prec, 0, -1);
        state[RH].regValue = state[RL].regValue = -1;
        state[RD].regSt = state[RE].regSt = sp;
    }
}
