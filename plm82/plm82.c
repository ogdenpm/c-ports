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
#ifdef V4
//    153    Illegal Control Record: First nonblank characterr was
//           other than a dollar sign

//    154    Bad code origin from pass-1 (e.g. Pass-1 error 48)
#endif

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
    "-l nn      start machine code generation at location nn\n"
    "-m         turn off symbol map\n"
    "-n         write emitter trace\n"
    "-O         enable the additional V4 load and arith immediate optimisations\n"
    "-s n       debug - write symbol info. 0 none, != 0 address , >= 2 detailed info\n"
    "-i nn      stack initialisation. 0 system determined, 1 user specified, > 1 stack location\n"
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
int outloc = 17;
int firsti = 7;
int casjmp = 0;

/* rgmapp */
//                             A  B  C  D  E  H  L     M
int const regmap[9 + 1] = { 0, 7, 0, 1, 2, 3, 4, 5, 6, 6 };

/* ... plm2 vers ...*/
int vers = 40;

/* symbl */
int symbol[MAXSYM + 1];
int symax  = MAXSYM;
int sytop  = 0;
int syinfo = MAXSYM;
int lmem;

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

/* pstack is the procedure stack used IN hl optimization*/
int prstk[15 + 1];
int maxdep[16];
int curdep[16];
int prsmax = 15;
int prsp   = 0;
int lxis   = 0;

// clang-format on
/* memory */
uint8_t mem[MAXMEM]; // upto max memory of 8080
int codeBase = 0;
int entry    = -1; // entry point for hex trailer
int preamb;

/* regall */
typedef struct {
    int regSP;
    int regValue;
    bool lock;
} reg_t;
reg_t state[1 + RL] = { { 0, -1, false }, { 0, -1, false }, { 0, -1, false }, { 0, -1, false },
                        { 0, -1, false }, { 0, -1, false }, { 0, -1, false }, { 0, -1, false } };

regAlloc_t regAlloc[16 + 1];

int sp    = 0;
int maxsp = 16;

/* intbas is the largest intrinsic symbol number*/
int intbas = 23;

int errorCnt;
int C_ANALYSIS;
int C_COUNT;           // line counter
bool C_FINISH  = true; // dump code at finish
int C_GENERATE = 2;
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

regAlloc_t newReg(uint16_t assignment, uint8_t prec, int16_t st, int32_t litv) {
    regAlloc_t r = { .assignment = assignment, .prec = prec, .st = st, .litv = litv };
    return r;
}

int parseNum(char const *s) {
    int val       = 0;
    char const *t = s;
    if (!isxdigit(*s))
        usage("Invalid number %s\n", s);

    while (isxdigit(*t))
        t++;
    uint8_t radix = toupper(*t) == 'H' ? 16 : 10;
    if (radix == 16)
        t++;
    if (*t)
        usage("Invalid number %s\n", s);

    for (t = s; isxdigit(*t); t++) {
        uint8_t digit = isdigit(*t) ? *t - '0' : toupper(*t) - 'A' + 10;
        if (digit >= radix)
            usage("Invalid digit %c in decimal number %s\n", *t, s);
        val = val * radix + digit;
    }
    return val;
}

int main(int argc, char **argv) {

    while (getOpt(argc, argv, "a:e:fg:i:l:mn:Os:v:w:x") != EOF) {
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
            if ((C_STACKHANDLING = parseNum(optArg)) >= 0x10000)
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
    fprintf(lstFp, "         pl/m-8080 pass2 Version 4.0 - %s\n", ctime(&now));

    /* change margins for reading intermediate language */

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
                int np     = intbas + 1;
                for (int n = np; n <= sytop; n++) {
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
        Fatal("101: pass-2 address outside available storage");
        return 0;
    }
    return mem[ip];
}

uint16_t getword(int ip) {
    return get(ip) + get(ip + 1) * 256;
}

void put(uint32_t ip, uint8_t val) {
    if (ip >= MAXMEM)
        Fatal("102: pass-2 address outside available storage");
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
        int reg;
        if ((reg = REGHIGH(regAlloc[sp].assignment))) {
            if (state[RA].regSP == reg)
                state[RA].regSP = 0;
            state[reg].lock  = false;
            state[reg].regSP = 0;
        }
        if ((reg = REGLOW(regAlloc[sp].assignment))) {
            if (state[RA].regSP == reg)
                state[RA].regSP = 0;
            state[reg].lock  = false;
            state[reg].regSP = 0;
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
    for (int ip = 0, j = sp - 1; j <= sp; j++) {
        if (regAlloc[j].st == 0 && regAlloc[j].assignment == 0 &&
            regAlloc[j].litv < 0) { /* operand is stacked */
            genreg(-2, &ia, &ib);
            state[ia].regSP = j;
            if (ip != 0)
                error("152: Invalid stack order in 'apply'");
            ip = ib;
            if (regAlloc[j].prec > 1) /* double byte operand */
                state[ib].regSP = j;
            else /* single precision result */
                ib = 0;
            regAlloc[j].assignment = REGPAIR(ib, ia);
            emit(POP, ip, 0);
            ustack();
        }
    }
    /* make a quick check for possible accumulator match */
    /* with the second operand */
    if (COND((ia = regAlloc[sp].assignment)))
        cvcond(sp);
    if (COND((ib = regAlloc[sp - 1].assignment)))
        cvcond(sp - 1);
    int regA = state[RA].regSP;
    if (ia && ib && regA && com &&
        regA == ia % 16) /* commutative operator, one may be in the accumulator */
        exch();          /* second operand in gpr's, regA.o. byte in accumulator */

    bool storeSetup = false; // replaces use of goto L110

    for (;; exch()) {
        if (!regAlloc[sp - 1].assignment) { /* is op1 IN gpr's */
            if (regAlloc[sp].assignment) {  /* is op2 IN gpr's */
                                            /* yes - can we exchange and try again */
                regAlloc[sp].litv = -1;     /* after insuring that a literal has no regs assigned */
                if (com)
                    continue;
            }
        } else if ((ia = REGLOW(regAlloc[sp - 1].assignment)) ==
                   0) { /* reg assigned, lock regs containing var */
            Fatal("107: register allocation error. No registers available");
            return;
        } else {
            state[ia].lock = true;
            if ((ib = REGHIGH(regAlloc[sp - 1].assignment)))
                state[ib].lock = true;

            /* may have to generate one free reg */
            if (regAlloc[sp - 1].prec < regAlloc[sp].prec)
                ib = ia - 1;

            /* check for pending register store */
            if ((jp = state[RA].regSP) != ia) {
                if (jp)
                    emit(LD, jp, RA);
                state[RA].regSP = ia;
                emit(LD, RA, ia);
            }
        }

        /* op2 NOT IN gpr's OR op is NOT commutative */
        /* check for literal value - is op2 literal */
        if ((k = regAlloc[sp].litv) >= 0) {
            if (regAlloc[sp].prec <= 1 && regAlloc[sp - 1].prec <= 1 &&
                k == 1 /* make special check for possible increment OR decrement */
                && (op == AD || op == SU)        /* must be ADD OR subtract without CARRY */
                && regAlloc[sp - 1].prec == 1) { /* first operand must be single byte variable */
                if (ia <= 1) {                   /* op1 must be IN memory, so load into gpr */
                    loadv(sp - 1, 0);
                    if ((ia = regAlloc[sp - 1].assignment & 0xf) == 0) {
                        Fatal("107: register allocation error. No registers available");
                        return;
                    }
                    lastIncReg = codloc; /* ...may change to inr memory if STD to op1 follows... */
                }
                emit(op == AD ? IN : DC, state[RA].regSP == ia ? RA : ia, 0);
                storeSetup = true;
            }
            break;
        }

        /* op1 NOT a literal,  check for literal op2 */
        if (regAlloc[sp - 1].litv < 0 || !com)
            break;
    }

    if (!storeSetup) {
        /* generate registers to hold results IN loadv */
        /* (loadv will load the low order byte into the ACC) */
        loadv(sp - 1, 1);
        if ((ia = REGLOW(regAlloc[sp - 1].assignment)) == 0) {
            Fatal("107: register allocation error. No registers available");
            return;
        }
        state[ia].lock = true;
        ib             = regAlloc[sp - 1].assignment / 16;

        /* is this a single byte / double byte operation */
        if (ib <= 0 && regAlloc[sp].prec != 1) {
            /* get a spare register */
            if ((ib = ia - 1) == 0) { // ia was A reg so can't double byte
                Fatal("107: register allocation error. No registers available");
                return;
            }
            state[ib].lock = true;
        } else
            ib &= 7;

        /* now ready to perform operation */
        /* l.o. byte is IN AC, h.o. byte is IN ib. */
        /* result goes to ia (l.o.) AND ib (h.o.) */

        /* is op2 IN gpr's */
        k = -1;
        if ((lp = regAlloc[sp].assignment) > 0) /* perform ACC-reg operation */
            emit(op, REGLOW(lp), 0);
        else if ((k = regAlloc[sp].litv) < 0) { /* is op2 a literal */
            loadv(sp, 2);                       /* perform operation with low order byte */
            emit(op, ME, 0);
        } else if (op == XR && k % 256 == 255) /* use CMA if op is XR AND op2 is LIT 255 */
            emit(CMA, 0, 0);
        else /* perform ACC-immediate operation */
            emit(op, -k % 256, 0);

        /* set up a pending register store */
        /* if this is NOT a compare */
        if (op != CP)
            state[RA].regSP = ia;
        if (regAlloc[sp].prec == 2) {
            if (k < 0 && lp <= 0) { /* is h.o. byte of op2 IN memory */
                emit(IN, RL, 0);    /* point to h.o. byte with h AND l */
                state[RL].regValue++;
            }

            /* do we need to pad with h.o. ZERO for op1 */
            if ((jp = state[RA].regSP)) {
                if (jp != ib) { // is store pending
                    emit(LD, jp, RA);
                    state[RA].regSP = 0;
                    if (regAlloc[sp - 1].prec > 1)
                        emit(LD, RA, ib);
                    else
                        emit(cyflag ? LD : XR, RA, 0);
                }
            } else if (regAlloc[sp - 1].prec > 1)
                emit(LD, RA, ib);
            else
                emit(cyflag ? LD : XR, RA, 0);

            if (lp) /* op2 in gpr's - perform ACC-register operation */
                emit(op2, REGHIGH(lp), 0);
            else if (k < 0) /* perform ACC-memory operation */
                emit(op2, ME, 0);
            else if (op2 != XR || k != 65535) /* yes - perform ACC-immediate operation */
                emit(op2, -HIGH(k), 0);
            else /* use CMA if op1 is XR AND op2 is 65535 */
                emit(CMA, 0, 0);
        } else if (regAlloc[sp - 1].prec >= 2) { /* second operand is single byte */
            /* may not need to perform operations for certain operators, but ... */
            /* perform operation with h.o. byte of op1 */
            /* op1 must be in the gpr's - perform dummy operation with ZERO */
            if ((jp = state[RA].regSP)) {
                if (jp != ib) {
                    emit(LD, jp, RA);
                    state[RA].regSP = 0;
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
        state[RA].regSP = ib;

    /* save the pending accumulator - register store */
    jp = state[RA].regSP;
    pop(2);
    state[RA].regSP    = jp;
    regAlloc[++sp]     = newReg(REGPAIR(ib, ia), 1, 0, -1);
    state[ia].regSP    = sp;
    state[ia].regValue = -1;
    if (ib > 0) {
        regAlloc[sp].prec  = 2;
        state[ib].regSP    = sp;
        state[ib].lock     = false;
        state[ib].regValue = -1;
    }
}

void genreg(int np, int *ia, int *ib) {
    int freeReg;

    /* generate abs(np) free registers for subsequent operation */
    // Note -2 <= np <= 2 np negative if no pushing allowed

    *ib = 0;
    *ia = 0;

    /* look for free RC or RE AND allocate in pairs (RC/RB,RE/RD) */
    while (state[freeReg = RC].regSP && state[freeReg = RE].regSP) {
        int candidate = 0;
        if (np >= 0 && sp > 0) {
            /* generate temporaries in the stack and re-try */
            /* search for lowest register pair assignment in stack */
            for (int reg = 1; reg <= sp; reg++) {
                int assign = regAlloc[reg].assignment;
                if (assign == 0) { // not assigned
                    // reset search if already stacked
                    if (regAlloc[reg].st == 0 && regAlloc[reg].litv < 0)
                        candidate = 0;
                } else if (!COND(assign) &&
                           candidate == 0) { // don't look at cond or if already have candidate
                    int lowReg  = REGLOW(assign);
                    int highReg = REGHIGH(assign);
                    // is single reg usage or pair and both not locked
                    if (!state[lowReg].lock &&
                        (highReg == 0 || (highReg == lowReg - 1 && !state[highReg].lock)))
                        candidate = reg; // have candidate
                }
            }
        }
        if (candidate == 0) { // didn't find a candidate
            saver();
            return;
        }
        /* found entry to push at ip */
        int lowReg          = REGLOW(regAlloc[candidate].assignment);
        int highReg         = REGHIGH(regAlloc[candidate].assignment);

        state[lowReg].regSP = 0; // mark as freed
        if (highReg > 0)         // part of a pair
            state[highReg].regSP = 0;
        int raRegs = state[RA].regSP;
        /* check pending register store */
        if (raRegs && (raRegs == lowReg || raRegs == highReg)) {
            emit(LD, raRegs == lowReg ? lowReg : highReg, RA);
            state[RA].regSP = 0;
        }

        /* free the register for allocation */
        stack(1);
        emit(PUSH, lowReg - 1, 0);

        /* mark element as stacked (st=0, rasn=0) */
        regAlloc[candidate].st = regAlloc[candidate].assignment = 0;
        regAlloc[candidate].litv                                = -1;
    }
    *ia = freeReg;
    if (abs(np) > 1)
        *ib = *ia - 1;
    return;
}

void loadv(int s, int typ) {
    int lp, jp;
    int ia, ib;

    /* load value to register if not a literal */
    /* typ = 0, load directly into registers */
    /* typ = 1 if call from 'apply' in which case the l.o. byte is */
    /* loaded into the accumulator instead of a gpr. */
    /* if typ = 2, the address is loaded, but the variable is not. */
    /* if typ = 3, a double byte (address) fetch is forced. */
    /* if typ = 4 then do a quick load into h and l */
    /* if typ = 5, a double byte quick load into h and l is forced */
    int reg = 0;
    if (typ != 2) {
        if (COND(regAlloc[s].assignment)) // convert conditional register to normal
            cvcond(s);
        if (typ == 4 || typ == 5) {
            int litv = regAlloc[s].litv;
            int st   = regAlloc[s].st;
            if (regAlloc[s].assignment) { /* registers are assigned */
                int regsA  = state[RA].regSP;
                int regLow = REGLOW(regAlloc[s].assignment);
                reg        = REGHIGH(regAlloc[s].assignment);
                if (regsA != 0 && regsA == reg)
                    reg = RA;
                if (regsA != 0 && regsA == regLow)
                    regLow = RA;
                if (regLow == RE && reg == RD)
                    emit(XCHG, 0, 0);
                else { /* NOT IN d AND e, so use two byte move */
                    emit(LD, RL, regLow);
                    /* note that the following may be a lhi 0 */
                    emit(LD, RH, reg);
                }
            } else {
                if (st) {
                    /* variable , literal  OR address reference */
                    if (st <= 0)
                        litadd(sp); /* ADR ref - set h AND l with litadd */
                    else if (litv < 0) {
                        /* simple variable OR literal ref, may use LHLD */
                        /* may want to check for possible INX OR DCX, but now... */
                        litv     = state[RH].regValue;
                        int regL = state[RL].regValue;
                        if (litv != -3 || (-regL) != st) {
                            if (litv == (-4) && (-regL) == st)
                                emit(DCX, RH, 0);
                            else
                                emit(LHLD, chain(st, codloc + 1), 0);
                        }
                        state[RH].regValue = -1;
                        state[RL].regValue = -1;
                        if (regAlloc[s].prec <= 1 && typ != 5)
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
                    if (regAlloc[s].prec < 2)
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
            if (regAlloc[s].assignment == 0)
                regAlloc[s].assignment = REGPAIR(RH, RL);
            return;
        } else if (regAlloc[s].assignment > 0)
            return;
        else { /* check for previously stacked value */
            if (regAlloc[s].st != 0 || regAlloc[s].litv >= 0) {
                /* no registers assigned.  allocate registers AND load value. */
                reg = regAlloc[s].prec;
                if (typ == 3) { /* force a double byte load */
                    reg = 2;
                    typ = 0;
                }
                genreg(reg, &ia, &ib);

                /* ia is low order byte, ib is high order byte. */
                if (ia <= 0) {
                    Fatal("112: register allocation error");
                    return;
                }
            } else {
                genreg(2, &ia, &ib);
                /* check to ensure the stack is in good shape */
                for (int reg = s + 1; reg <= sp; reg++) {
                    if (regAlloc[reg].st == 0 && regAlloc[reg].assignment == 0 &&
                        regAlloc[reg].litv < 0)
                        /* found another stacked value */
                        error("147: stack not empty at end of compilation. Register stack "
                              "order is invalid");
                }
                /* available cpu register is based at k */
                emit(POP, ia - 1, 0);
                state[ia].regSP = s;
                if (regAlloc[sp].prec >= 2) {
                    state[ia - 1].regSP = s;
                    ia                  = REGPAIR(ia - 1, ia);
                }
                regAlloc[s].assignment = ia;
                if (typ == 1) {
                    if (state[RA].regSP)
                        emit(LD, state[RA].regSP, RA);
                    emit(LD, RA, ia);
                }

                /* decrement the stack count for this level */
                ustack();
                return;
            }
        }
    }

    /* check for literal value (in arith exp) */
    // guard typ == 2 only ever has -ve lit, so doesn't need ia or ib
    int litv = regAlloc[s].litv;
    if (typ != 2 && 0 <= litv && litv <= 65535) {
        regAlloc[s].litv   = -1;
        lp                 = litv % 256;
        state[ia].regSP    = s;
        state[ia].regValue = lp;
        if (typ == 1) { /* check for pending register store */
            jp = state[RA].regSP;
            if (jp != 0) { /* store ACC into register before continuing */
                emit(LD, jp, RA);
                state[RA].regSP = 0;
            }
            if (lp == 0)
                emit(XR, RA, 0);
            if (lp != 0)
                emit(LD, RA, -lp);
            if (ib != 0) {
                emit(LD, ib, -litv / 256);
                state[ib].regSP    = s;
                state[ib].regValue = -litv;
            }
        } else {
            /* typ = 0, load directly into registers */
            /* may be possible to LXI */
            ib &= 7; // avoid compiler warning
            if (ib != ia - 1) {
                emit(LD, ia, -lp);
                if (ib != 0) {
                    emit(LD, ib, -litv / 256);
                    state[ib].regSP    = s;
                    state[ib].regValue = -litv;
                }
            } else {
                emit(LXI, ib, litv);
                state[ib].regSP    = s;
                state[ib].regValue = -litv;
            }
        }
        regAlloc[s].assignment = REGPAIR(ib, ia);
    } else { /* otherwise fetch from memory */
        sp++;
        setadr(regAlloc[s].st);
        litadd(sp);

        /* address of variable is in h and l */
        if (typ == 0) /* call from gensto (typ = 0) */
            emit(LD, ia, ME);
        else if (typ == 1) { /* call from apply to load value of variable */
            jp = state[RA].regSP;
            /* check for pending register store */
            if (jp != 0) { /* have to store ACC into register before reloading */
                emit(LD, jp, RA);
                state[RA].regSP = 0;
            }
            emit(LD, RA, ME);
        }

        /* check for double byte variable */
        if (typ != 2 && reg > 1) { /* load high order byte */
            emit(IN, RL, 0);
            state[RL].regValue++;
            emit(LD, ib, ME);
        }

        /* value is now loaded */
        pop(1);
        if (typ != 2) {
            regAlloc[s].assignment = REGPAIR(ib, ia);
            state[ia].regSP        = s;
            state[ia].regValue     = -1;
            if (ib) {
                state[ib].regSP    = s;
                state[ib].regValue = -1;
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
        regAlloc[sp] = newReg(0, INFO_PREC(attrib), -val,
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

int chain(int sy, int loc) {
    /* chain in double-byte refs to symbol sy, if necessary */
    if (symAddr(sy) < 0)
        return -symAddr(sy) & 0xffff; // absolute address already assigned

    int next   = symRef(sy);
    symRef(sy) = loc;
    return next;
}

void gensto(bool keep) {
    int reg, j;
    static int iq;

    /* keep = false if STD, keep = true if STO (value retained) */
    /* generate a store into the address at stack top */
    /* load value if not literal */
    int l = regAlloc[sp - 1].litv;
    if (l < 0)
        loadv(sp - 1, iq = 0);

    uint8_t regLow  = REGLOW(regAlloc[sp - 1].assignment);
    uint8_t regHigh = REGHIGH(regAlloc[sp - 1].assignment);

    /* check for pending register store */
    int regA = state[RA].regSP;
    if (regA) {
        if (regA == regHigh)
            regHigh = RA;
        if (regA == regLow)
            regLow = RA;
    }

    do {                             // used to change goto L230 into break
        if (-regAlloc[sp].st != 6) { /* ** note that this assumes 'stackptr' is at 6 IN sym tab */
            if (regAlloc[sp].litv < 0) {
                bool skipUnlockHL = false; // used to eliminate goto L220
                do {                       // used change goto L210 & L220 to break
                    reg = regAlloc[sp].assignment;
                    if (reg <= 0 && regAlloc[sp].st ==
                                        0) { /* registers NOT allocated - check for stacked value */
                        emit(POP, RH, 0);    // address is stacked so POP to h AND l
                        ustack();
                        break;
                    } else if ((reg = regAlloc[sp].st) <=
                               intbas) { /* check for ref to simple based variable */
                        if (regLow)
                            state[regLow].lock = true;
                        if (regHigh)
                            state[regHigh].lock = true;

                        loadv(sp, 3); /* force a double byte fetch into gprs */
                        reg = regAlloc[sp].assignment;
                    } else { /* may be able to simplify (OR eliminate) the LHLD */
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
                        skipUnlockHL = true;
                        break;
                    }
                    regA = state[RA].regSP;
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
                            break;
                        }
                    } else if (reg != RD ||
                               lastex != codloc - 1) { /* if prevous syllable is XCHG then do
                                                          another - peep will fix it */
                        if (regLow != RA) {            /* use STAX - set up accumulator */
                            if (regA != 0)
                                emit(LD, regA, RA);
                            if (regHigh == RA)
                                regHigh = regA;
                            emit(LD, RA, regLow);
                            state[RA].regSP = 0;
                        }
                        emit(STAX, reg, 0);
                        if (regAlloc[sp].prec >= 2) { /* if byte dest we are done */
                            emit(INCX, reg, 0);
                            if (regHigh != 0) { /* store high order byte */
                                if (regLow == 1 && keep) {
                                    emit(LD, REGLOW(regAlloc[sp - 1].assignment), RA);
                                    state[RA].regSP = 0;
                                }
                                emit(LD, RA, regHigh);
                                emit(STAX, reg, 0);
                            } else {                     /* store high order ZERO */
                                if (regLow == 1 && keep) // V4 fix
                                    emit(LD, REGLOW(regAlloc[sp - 1].assignment), RA);
                                state[RA].regSP = 0;
                                emit(XR, RA, 0);
                                emit(STAX, reg, 0);
                            }
                        }
                        pop(1);
                        return;
                    }
                    emit(XCHG, 0, 0);
                } while (false);
                if (!skipUnlockHL) {
                    if (regHigh != 0)
                        state[regHigh].lock = false;
                    if (regLow != 0)
                        state[regLow].lock = false;
                    state[RH].regValue = state[RL].regValue = -1;
                }
            } else if (regHigh != RD || regLow != RE || lastex != codloc - 1 ||
                       regAlloc[sp].prec != 2) /* otherwise this is a literal address */
                litadd(sp);                    /* if possible, generate a SHLD */
            else {
                emit(XCHG, 0, 0);
                reg = abs(regAlloc[sp].st);
                j   = chain(reg, codloc + 1);
                emit(SHLD, j, 0);
                state[RH].regValue = -3;
                state[RL].regValue = -reg;
                if (keep)
                    emit(XCHG, 0, 0);
                break;
            }

            /* we may change mov r,m inr r mov m,r to inr m. */
            /* if so, AND this is a non-destructive store, the register */
            /* assignment must be released. */
            iq = lastIncReg;

            /* generate low order byte store */
            emit(LD, ME, regLow != 0 ? regLow : -abs(l) % 256); /* check for immediate store */
            if (regAlloc[sp].prec != 1) { /* now store high order byte (if any) */
                emit(INCX, RH, 0);        /* store second byte */
                /* regv(RH) = -3 then LHLD has occurred on symbol -regv(RL) */
                /* regv(RH) = -4 then LHLD AND INCX h has occurred */
                j = state[RH].regValue;
                if (j >= 0)
                    state[RL].regValue++;
                else {
                    state[RH].regValue = -4;
                    if (j != -3) /* RH AND RL have unknown values */
                        state[RH].regValue = state[RL].regValue = -1;
                }
                if (regAlloc[sp - 1].prec >= 2) {
                    if (regHigh == 0) /* second byte is literal */
                        emit(LD, ME, -abs(l / 256));
                    else /* LD memory from register */
                        emit(LD, ME, regHigh);
                } else
                    emit(LD, ME, 0);
            }
        } else if (regLow == 0) /* load sp immediate */
            emit(LXI, RSP, l);
        else {
            emit(LD, RL, regLow);
            emit(LD, RH, regHigh);
            emit(SPHL, 0, 0);
            state[RH].regValue = state[RL].regValue = -1;
        }
    } while (false);

    /* now release register containing address */
    /* release register assignment for value */
    /* if mov r,m inr r mov m,r was changed to inr m. */
    if (iq != codloc)
        pop(1);
    else {
        reg = -regAlloc[sp].st;
        pop(2);
        regAlloc[++sp] = newReg(0, 1, reg, -1);
    }
}

void litadd(int s) {
    int it, assignment, k, j;
    /* load h AND l with the address of the variable at s in */
    /* the stack */

    if (regAlloc[s].litv < 0)
        error("114: pass-2 compiler error in 'litadd'");
    else if ((assignment = regAlloc[s].assignment) != REGPAIR(RH, RL)) { /* deassign registers */
        int jp = state[RA].regSP;
        if ((k = REGLOW(assignment))) {
            if (k == jp)
                state[RA].regSP = 0;
            state[k].regSP    = 0;
            state[k].lock     = false;
            state[k].regValue = -1;
        }
        if ((k = REGHIGH(assignment))) {
            if (k == jp)
                state[RA].regSP = 0;
            state[k].regSP    = 0;
            state[k].lock     = false;
            state[k].regValue = -1;
        }
        regAlloc[s].assignment = 0;
        int il                 = regAlloc[s].litv % 256;
        int ih                 = regAlloc[s].litv / 256;
        int lp;
        for (int ir = RH; ir <= RL; ir++) {
            int regVal = ir == RH ? ih : il;
            if ((j = state[ir].regSP)) {
                if (REGLOW(regAlloc[j].assignment) == ir)
                    regAlloc[j].assignment &= ~0xf;
                if (REGHIGH(regAlloc[j].assignment) == ir)
                    regAlloc[j].assignment &= ~0xf0;
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
                        it = regAlloc[s].st;
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
                else if ((it = regAlloc[s].st) >= 0) { /* the address must be backstuffed later */
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
                regAlloc[s].assignment = REGPAIR(RH, RL);

            state[ir].regSP    = s;
            state[ir].regValue = regVal;
        }
    }
}

unsigned char cbits[43 + 1] = { 0,   0x40, 4,   5,   0x80, 136, 144, 152, 160, 168, 176,
                                184, 7,    195, 194, 205,  196, 201, 192, 199, 219, 211,
                                118, 50,   58,  235, 249,  233, 47,  55,  63,  39,  34,
                                42,  251,  243, 1,   197,  193, 9,   2,   10,  3,   11 };
unsigned char cc[2][4]      = { { 0x10, 0x00, 0x30, 0x20 }, { 0x18, 0x08, 0x38, 0x28 } };
// fortan used bits(3)
// bits(1) -> opcode
// bits(2) -> operand low
// bits(3) -> operand high

/* the following comments are sample calls to the emit */
/* routine.  note that emit requires three argument at all times */
/* (the unused arguments are ZERO). */

/* call emit(LD,RA,RB) */
/* call emit(LD,RC,-34) */
/* call emit(LD,RD,ME) */
/* call emit(LD,ME,RE) */
/* call emit(IN,RH,0) */
/* call emit(DC,RL,0) */
/* call emit(AD,RB,0) */
/* call emit(AD,ME,0) */
/* call emit(AD,-5,0) */
/* call emit(SU,RB,0) */
/* call emit(SB,ME,0) */
/* call emit(ND,-5,0) */
/* call emit(XR,0,0) */
/* call emit(OR,RB,0) */
/* call emit(CP,RH,0) */
/* call emit(ROT,ACC,LFT) */
/* call emit(ROT,CY,LFT) */
/* call emit(ROT,CY,RGT) */
/* call emit(JMP,148,0) */
/* call emit(JMC,TRU*32+ZERO,148) */
/* call emit(CAL,1048,0) */
/* call emit(CLC,FAL*32+PARITY,148) */
/* call emit(RTN,0,0) */
/* call emit(RTC,FAL*32+CARRY,255) */
/* call emit(RST,3,0) */
/* call emit(INP,6,0) */
/* call emit(OUT,10,0) */
/* call emit(HALT,0,0) */
/* emit a literal between 0 AND 255 */
/* call emit(0,44,0) */
/* call emit(STA,300,0) */
/* call emit(LDA,300,0) */
/* call emit(XCHG,0,0) */
/* call emit(SPHL,0,0) */
/* call emit(PCHL,0,0) */
/* call emit(CMA,0,0) */
/* call emit(STC,0,0) */
/* call emit(CMC,0,0) */
/* call emit(DAA,0,0) */
/* call emit(SHLD,300,0) */
/* call emit(LHLD,300,0) */
/* call emit(EI,0,0) */
/* call emit(DI,0,0) */
/* call emit(LXI,(RB,RD,RH,RSP),300) */
/* call emit(PUSH,(RB,RD,RH,RA),0) */
/* call emit(POP,(RB,RD,RH,RA),0) */
/* call emit(DAD,(RB,RD,RH,RSP),0) */
/* call emit(STAX,(RB,RD),0) */
/* call emit(LDAX,(RB,RD),0) */
/* call emit(INX,(RB,RD,RH,RSP),0) */
/* call emit(DCX,(RB,RD,RH,RSP),0) */

void emit(int opr, int opa, int opb) {
    int opcode, operand, n, reg;

    n = 1;
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
                    lastRegImm < 8 && lastRegImm > 1) {
                    // here we have a lri operation following a lri to the other
                    // register in this pair (b/c,d/e,h/l) or to the same reg.
                    if (lastRegImm == opa) { // here to same register
                        put(codloc - 1, -opb);
                        return;
                    }
                    // here to the other register in the pair
                    // we have to change the opcode to a lxi of the high reg (b,d,h)
                    // also we will only add 1 word and we can clear lastli
                    opcode = 1 + (opa / 2 - 1) * 16; // replace mvi with the lxi instruction
                    put(codloc - 2, opcode);
                    n           = 1;
                    lastLoadImm = 0;
                    if (opa % 2 == 0) {
                        // here the low register (c,e,l) was already there so we just add
                        // the high reg.
                        opcode = -opb;
                    } else {
                        // here the high register (b,d,h) was already there so we need to
                        // insert the low register between the opcode and it.
                        opcode = get(codloc - 1);
                        put(codloc - 1, -opb);
                    }
                } else { // here the lri didn-t follow a lri that we could change
                    lastLoadImm = codloc;
                    lastRegImm  = opa;

                    n           = 2;                     /* 2 byte instruction */
                    opcode      = 0x6 + regmap[opa] * 8; /* gen MVI instruction */
                    operand     = -opb;                  /* and the value */
                }
            } else {
                /* check for possible load register elimination */
                /* is this a lmr OR lrm instruction... */
                // if mov m,r followed by mov r,m then optimise 2nd instruction away
                if (opa != ME) {
                    if (opb == ME && lastLoad == codloc - 1 && lastReg == opa)
                        return;

                    /* may change a mov r,m inr/dcr r mov m,r to inr/dcr m */
                } else if (lastIncReg == codloc - 1) {
                    // other code has flagged this is inc of same register
                    reg = (get(codloc - 1) & 7) + 0x30; // generate the inr/dcr

                    /* the register load may have been eliminated... */
                    if (lastLoad != codloc - 2 || opb != lastReg)
                        codloc--;
                    put(codloc - 1, reg);
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
                /* this is a load memory from register operation - save */

                opcode += (regmap[opa] << 3) + regmap[opb];
            }
            break;
        case IN:
        case DC:                        /* inr & dcr */
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
            else {     /* immediate operand */
                n = 2; // normally 2 byte code
                if (v4Opt && opa == 0) {
                    switch (opr) {
                    case AD:
                    case SU:
                    case XR:
                    case OR:
                        // replace adi,0 sui,0 xri,0 or ori 0 with ora,a
                        opcode = cbits[OR] + 7;
                        n      = 1;
                        break;
                    case ND: // replace ani,0 with xra,a
                        opcode = cbits[XR] + 7;
                        n      = 1;
                        break;
                    }
                }
                if (n == 2) {       // make the 2 byte code
                    opcode += 0x46; /* gen imm arith instruction */
                    operand = -opa; /* the immediate value*/
                }
            }
            break;
        case ROT: /* rotate group */
            reg = (opa - CY) * 2 + (opb - LFT);
            opcode += reg * 8;
            break;

        case JMC:
        case CLC: /* condiitonal jmp & call */
            n       = 3;
            operand = opb;
            // fallthrough
        case RTC: /* conditional return */
            opcode += cc[(opa / 32 - FAL) & 1][(opa % 32 - CARRY) & 3];
            break;
        case RST: /* rst xx */
            opcode += (opa & 7) << 3;
            break;
        case INP:
        case OUT: /* in & out */
            n       = 2;
            operand = opa;
            break;
        case JMP:
        case CAL: /* jmp & call */
        case STA:
        case LDA:
        case SHLD:
        case LHLD: /* STA LDA SHLD LHLD */
            n       = 3;
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
        case HALT: /* ret & halt */
        case SPHL:
        case PCHL:
        case CMA:
        case STC:
        case CMC:
        case DAA:
        case EI:
        case DI:
            break;
        case LXI:                      /* LXI (get immediate part) */
            n = 3;                     /* 3 byte instruction */
            opcode += regmap[opa] * 8; /* merge in the register */
            operand = opb;             /* and the location */
            break;
        case PUSH:
            /* PUSH r - check for XCHG PUSH d combination. change to PUSH h */
            // PMO, relies on no code being generated that uses existing DE or HL after this
            if (lastex == codloc - 1 && opa == RD) {
                codloc--;
                lastex = 0;
                opcode += regmap[RH] * 8;
                break;
            }
            // fallthrough
        case POP:
        case DAD:
        case STAX:
        case LDAX:
        case INCX:
        case DCX:
            /* adjust for push/pop psw */
            reg = opa == RA ? 6 : regmap[opa];
            opcode += reg << 3;
        }
    }
    put(codloc++, opcode);
    if (n > 1) {
        put(codloc++, operand % 256);
        if (n > 2)
            put(codloc++, operand / 256);
    }
    return;
}

void cvcond(int s) {

    /* convert the condition code at s IN the stack to a boolean value */
    int assign = regAlloc[s].assignment;
    int ia     = assign & 0xf;
    int test   = (assign >> 8) & 0xf;
    int j      = (assign >> 12) & 0xf;

    /* test = 1 if CARRY, 2 if ZERO, 3 if SIGN, AND 4 if PARITY */

    /* we may generate a short sequence */
    if (test <= 2 && ia && state[RA].regSP == ia) {
        if (test != 2) { /* short conversion for true OR false CARRY */
            emit(SB, RA, 0);
            if (j == 0)
                emit(CMA, 0, 0);
        } else {        /* short conversion for true OR false ZERO */
            if (j == 0) /* j = 1 if true , j = 0 if false */
                emit(AD, -255, 0);
            else if (j == 1)
                emit(SU, -1, 0);
            emit(SB, RA, 0);
        }
    } else { /* do we have to assign a register */
        int jp;
        if (ia == 0) {
            genreg(1, &ia, &jp);
            if (ia) {
                state[ia].regSP = sp;
                assign          = ia;
            } else {
                Fatal("118: register allocation error");
                return;
            }
        }

        /* check pending register store */
        jp = state[RA].regSP;
        if (jp && jp != ia) {
            emit(LD, jp, RA);
            state[RA].regSP = 0;
        }
        emit(LD, RA, -255);
        j = (FAL + j) * 32 + (CARRY + test - 1);
        emit(JMC, j, codloc + 4);
        emit(XR, RA, 0);
    }

    /* set up pending register store */
    state[RA].regSP        = ia;
    regAlloc[s].assignment = assign & 0xff;
    return;
}

void saver() {
    int byteCnt, wordCnt, byteChain, wordChain, reg;

    /* save the active registers and reset tables */
    /* first determine the stack elements which must be saved */

    if (sp != 0) {
        wordChain = byteChain = wordCnt = byteCnt = 0;
        for (int j = 1; j <= sp; j++) {
            int regPair = regAlloc[j].assignment;
            if (COND(regPair)) {
                cvcond(j);
                regPair = regAlloc[j].assignment;
            }
            if (regPair >= 16) { /* double byte */
                if (!(state[REGLOW(regPair)].lock || state[REGHIGH(regPair)].lock)) {
                    regAlloc[j].st = wordChain;
                    wordChain      = j;
                    wordCnt++;
                }
            } else if (regPair > 0 && !state[REGLOW(regPair)].lock) { /* single byte */
                regAlloc[j].st = byteChain;
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
            while (byteChain + wordChain) {
                if (loc % 2 || wordChain == 0) /* single byte */
                    byteChain = regAlloc[reg = byteChain].st;
                else /* even byte boundary with double bytes to store */
                    wordChain = regAlloc[reg = wordChain].st;

                if (reg <= 0) {
                    error("120: memory allocation error");
                    return;
                }

                /* place temporary into symbol table */
                regAlloc[reg].st = ++sytop;
                symbol[sytop]    = syinfo;

                symbol[syinfo]   = loc;
                loc += REGHIGH(regAlloc[reg].assignment) ? 2 : 1;

                symbol[--syinfo] = PACK_ATTRIB(1, REGHIGH(regAlloc[reg].assignment) ? 2 : 1, VARB);

                /* leave room for LXI chain */
                symbol[--syinfo] = 0;
                if (sytop > --syinfo) {
                    Fatal("121: pass-2 symbol table overflow");
                    return;
                }

                /* store into memory */
                int regPair              = regAlloc[reg].assignment;
                regAlloc[reg].assignment = 0;
                sp++;
                setadr(sytop);
                litadd(sp);
                while (regPair) {
                    reg = REGLOW(regPair);
                    if (reg == state[RA].regSP) {
                        reg                = RA;
                        state[RA].regSP    = 0;
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
            state[reg].regSP    = 0;
            state[reg].regValue = -1;
        }
    return;
}

void reloc() {
    int stloc;
    int stsize;
    int reg, j, k, l, n, ip;
    if (C_SYMBOLS >= 2) {
        for (int reg = 1; reg <= sytop; reg++)
            fprintf(lstFp, "%4d = %6d\n", reg, symbol[reg]);
        for (int reg = syinfo; reg <= symax; reg++)
            fprintf(lstFp, "%5d = %c%08XH\n", reg, (symbol[reg] < 0) ? '-' : ' ', abs(symbol[reg]));
    }

    /* compute max stack depth required for correct execution */
    stsize = maxdep[0];
    for (int n = 0; n < 8; n++) {
        if (intpro[n]) /* get interrupt procedure depth */
            /* note that i exceeds depth by 1 since RET may be pending */
            stsize += symIProcDepth(intpro[n]) + 1;
    }
    stsize *= 2;
    n = stsize;
    if (C_STACKHANDLING != 0)
        n = 0;

    /* align to even boundary, if necessary */
    if (n != 0 && lmem % 2 == 1)
        lmem--;
    stloc = lmem;
    lmem -= n;

    /* stsize is number of bytes reqd for stack, stloc is addr */
    // iw = C_WIDTH / 14;   not used
    n = 0;

    /* compute page to start variables */
    reg = 0;
    if (codloc % 256 > lmem % 256)
        reg = 1;
    reg += codloc / 256;
    if (C_VARIABLES > reg)
        reg = C_VARIABLES;

    /* compute first relative address page */
    j = lmem / 256 - reg;
    if (j < 0)
        error("122: program requires too much program and variable storage");

    else {
        for (int reg = 1; reg <= sytop; reg++) {
            k = symAddr(reg);
            if (k >= 0) {                   /* now fix page number */
                l            = HIGH(k) - j; /* l is relocated page number */
                symAddr(reg) = (l << 8) + LOW(k);
                int target   = HIGHWORD(k);
                for (;;) {
                    if (target) { /* backstuff lhi l into location target-1 */
                        ip = get(target - 1) * 256 + get(target);
                        put(target - 1, 0x26);
                        put(target, l);
                        target = ip;
                    } else { /* backstuff LXI references to this variable */
                        target = symAddr(reg);
                        for (int link = symRef(reg); link;) {
                            int next = getword(link);
                            putword(link, target);
                            link = next;
                        }
                        break;
                    }
                }
            }
        }
        if (C_MAP)
            putc('\n', lstFp);

        /* relocate AND backstuff the stack top references */
        stloc -= j * 256;
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
        for (int reg = 1; reg <= sytop; reg++) {
            k = -symAddr(reg);
            l = INFO_TYPE(abs(symAttrib(reg)));
            if (l == LABEL || l == PROC) {
                int n       = k & 3;
                int addr    = k / 4; // does right thing if k -1 .. -3
                int backref = symRef(reg);
                while (addr) { // V4
                    int next = getword(addr);
                    putword(addr, backref);
                    addr = next;
                }
                symAddr(reg) = n;
            }
        }
        if (preamb > 0) {
            for (int reg = 0; reg < 8; reg++)
                if (intpro[reg]) {
                    put(reg * 8, 0xc3);
                    putword(reg * 8 + 1, symRef(intpro[reg]));
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

void unary(const int val) {
    int reg, j, k, ia, ib, jp, iq;

    /* 'val' is an integer corresponding to the operations-- */
    /* RTL(1) RTR(2) SFL(3) SFR(4) scl(5) scr(6) HIV(7) LOV(8) */
    if (COND(regAlloc[sp].assignment))
        cvcond(sp);
    int ip = regAlloc[sp].prec;
    switch (val) {
    case B_ROL:
    case B_ROR:
        if (ip <= 1) {
            if (regAlloc[sp].assignment == 0) {
                loadv(sp, 1);
                state[RA].regSP = REGLOW(regAlloc[sp].assignment);
            }
            reg = REGLOW(regAlloc[sp].assignment);
            k   = state[RA].regSP;
            if (k != 0) {
                if (k != reg) {
                    emit(LD, k, RA);
                    emit(LD, RA, reg);
                    state[RA].regSP = reg;
                }
            } else {
                emit(LD, RA, reg);
                state[RA].regSP = reg;
            }
            reg = LFT;
            if (val == B_ROR)
                reg = RGT;
            emit(ROT, CY, reg);
            return;
        }
        break;
    case B_SHL:
    case B_SHR:
    case B_SCL: // SCL
    case B_SCR: // SCR

        j   = (val == B_SHR || val == B_SCR) && ip > 1 ? 0 : 1;

        reg = regAlloc[sp].assignment;
        if (reg <= 0) {
            /* load from memory */
            loadv(sp, j);
            reg = regAlloc[sp].assignment;
            if (j == 1)
                state[RA].regSP = REGLOW(reg);
        }

        /* may have to store the accumulator */
        ia = REGLOW(reg);
        ib = REGHIGH(reg);
        k  = j != 1 ? ib : ia;

        jp = state[RA].regSP;

        /* we want register k to be IN the accumulator */
        if (jp != k) {
            if (jp != 0)
                emit(LD, jp, RA);
            emit(LD, RA, k);
        }
        state[RA].regSP = k;

        /* SFL AND SFR take separate paths now... */
        if (val == B_SHR || val == B_SCR) {
            if (val == B_SHR)
                emit(OR, RA, 0);
            emit(ROT, ACC, RGT);
            if (ip >= 2) {
                emit(LD, ib, RA);
                emit(LD, RA, ia);
                emit(ROT, ACC, RGT);
                state[RA].regSP = ia;
            }
        } else {
            /* SFL - clear CARRY AND shift */
            if (val == B_SHL)
                emit(AD, RA, RA);
            if (val == B_SCL)
                emit(ROT, ACC, LFT);
            if (ip >= 2) {
                emit(LD, ia, RA);
                emit(LD, RA, ib);
                emit(ROT, ACC, LFT);
                state[RA].regSP = ib;
            }
        }
        return;
    case B_TIME:
        break;
    case B_HIGH:
        if (ip >= 2) {
            if (regAlloc[sp].assignment <= 0)
                loadv(sp, 0);
            ip = REGHIGH(regAlloc[sp].assignment);
            iq = REGLOW(regAlloc[sp].assignment);
            if (state[RA].regSP == iq)
                state[RA].regSP = 0;
            state[ip].regSP         = 0;
            state[ip].regValue      = -1;
            regAlloc[sp].assignment = iq;
            regAlloc[sp].prec       = 1;
            if (state[RA].regSP != ip)
                emit(LD, iq, ip);
            else
                state[RA].regSP = iq;
            return;
        }
        break;
    case B_LOW:
        regAlloc[sp].prec = 1;
        /* may have to release register */
        if ((reg = REGHIGH(regAlloc[sp].assignment))) {
            state[reg].regSP    = 0;
            state[reg].regValue = -1;
            if (state[RA].regSP == reg)
                state[RA].regSP = 0;
        }
        regAlloc[sp].assignment = REGLOW(regAlloc[sp].assignment);
        return;
    }
    error("126: built-in function improperly called");
    return;
}

void exch() {
    int j, ia, ib;

    /* exchange the top two elements of the stack */
    j = sp - 1;
    if (regAlloc[j].st == 0 && regAlloc[j].assignment == 0 && regAlloc[j].litv < 0)

        /* second element is pushed - check top elt */
        if (regAlloc[sp].assignment == 0 && regAlloc[sp].litv < 0)

            /* second elt is pushed, top elt is NOT IN cpu */
            if (regAlloc[sp].st == 0) {
                /* both are pushed, so go thru 20 twice */
                j = sp;
                for (;;) {
                    /* POP element (second if drop thru, top if from 30) */
                    genreg(-1, &ia, &ib);
                    if (ia == 0) {
                        Fatal("107: register allocation error. No registers available");
                        break;
                    } else {
                        if (regAlloc[j].prec > 1)
                            ib = ia - 1;
                        emit(POP, ia - 1, 0);
                        ustack();
                        state[ia].regSP = j;
                        if (ib != 0)
                            state[ib].regSP = j;
                        regAlloc[j].assignment = REGPAIR(ib, ia);
                        if (j != sp)
                            break;
                        j = sp - 1;
                    }
                }
            }

    for (int reg = 2; reg <= 7; reg++) {
        if (state[reg].regSP == sp)
            state[reg].regSP = sp - 1;
        else if (state[reg].regSP == sp - 1)
            state[reg].regSP = sp;
    }
    /* now swap the top two elements */
    regAlloc_t tmp;
    tmp              = regAlloc[sp];
    regAlloc[sp]     = regAlloc[sp - 1];
    regAlloc[sp - 1] = tmp;
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
    int lcnt, typ, lline, lloc, val, reg, j, k;
    int ibase, ip, kp, lp, ia, ib;
    char *rmap       = "-ABCDEFGHIJKLMOP";
    char polchr[][4] = { "OPR", "ADR", "VAL", "DEF", "LIT", "LIN" };
    char opcval[][4] = { "NOP", "ADD", "ADC", "SUB", "SBC", "MUL", "DIV", "MDF", "NEG",
                         "AND", "IOR", "XOR", "NOT", "EQL", "LSS", "GTR", "NEQ", "LEQ",
                         "GEQ", "INX", "TRA", "TRC", "PRO", "RET", "STO", "STD", "XCH",
                         "DEL", "DAT", "LOD", "BIF", "INC", "CSE", "END", "ENB", "ENP",
                         "HAL", "RTL", "RTR", "SFL", "SFR", "HIV", "LOV", "CVA", "ORG",
                         "DRT", "ENA", "DIS", "AX1", "AX2", "AX3" };

    C_COUNT          = 1;
    lline            = 0;
    lloc             = 0;
    lcnt             = -1;
    alter            = false;

    /* reserve space for interrupt locations */
    preamb = 0;
    for (int reg = 7; reg >= 0; reg--) {
        if (intpro[reg] != 0) {
            preamb = reg * 8 + 3;
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
        ibase = 0;

        /* may have been stack overflow so... */
        if (sp < 0)
            sp = 0;
        if (C_ANALYSIS && alter && sp > 0) {
            /* write stack */
            fprintf(lstFp, "  PR   ST   RASN  LITV\n");
            for (int ip = sp; ip > 0; ip--) {
                fprintf(lstFp, "%02d %d ", ip, regAlloc[ip].prec);
                if (regAlloc[ip].st)
                    fprintf(lstFp, "%c%05d", regAlloc[ip].st < 0 ? 'A' : 'S', abs(regAlloc[ip].st));
                else
                    fputs("      ", lstFp);

                fprintf(lstFp, "  %c %c", rmap[HIGHNIBBLE(regAlloc[ip].assignment)],
                        rmap[LOWNIBBLE(regAlloc[ip].assignment)]);
                if (regAlloc[ip].litv >= 0)
                    fprintf(lstFp, " %c%05d", HIGHWORD(regAlloc[ip].litv) ? 'R' : ' ',
                            LOWWORD(regAlloc[ip].litv));
                putc('\n', lstFp);
            }

            /* write registers */
            if (C_ANALYSIS >= 2) {
                for (int reg = 1; reg <= 7; reg++) {
                    ip = state[reg].regSP;
                    kp = state[reg].lock;
                    lp = state[reg].regValue;
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
        typ = k & 7;
        val = k >> 3;

        /* $g=0 for no trace, $g=1 gives lines vs locs, */
        /* $g=2 yields full interlist of i.l. */
        if (C_GENERATE != 0) {
            if (C_GENERATE > 1) {
                /* otherwise interlist the i.l. */
#if 0
                FILE *tmp = lstFp;
                lstFp     = stdout;
#endif
                fprintf(lstFp, "sp=%d, %05d %04XH %d %s ", sp, codloc, codloc, polCnt - 1,
                        polchr[typ]);
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
        regAlloc[sp] = newReg(0, 0, 0, -1);

        alter        = false;
        switch (typ) {
        case OPR: /* operator */
            sp--;
            alter |= operat(val);
            continue;
        case ADR:
            if (sp > 1) {
                /* check for active condition code which must be changed to boolean */
                if (regAlloc[sp - 1].assignment > 255)
                    cvcond(sp - 1);
            }
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

            /* save registers if this is a PROC OR a LABEL which was */
            /* referenced IN a go-to statement OR was compiler-generated. */
            ip  = symbol[val];
            reg = abs(symAttrib(val));

            /* save this DEF symbol number AND the literal values of the */
            /* h AND l registers for possible TRA chain straightening. */
            if (INFO_TYPE(reg) == LABEL) {
                defsym = val;
                defrh  = state[RH].regValue;
                defrl  = state[RL].regValue;
            }

            /* we may convert the sequence */

            /* TRC l, TRA/PRO/RET, DEF l */

            /* to an equivalent conditional TRA/PRO/RET... */
            if (INFO_ECNT(reg) == 1)
                if (tstloc == codloc)
                    if (conloc == xfrloc - 3) {
                        int addr = -symAddr(val);
                        // V4
                        if ((-symAddr(val)) / 4 == conloc + 1) {
                            /* adjust backstuffing chain for JMP OR call */
                            if (xfrsym > 0)
                                /* decrement backstuff location by 3 */
                                symAddr(xfrsym) += 3 << 2;

                            /* arrive here with the configuration TRC...DEF */
                            // V4
                            symAddr(val) = -(addr % 4);
                            if (symAttrib(val) < 0)
                                symAttrib(val) = -(abs(symAttrib(val)) & 0xff);
                            else
                                symAttrib(val) &= 0xff;
                            int j = ((get(conloc) ^ 8) & ~7) + (get(xfrloc) & 6);
                            for (;;) {
                                put(conloc, j);
                                conloc++;
                                xfrloc++;
                                j = get(xfrloc);
                                if (xfrloc == codloc) {
                                    codloc = conloc;
                                    conloc = xfrloc = tstloc = -1;

                                    /* notice that defrh AND defrl are now incorrect */
                                    /* defsym=0 prevents use of these variables... */
                                    /* ... if a TRA immediately follows */
                                    defsym = 0;
                                    break;
                                }
                            }
                        }
                    }
            int type = INFO_TYPE(reg);

            if (type == LABEL) { // check for label with references
                int refCnt = INFO_ECNT(reg);
                if (refCnt == 1) { /* single reference, so no conflict with h and l */
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
                } else if (refCnt != 0)
                    saver();
                k = codloc;
            } else if (type == PROC) {
                /* set up procedure stack for procedure entry */
                if (++prsp <= prsmax) {
                    j           = ip - 3;
                    prstk[prsp] = j;

                    /* mark h AND l as unaltered initially */
                    /* /  1b  /  1b  /  1b  /  1b  /  9b  /  8b  / */
                    /* /h unal/l unal/h vald/l vald/h valu/l valu/ */
                    /* ------------------------------------------- */
                    symbol[j] = HNOTSET | LNOTSET;
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
                            prstk[prsp] = j + 65536;
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
            } else {
                saver();
                /* LABEL is resolved.  last two bits of entry must be 01 */
                k = codloc;
            }

            reg = -symAddr(val);
            j   = reg % 4;
            reg = reg / 4;
            if (j != 1)
                error("131: label resolution error in pass-2");
            // V4
            symAddr(val) = -((reg << 2) + 3);
            symRef(val)  = k;
            /* now check for procedure entry point */
            reg = symAttrib(val);
            if (right(reg, 4) == PROC) {
                reg >>= 8;

                /* build receiving sequence for register parameters */
                if (reg >= 1) {
                    k = reg < 2 ? 1 : reg - 1;
                    if (reg > 2)
                        reg = 2;
                    for (int j = 0; j < reg; j++) {
                        if (++sp > maxsp) {
                            Fatal("113: register allocation stack overflow");
                            sp = 1;
                        }

                        /* (RD,RE) = 69    (RB,RC) = 35 */
                        regAlloc[sp] = newReg(j == 0 ? 35 : 69, 2, 0, -1);
                        if (++sp > maxsp) {
                            Fatal("113: register allocation stack overflow");
                            sp = 1;
                        }
                        regAlloc[sp].assignment = 0;
                        regAlloc[sp].litv       = -1;
                        setadr(val + k + j);
                        alter |= operat(STD);
                    }
                }
            }
            continue;
        case LIT:
            /* check for active condition code which must be changed to boolean */
            if (sp > 1 && COND(regAlloc[sp - 1].assignment))
                cvcond(sp - 1);
            alter             = true;
            regAlloc[sp].litv = val;
            regAlloc[sp].prec = regAlloc[sp].litv > 255 ? 2 : 1;
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
                if (COND(regAlloc[sp - 1].assignment))
                    cvcond(sp - 1);
            }
        /* check for condition codes */
        if (val <= intbas) {
            if (val <= 4) {
                /* CARRY ZERO MINUS PARITY */
                /* set to true/condition (1*16+val) */
                regAlloc[sp].assignment = ASSIGN(16 + val, 0, 0);
                regAlloc[sp].st         = 0;
                regAlloc[sp].prec       = 1;
                alter                   = true;
                continue;
            } else if (val < firsti || val > intbas) { /* may be a call to input or output */
                if (val != 5) {                        /* check for reference to 'memory' sybmol 5*/
                    /* ** note that 'stackptr' must be at 6 in sym tab */
                    if (val == 6) { /* load value of stackpointer to registers immediately */
                        genreg(2, &ia, &ib);
                        if (ib == 0)
                            Fatal("107: register allocation error. No registers available");
                        else {
                            regAlloc[sp]    = newReg(REGPAIR(ib, ia), 2, 0, -1);
                            state[ib].regSP = state[ia].regSP = sp;

                            emit(LXI, RH, 0);
                            emit(DAD, RSP, 0);
                            emit(LD, ia, RL);
                            emit(LD, ib, RH);
                            state[RH].regValue = -1;
                            state[RL].regValue = -1;
                            alter              = true;
                        }
                    } else
                        error("129: invalid use of built-in function in an assignment");
                    continue;
                }
            }
        }
        if (j < 0)
            /* value reference to based variable. first insure that this */
            /* is not a length attribute reference, (i.e., the variable is */
            /* not an actual parameter for a call on length or last) by */
            /* insuring that the next polish elt is not an address */
            /* reference to symbol (length+1) OR (last+1) */
            /* note that this assumes length and last are symbol numbers */
            /* 18 AND 19 */
            if (lapol != 153 && lapol != 161) {
                /* load value of base variable.  change to load */
                /* value of base, followed by a LOD op. */
                ibase = 16 + ((-j >> 4) & 0xf);
                val   = symRef(val);
                j     = symAttrib(val);
            }
        alter = true;

        /* examine attributes */
        regAlloc[sp].st   = val;
        k                 = ibase > 0 ? ibase & 0xf : INFO_PREC(j);
        regAlloc[sp].prec = k;
        if (INFO_TYPE(j) >= LITER) {
            if (k > 0 && k < 3) // byte, word, or string
                regAlloc[sp].litv = INFO_ECNT(j);
            else {
                error("130: pass-2 compiler error. Invalid variable precision");
                continue;
            }
        }

        /* check for base address which must be loaded */
        if (ibase >= 16) {
            /* must be a based variable value reference. */
            /* load the value of the base AND follow it by */
            /* a load operation. */
            k = regAlloc[sp].prec;

            /* mark as a byte load for the LOD operation IN operat */
            /* leaves 2 if double byte result AND 6 (=2 mod 4) if single byte */
            regAlloc[sp].prec = 10 - 4 * k;
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

bool doBuiltin(int val) {
    int reg, j, k, m;
    int ip, kp;
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
            reg = regAlloc[sp].litv;
            if (reg > 0) { /* generate in-line code for shift counts of */
                           /* 1 or 2 for address values */
                           /* 1 to 3 for shr of byte values */
                           /* 1 to 6 for all other shift functions on byte values */
                j = regAlloc[sp - 1].prec != 1 ? 2 : val == B_SHR ? 3 : 6;
                if (reg <= j) {
                    pop(1);
                    for (int j = 0; j < reg; j++)
                        unary(val);
                    return true;
                }
            }
            exch();
            /* load the value to decrement */
            loadv(sp - 1, 0);
            j = REGLOW(regAlloc[sp - 1].assignment);
            if (state[RA].regSP == j) {
                emit(LD, j, RA);
                state[RA].regSP = 0;
            }
            state[j].lock = true;

            /* load the value which is to be operated upon */
            kp  = regAlloc[sp].prec;
            reg = kp > 1 ? 0 : 1;
            if (regAlloc[sp].assignment == 0) {
                loadv(sp, reg);
                if (reg == 1)
                    state[RA].regSP = REGLOW(regAlloc[sp].assignment);
            }
            m = REGLOW(regAlloc[sp].assignment);
            if (reg != 1 || state[RA].regSP != m) {
                if (state[RA].regSP) {
                    emit(LD, state[RA].regSP, RA);
                    state[RA].regSP = 0;
                }
                if (reg) {
                    emit(LD, RA, m);
                    state[RA].regSP = m;
                }
            }
            reg = codloc;
            unary(val);
            if (kp != 1) {
                if (state[RA].regSP)
                    emit(LD, state[RA].regSP, RA);
                state[RA].regSP = 0;
            }
            emit(DC, j, 0);
            emit(JMC, FAL * 32 + ZERO, reg);

            /* END up here after operation completed */
            exch();
            state[j].lock = false;
            pop(1);
        }
        return true;
    case B_TIME:
        if (regAlloc[sp].assignment > 255)
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
        j   = state[RA].regSP;
        ip  = REGHIGH(regAlloc[sp].assignment);
        reg = REGLOW(regAlloc[sp].assignment);
        if (!j || j != reg) {
            /* get time parameter into the accumulator */
            if (j != 0 && j != ip)
                emit(LD, j, RA);
            state[RA].regSP = 0;
            if (reg == 0)
                loadv(sp, 1);
            reg = REGLOW(regAlloc[sp].assignment);
            if (j)
                emit(LD, RA, reg);
        }
        state[RA].regSP = 0;
        emit(LD, reg - 1, -12);
        emit(LD, reg, reg - 1);
        emit(DC, reg, 0);
        emit(JMC, FAL * 32 + ZERO, codloc - 1);
        emit(DC, RA, 0);
        emit(JMC, FAL * 32 + ZERO, codloc - 6);
        pop(1);
        return true;
    case B_INPUT:
        /* input function. get input port number */
        reg = regAlloc[sp].litv;
        if (reg >= 0 && reg <= 255) {
            pop(1);
            sp++;
            genreg(1, &j, &k);
            if (j != 0) {
                k = state[RA].regSP;
                if (k != 0)
                    emit(LD, k, RA);
                state[RA].regSP = j;
                regAlloc[sp]    = newReg(j, 1, 0, -1);
                state[j].regSP  = sp;
                emit(INP, reg, 0);
                return true;
            }
        }
        break;
    case B_OUTPUT:
        /* check for proper output port number */
        reg = regAlloc[sp].litv;
        if (0 <= reg && reg <= 255) {
            pop(1);
            /* now build an entry which can be recognized by */
            /* operat. */
            regAlloc[++sp] = newReg(0, 1, outloc, reg);
            return true;
        }
        break;
    case B_LENGTH:
    case B_LAST:
        j = regAlloc[sp].st;
        if (j > 0) {
            int rval = abs(symAttrib(j)) / 256 + B_LENGTH - val;
            pop(1);
            regAlloc[++sp] = newReg(0, rval > 255 ? 2 : 1, 0, rval);
            return true;
        }
        break;
    case B_MOVE: // move is explicitly expanded in pass 1
        break;
    case B_DOUBLE:
        if (regAlloc[sp].prec > 1) // already double
            return false;
        if (regAlloc[sp].assignment == 0) {
            if (regAlloc[sp].litv < 0) {
                loadv(sp, 1); // load value to accumulator and get a register
                state[RA].regSP = REGLOW(regAlloc[sp].assignment);
            } else {
                regAlloc[sp].prec = 2; // convert to 2 byte literal
                regAlloc[sp].st   = 0;
                return true;
            }
        }
        regAlloc[sp].prec = 2;
        regAlloc[sp].st   = 0;
        if (regAlloc[sp].assignment <= 15) { // only low register
            ia                      = regAlloc[sp].assignment;
            ib                      = ia - 1;
            state[ib].regSP         = sp;
            state[ia].lock          = false;
            regAlloc[sp].assignment = REGPAIR(ib, ia);
            /* ZERO the register */
            emit(LD, ib, 0);
            if (ib == 0)
                Fatal("133: register allocation stack overflow");
        }
        return true;
    case B_DEC:
        j = REGLOW(regAlloc[sp].assignment);
        if (j != 0)
            if (regAlloc[sp].prec == 1) {
                reg = state[RA].regSP;
                if (reg != j) { /* may be a pending register store */
                    if (reg != 0)
                        emit(LD, reg, RA);
                    emit(LD, RA, j);
                    state[RA].regSP = j;
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
    int reg, j, k, m, l, /* icy, iq,*/ iop = 0, iop2 = 0, ip, jp; // assign to appease GCC
    int jph, kp, id, ml, mh, ia, ib, il, lp;
    bool skip;

    /* ADD ADC SUB SBC MUL div mod NEG AND IOR */
    /* XOR NOT EQL LSS GTR NEQ LEQ GEQ INX TRA */
    /* TRC PRO RET STO STD XCH DEL cat LOD BIF */
    /* INC CSE END ENB ENP HAL RTL RTR SFL SFR */
    /* HIV LOV CVA ORG AX1 AX2 AX3 */
    // icy = 0;
    // iq  = 0;
    switch (val) {
    case INC: /* place a literal 1 at stack top AND apply ADD operator */
        regAlloc[++sp].litv = 1;

        if (regAlloc[sp - 1].prec ==
            1) { /* check for single byte increment, may be comparing with 255 */
            apply(AD, AC, true, true);
            lastin = codloc;
            return true;
        }
        // fallthrough
    case ADD:
        /* may do the ADD IN h AND l (using INX operator) */
        if (regAlloc[sp].prec != 1)
            exch();
        if (regAlloc[sp - 1].prec != 1)
            inx(1); // prec=1 for inx
        else {
            exch();
            apply(AD, AC, true, true);
        }
        return true;
    case ADC:
        apply(AC, AC, true, true);
        return true;
    case SUB: /* change address value - 1 to address value + 65535 AND apply ADD */
        if (regAlloc[sp - 1].prec != 1 && regAlloc[sp].litv == 1) {
            regAlloc[sp].litv = 65535;
            regAlloc[sp].prec = 2;
            inx(1);
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
    case NEG:
    case BIF:
    case END:
    case ENB:
    case ENP:
    case AX3:
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
        if (COND(regAlloc[sp].assignment)) /* condition code - change PARITY */
            regAlloc[sp].assignment ^= 0x1000;
        else { /* perform XOR with 255 OR 65535 (byte OR address) */
            int prec            = regAlloc[sp].prec;
            regAlloc[++sp].litv = prec == 1 ? 0xff : 0xffff;
            regAlloc[sp].prec   = prec;
            apply(XR, XR, true, false);
        }
        return true;
    case EQL: /* equal test */
        if (regAlloc[sp].prec + regAlloc[sp - 1].prec <= 2) {
            apply(SU, 0, true, false);
            regAlloc[sp].assignment += ZFLAG; /* mark as true/ZERO (1*16+2) */
        } else
            compare16(true, ZFLAG, true);
        return true;
    case GTR: /* GTR - change to LSS */
        exch();
        // FALLTHROUGH
    case LSS:                                                 /* LSS set to true/CARRY */
        if (regAlloc[sp].prec + regAlloc[sp - 1].prec <= 2) { // both bytes
            apply(regAlloc[sp].litv != 1 ? SU : CP, 0, false, false);
            regAlloc[sp].assignment += CFLAG; /* mark as true/CARRY code */
        } else
            compare16(false, CFLAG, false);
        return true;
    case NEQ:
        if (regAlloc[sp].prec + regAlloc[sp - 1].prec <= 2) { // both bytes
            apply(SU, 0, true, false);
            regAlloc[sp].assignment += NZFLAG; /* mark as false/ZERO */
        } else
            compare16(true, NZFLAG, true);
        return true;
    case LEQ: /* LEQ - change to GEQ */
        exch();
        // fallthrough
    case GEQ:
        if (regAlloc[sp].prec + regAlloc[sp - 1].prec <= 2) { // both bytes
            apply(regAlloc[sp].litv != 1 ? SU : CP, 0, false, false);
            regAlloc[sp].assignment += NCFLAG; /* mark as false/CARRY code */
        } else
            compare16(false, NCFLAG, false);
        return true;
    case INX:
        inx(regAlloc[sp - 1].prec);
        return true;
    case TRC:
        if (regAlloc[sp - 1].litv > 0 && (regAlloc[sp - 1].litv & 1)) {
            /* this is a do forever (OR something similar) so ignore the jump */
            pop(2);
            return true;
        }
        iop = 2; /* NOT a literal '1' */
        /* check for condition code */
        if (COND(regAlloc[sp - 1].assignment)) { /* active condition code, construct mask for JMC */
            int cond = COND(regAlloc[sp - 1].assignment);
            iop2     = (TRU - cond / 16) * 32 + (CARRY + cond % 16 - 1);
        } else {
            if (regAlloc[sp - 1].assignment == 0) { /* load value to accumulator */
                regAlloc[sp - 1].prec = 1;
                loadv(sp - 1, 1);
            } else if (REGLOW(regAlloc[sp - 1].assignment) !=
                       state[RA].regSP) { /* value already loaded */
                if (state[RA].regSP)
                    emit(LD, state[RA].regSP, RA);
                emit(LD, RA, REGLOW(regAlloc[sp - 1].assignment));
            }
            state[RA].regSP = 0;
            emit(ROT, CY, RGT);
            iop2 = FAL * 32 + CARRY;
        }
        break;
    case PRO:
        reg = regAlloc[sp].st;
        if (reg > intbas) { /* pass the last two (at most) parameters IN the registers */
            reg = right(regAlloc[sp].st, 16);
            reg = min(INFO_ECNT(symAttrib(reg)), 2);
            if (reg < 1) {
                state[RH].lock = state[RL].lock = true;
                saver();
                state[RH].lock = state[RL].lock = false;
                iop                             = 3;
            } else {
                j = sp - reg - reg;
                for (int k = 1; k <= reg; k++) {
                    ip = regAlloc[j].assignment & 0xf;
                    jp = (regAlloc[j].assignment >> 4) & 0xf;
                    if (ip != 0)
                        state[ip].lock = true;
                    if (jp != 0)
                        state[jp].lock = true;
                    regAlloc[j].prec = min(regAlloc[j].prec, regAlloc[j + 1].prec);
                    if (regAlloc[j].prec <= 1 && jp != 0) {
                        // bug fix from V4 -- clear pending store if passing address var to byte
                        // var
                        if (state[RA].regSP == jp)
                            state[RA].regSP = 0;
                        state[jp].regSP = state[jp].lock = false;
                        jp                               = 0;
                        if (state[RA].regSP == ip)
                            state[RA].lock = true;
                        if (state[RA].regSP == jp)
                            state[RA].lock = true;
                    }
                    regAlloc[j].assignment = REGPAIR(jp, ip);
                    j += 2;
                }
                j  = sp - 1 - 2 * reg;
                it = 0;
                /* stack any stuff which does NOT go to the procedure */
                for (int k = 1; k <= sp; k++) { /* check for value to PUSH */
                    if (regAlloc[k].assignment ==
                        0) { /* registers NOT assigned - check for stacked value */
                        if (regAlloc[k].st == 0 && regAlloc[k].litv < 0 && it != 0)
                            error("150: pass-2 compiler error in 'loadv'");
                    } else if (k <= j) { /* possible PUSH if NOT a parameter */
                                         /* registers must be pushed */
                        jph = REGHIGH(regAlloc[k].assignment);
                        kp  = state[RA].regSP;
                        jp  = REGLOW(regAlloc[k].assignment);
                        if (kp != 0) {       /* pending ACC store, check ho AND lo registers */
                            if (kp == jph) { /* pending ho byte store */
                                emit(LD, jph, RA);
                                state[RA].regSP = 0;
                            } else if (kp == jp) { /* check lo byte */
                                emit(LD, jp, RA);
                                state[RA].regSP = 0;
                            }
                        }
                        emit(PUSH, jp - 1, 0);
                        stack(1);
                        regAlloc[k].st = 0;
                        jp             = REGLOW(regAlloc[k].assignment);
                        if (jp != 0)
                            state[jp].regSP = 0;
                        jp = REGHIGH(regAlloc[k].assignment);
                        if (jp != 0)
                            state[jp].regSP = 0;
                        regAlloc[k].assignment = 0;
                        regAlloc[k].litv       = -1;
                        it                     = k;
                    }
                }
                it = RH;
                j  = sp - reg - reg;
                for (int k = 1; k <= reg; k++) {
                    id = 2 * k + 2;
                    jp = REGHIGH(regAlloc[j].assignment);
                    ip = REGLOW(regAlloc[j].assignment);
                    for (;;) {
                        id--;
                        if (ip == 0)
                            break;
                        if (ip != id) {
                            if (state[id].regSP != 0) {
                                m  = state[id].regSP;
                                mh = REGHIGH(regAlloc[m].assignment);
                                ml = REGLOW(regAlloc[m].assignment);
                                if (ml == id)
                                    ml = it;
                                if (mh == id)
                                    mh = it;
                                // Bug fix from V4 -- CLEAR PENDING STORE WHEN REG PAIRS ARE TO
                                // BE EXCHANGED ***
                                if (state[RA].regSP == id) {
                                    emit(LD, it, RA);
                                    state[RA].regSP = state[RA].lock = false;
                                } else
                                    // end of change
                                    emit(LD, it, id);
                                state[it].regSP        = m;
                                regAlloc[m].assignment = REGPAIR(mh, ml);
                                it++;
                            }
                            state[ip].regSP = state[ip].lock = false;
                            if (state[RA].regSP == ip) {
                                ip              = 1;
                                state[RA].regSP = state[RA].lock = false;
                            }
                            emit(LD, id, ip);
                            state[id].regSP = j;
                        }
                        state[id].lock = true;
                        ip             = jp;
                        if (ip == (-1))
                            break;
                        jp = -1;
                    }
                    j += 2;
                }
                j = sp - 2 * reg;
                for (int k = 1; k <= reg; k++) {
                    if (regAlloc[j].assignment == 0)
                        loadv(j, 0);
                    ip              = 2 * k;
                    state[ip].regSP = j;
                    state[ip].lock  = true;
                    if (regAlloc[j + 1].prec == 2 && regAlloc[j].prec == 1)
                        emit(LD, ip, 0);
                    j += 2;
                }
                if (state[RA].regSP != 0)
                    emit(LD, state[RA].regSP, RA);
                for (int k = 1; k <= 7; k++) {
                    state[k].regSP = state[k].lock = false;
                    state[k].regValue              = -1;
                }
                j = 2 * reg;
                for (int k = 1; k <= j; k++) {
                    exch();
                    if (regAlloc[sp].st == 0 && regAlloc[sp].assignment == 0 &&
                        regAlloc[sp].litv < 0) {
                        emit(POP, RH, 0);
                        ustack();
                        state[RH].regValue = state[RL].regValue = -1;
                    }
                    pop(1);
                }
                iop = 3;
            }
            break;
        } else                                  /* this is a built-IN function. */
            return doBuiltin(reg - firsti + 1); // pass function number
        break;
    case RET:
        jp = prsp;
        if (jp <= 0) {
            error("146: procedure optimization stack underflow");
            state[RH].regValue = state[RL].regValue = -255; /* mark as nil */
            return true;
        }
        // V4 /* check for type AND precision of procedure */
        l = LOWWORD(prstk[jp]) + 2;
        l = symbol[l] / 16 % 16;

        /* l is the precision of the procedure */
        if (l != 0) {
            reg = regAlloc[sp].assignment;
            if (reg == 0)
                loadv(sp, 1);
            else if (reg >= 256)
                cvcond(sp);
            jp = state[RA].regSP;
            j  = REGLOW(regAlloc[sp].assignment);
            k  = REGHIGH(regAlloc[sp].assignment);
            if (reg != 0 && j != jp) { /* have to load the accumulator.  may have h.o. byte. */
                if (jp != 0 && jp == k)
                    emit(LD, k, RA);
                emit(LD, RA, j);
            }
            if (k != 0 && k != RB)
                emit(LD, RB, k);
            if (l > regAlloc[sp].prec) /* compare precision of procedure with stack */
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
        /* STO AND STD */
        reg = regAlloc[sp].st;

        /* check for output function */
        if (reg != outloc) { /* check for computed address OR saved address */
            if (reg < 0) {   /* check for address reference outside intrinsic range */
                reg = -reg;
                if (reg <= intbas) /* check for 'memory' address reference */
                                   /* ** note that stacktop must be at 6 ** */
                    if (reg > 6) {
                        if (val == STD)
                            pop(1);
                        return true;
                    }
            }
            gensto(val != STD);
        } else {
            j   = regAlloc[sp].litv;
            reg = regAlloc[sp - 1].assignment;
            if (reg <= 0 || reg >= 256) { /* load value to ACC */
                reg = state[RA].regSP;
                if (reg > 0)
                    emit(LD, reg, RA);
                loadv(sp - 1, 1);
                reg = regAlloc[sp - 1].assignment;
            } else { /* operand is IN the gprs */
                reg %= 16;
                k = state[RA].regSP;
                if (k > 0 && k != reg)
                    emit(LD, k, RA);
                if (k != reg)
                    emit(LD, RA, reg);
            }
            /* now mark ACC active IN case subsequent STO operator */
            state[RA].regSP = reg % 16;
            emit(OUT, j, 0);
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
        if (regAlloc[sp].st == 0 && regAlloc[sp].assignment == 0 && regAlloc[sp].litv < 0) {
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
        il   = 0;
        skip = false;
        k    = regAlloc[sp].prec;

        /* may be a LOD from a base for a based variable */
        regAlloc[sp].prec = k % 4;
        ia                = regAlloc[sp].assignment;
        if (ia <= 0) {
            /* check for simple based variable case */
            reg = regAlloc[sp].st;
            if (reg > 0) { /* reserve registers for the result */
                genreg(2, &ia, &ib);
                state[ia].regSP         = sp;
                state[ib].regSP         = sp;
                regAlloc[sp].assignment = REGPAIR(ib, ia);
                /* may be able to simplify LHLD */
                lp = state[RH].regValue;
                l  = state[RL].regValue;
                if (lp != -3 || (-l) != reg) {
                    if (lp == (-4) && (-l) == reg) {
                        emit(DCX, RH, 0);
                        state[RH].regValue = -3;
                    } else {
                        j = chain(reg, codloc + 1);
                        emit(LHLD, j, 0);
                        state[RH].regValue = -3;
                        state[RL].regValue = -reg;
                    }
                }
                skip = true;
            } else if (regAlloc[sp].st == 0) { /* first check for an address reference */
                loadv(sp, 0);
                ia = regAlloc[sp].assignment;
            } else { /* change the address reference to a value reference */
                regAlloc[sp].st   = -regAlloc[sp].st;
                regAlloc[sp].litv = -1;
                return true;
            }
        }
        if (!skip) {
            ib  = REGHIGH(ia);
            ia  = REGLOW(ia);
            reg = state[RA].regSP;
            if (ia == reg)
                ia = 1;
            if (ib == reg)
                ib = 1;
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
            } else if (lastex == codloc - 1 || regAlloc[sp].prec % 2 != 1) {
                /* double XCHG OR double byte load with addr IN d AND e */
                emit(XCHG, 0, 0);
                il                 = 0;
                state[RH].regValue = state[RL].regValue = -1;
            }
        }

        reg               = regAlloc[sp].prec - k / 4;
        regAlloc[sp].prec = reg;

        /* recover the register assignment from rasn */
        ia = REGLOW(regAlloc[sp].assignment);
        ib = REGHIGH(regAlloc[sp].assignment);
        j  = state[RA].regSP;

        /* skip if j=0, ia, OR ib */
        if (j != 0 && j != ia && j != ib)
            emit(LD, j, RA);

        /* may be able to change register assignment to bc */
        if (ia == RE)
            if (state[RB].regSP == 0 && state[RC].regSP == 0) { /* bc available, so RE-assign */
                state[ia].regSP = state[ib].regSP = 0;
                state[RB].regSP = state[RC].regSP = sp;
                ia                                = RC;
                ib                                = RB;
                regAlloc[sp].assignment           = REGPAIR(RB, RC);
            }
        state[RA].regSP = ia;
        if (il == 0)
            emit(LD, RA, ME);
        else
            emit(LDAX, il, 0);
        if (reg > 1) {
            emit(INCX, RH, 0);
            /* may have done a prevous LHLD, if so mark INCX h */
            if (state[RH].regValue == -3)
                state[RH].regValue = -4;
            emit(LD, ib, ME);
        } else { /* single byte load - release h.o. register */
            ib = REGHIGH(regAlloc[sp].assignment);
            if (ib == state[RA].regSP)
                state[RA].regSP = 0;
            state[ib].regSP         = 0;
            state[ib].regValue      = -1;
            regAlloc[sp].assignment = REGLOW(regAlloc[sp].assignment);
        }
        state[RH].regSP = state[RL].regSP = regAlloc[sp].st = 0;
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
    case RTL:
    case RTR:
    case SFL: // not used
    case SFR:
    case HIV:
    case LOV:
        unary(val - 36);
        return true;
    case CVA:
        /* CVA must be immediately preceded by an INX OR ADR ref */
        regAlloc[sp].prec = 2;

        /* if the address is already IN the gpr's then nothing to do */
        if (regAlloc[sp].assignment > 0)
            return true;
        if (regAlloc[sp].st < 0) { /* check for address ref to data IN rom. */
            jp = regAlloc[sp].litv;
            if (jp <= 65535) {
                if (jp < 0)
                    error("149: pass-2 compiler error. Attempt to convert invalid value to "
                          "address type");
                /* leave literal value */
                regAlloc[sp].st = 0;
                return true;
            } /* do LXI r with the address */
            genreg(2, &ia, &ib);
            if (ia <= 0) {
                Fatal("140: register allocation error");
                return false;
            } else {
                j = chain(-regAlloc[sp].st, codloc + 1);
                emit(LXI, ib, j);
                regAlloc[sp].st         = 0;
                regAlloc[sp].assignment = REGPAIR(ib, ia);
                state[ia].regSP         = sp;
                state[ib].regSP         = sp;
                return true;
            }
        } else if (regAlloc[sp].st > 0) {
            /* load value of base for address ref to a based variable */
            loadv(sp, 3);
            return true;
        } else {
            error("139: error in changing variable to address reference");
            return false;
        }
        break; // not needed no path to here
    case ORG:
        reg = regAlloc[sp].litv;
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
        jp = prsp;
        if (prstk[jp] > 65535) /* this is the END of an interrupt procedure */
            curdep[jp] -= 4;
        if (prsp > 0)
            prsp--;

        /* get stack depth for symbol table */
        if (jp > 0) {
            if (curdep[jp] != 0)
                error("150: stack not empty at end of compilation");
            k = maxdep[jp];
            l = prstk[jp] % 65536 - 1;

            /* k is max stack depth, l is symbol table count entry */
            symbol[l] = k;
        }
        k = state[RH].regValue;
        l = state[RL].regValue;
        if (k == (-255) && l == (-255))
            return false;
        if (prstk[jp] > 65535) { /* POP interrupted registers AND enable interrupts */
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
        state[RH].lock = state[RL].lock = true; /* IN case there are any pending values ... */
        saver();
        state[RH].lock = state[RL].lock = false;
        m                               = regAlloc[sp].litv;
        if (m >= 0) { /* absolute jump - probably to assembly language subrtne... */

            state[RH].regValue = state[RL].regValue = -1; /* ...so make h AND l registers unknown */
            emit(JMP, m, 0);
            pop(1);
            return true;
        }
        break;
    case AX2: /* may NOT be omitted even though no obvious path exists). */
        iop = 4;
        /* casjmp points to symbol table attributes - INC len field */
        symbol[casjmp] += 256;
        break;
    }

    reg = regAlloc[sp].st;
    if (reg > 0) {
        j = INFO_TYPE(symAttrib(reg));
        /* may be a simple variable */
        if (iop != 1 || j != VARB) {
            if ((iop != 3 || j != PROC) && j != LABEL) {
                error("135: invalid program transfer (only computed jumps are allowed with a "
                      "'go to')");
                sp--;
                return true;
            } else {
                j = -symAddr(reg);
                m = symRef(reg);

                if (iop == 1) {
                    it = INFO_PREC(abs(symAttrib(reg)));

                    /* it is type of LABEL... */
                    /* 3 is user-defined outer block, 4 is user defined */
                    /* NOT outer block, 5 is compiler defined */
                    if (it == 5 && defsym > 0 && INFO_PREC(symAttrib(defsym)) == 5) {
                        /* this TRA is one of a chain of compiler generated */
                        /* TRA's - straighten the chain if no code has been */
                        /* generated since the previous DEF. */

                        l  = -symAddr(defsym);
                        jp = symRef(defsym);
                        if (jp == codloc) {
                            /* adjust the reference counts AND optimization */
                            /* information for both DEF's. */
                            ia = INFO_ECNT(abs(symAttrib(defsym)));
                            ib = ia == 1 ? symTrackHL(defsym) : 0;

                            if (defrh == -255)
                                ia--;
                            symAttrib(defsym) = 84;

                            /* i.e., ZERO references to compiler generated LABEL */
                            if (INFO_ECNT(abs(symAttrib(reg))) == 1)
                                symTrackHL(reg) = ib;

                            symAttrib(reg) += ia * 256;
                            for (;;) {
                                /* corrected reference count for object of the DEF */
                                /* merge the backstuffing chains */

                                if ((ia = l >> 2) == 0) {
                                    /* equate the defs */
                                    for (ia = 1; ia <= sytop; ia++)
                                        if (symbol[ia] == symbol[defsym])
                                            symbol[ia] = symbol[reg];

                                    /* omit the TRA if no path to it */
                                    state[RH].regValue = defrh;
                                    state[RL].regValue = defrl;
                                    break;
                                } else {
                                    ib = getword(ia);
                                    // V4
                                    l               = (ib << 2) + (l & 3);
                                    symAddr(defsym) = -l;
                                    // V4
                                    putword(ia, j >> 2);
                                    // V4
                                    j            = ((ia << 2) + (j & 3));
                                    symAddr(reg) = -j;
                                }
                            }
                        }
                        if (state[RH].regValue == (-255)) {
                            pop(1);
                            return true;
                        }
                    }
                }

                if (it == 3 && iop == 1) { // BUG FIX user labels all have 4
                    /* we have a TRA to the outer block... */
                    j = C_STACKHANDLING;
                    if (prsp != 0 && j != 1) {
                        if (j == 0) {
                            j    = lxis;
                            lxis = codloc + 1;
                        }
                        emit(LXI, RSP, j % 65536);
                    }
                }
                j = -symAddr(reg);
                m = j / 4;

                /* connect entry into chain */
                k = codloc + 1;
                if (iop == 4)
                    k = codloc;

                /* iop = 4 if we arrived here from case table JMP */
                // V4
                symAddr(reg) = -((k << 2) + right(j, 2));

                /* check for single reference */
                if (INFO_ECNT(abs(symAttrib(reg))) == 1) {
                    // V4
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

                    int lsym = symTrackHL(reg);

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
                        symTrackHL(reg) = ktotal & (HVALID | LVALID) ? ktotal : -1;
                    }
                }

                /* TRA, TRC, PRO, AX2 (case TRA) */
                switch (iop) {
                case 1:

                    /* may be INC TRA combination IN do-loop */
                    if (lastin + 1 == codloc) {
                        /* change to jfz to top of loop */
                        emit(JMC, FAL * 32 + ZERO, m);
                        pop(1);
                        return true;
                    } else {
                        xfrloc = codloc;
                        xfrsym = regAlloc[sp].st;
                        tstloc = codloc + 3;
                        emit(JMP, m, 0);
                        pop(1);
                        state[RH].regValue = state[RL].regValue = -255; /* mark as nil */
                        return true;
                    }
                    break;
                case 2:
                    conloc = codloc;
                    emit(JMC, iop2, m);
                    pop(2);
                    return true;
                    ;
                case 3:
                    xfrloc = codloc;
                    xfrsym = regAlloc[sp].st;
                    tstloc = codloc + 3;
                    emit(CAL, m, 0);

                    /* adjust the maxdepth, if necessary */
                    // V4
                    j = symIProcDepth(reg) + 1;
                    /* j is number of double-byte stack elements reqd */
                    stack(j);

                    /* now returned from call so... */
                    curdep[prsp] -= j;

                    /* now fix the h AND l values upon return */
                    // V4
                    j = symTrackHL(reg);
                    if (j < 0)
                        j = 0; // will force regv values to -1
                    /* may be unchanged from call */
                    if ((j >> 19) != 3) {
                        /* compare values */
                        state[RL].regValue = (j & LVALID) ? j & 0xff : -1;
                        state[RH].regValue = (j & HVALID) ? (j >> 8) & 0x1ff : -1;
                    }
                    pop(1);

                    /* may have to construct a returned */
                    /* value at the stack top */
                    j = INFO_PREC(symAttrib(reg));
                    if (j > 0) {
                        /* set stack top to precision of procedure */
                        regAlloc[++sp]  = newReg(j > 1 ? REGPAIR(RB, RC) : RC, j, 0, -1);
                        state[RA].regSP = RC;
                        state[RC].regSP = sp;
                        if (j > 1)
                            state[RB].regSP = sp;
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
    } else if (iop != 1 || reg != 0) { /* could be a computed address */
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
    int reg  = prstk[jp] % 65536;
    int dsym = state[RH].regValue;
    int l    = state[RL].regValue;
    int j    = symbol[reg]; // masking not needed as implicit in code below
    int lp, kp;

    alter = 1; // hoisted here to simplify return
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
    int regLow  = REGLOW(regAlloc[sp].assignment);
    int regHigh = REGHIGH(regAlloc[sp].assignment);
    if (zeroTest)
        emit(OR, regLow, 0);

    /* get rid of high order register in the result */
    state[RA].regSP = regLow;
    regAlloc[sp]    = newReg(flag + regLow, 1, 0, -1);

    if (regHigh) { // free high register if previously used
        state[regHigh].lock     = false;
        state[regHigh].regSP    = 0;
        state[regHigh].regValue = -1;
    }
    return;
}

// genrate call to builtin function bf, result determines result reg pair to use
// modified to use bf = 0->mutliply 1->divide/mod
void builtin(int bf, int targetReg) {
    /* clear condition code */
    if (COND(regAlloc[sp].assignment))
        cvcond(sp);

    /* clear pending store */
    if (state[RA].regSP != 0) {
        emit(LD, state[RA].regSP, RA);
        state[RA].regSP = 0;
    }

    /* lock any correctly assigned registers */
    /* ....AND store the remaining registers. */
    if (REGLOW(regAlloc[sp].assignment) == RE)
        state[RE].lock = true;
    if (REGHIGH(regAlloc[sp].assignment) == RD)
        state[RD].lock = true;
    if (REGLOW(regAlloc[sp - 1].assignment) == RC)
        state[RC].lock = true;
    if (REGHIGH(regAlloc[sp - 1].assignment) == RB)
        state[RB].lock = true;
    saver();

    /* mark register c used. */
    if (state[RC].regSP == 0)
        state[RC].regSP = -1;

    /* load top of stack into registers d AND e. */
    loadv(sp, 0);
    if (regAlloc[sp].prec == 1)
        emit(LD, RD, 0);

    /* now deassign register c unless correctly loaded. */
    if (state[RC].regSP == (-1))
        state[RC].regSP = 0;

    /* load t.o.s. - 1 into registers b AND c. */
    loadv(sp - 1, 0);
    if (regAlloc[sp - 1].prec == 1)
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
    regAlloc[++sp]         = newReg(REGPAIR(targetReg, targetReg + 1), 2, 0, -1);
    state[targetReg].regSP = state[targetReg + 1].regSP = sp;
}

void inx(int prec) {

    if (regAlloc[sp].litv == 0) /* zero index just pop it and ignore the INX operator */
        pop(1);
    else {
        if (COND(regAlloc[sp].assignment))
            cvcond(sp);
        int j         = state[RA].regSP;
        int indexLow  = REGLOW(regAlloc[sp].assignment);
        int indexHigh = REGHIGH(regAlloc[sp].assignment);
        int baseLow   = REGLOW(regAlloc[sp - 1].assignment);
        int baseHigh  = REGHIGH(regAlloc[sp - 1].assignment);

        /* check for pending store to base or index */
        if (j && (j == baseHigh || j == baseLow || j == indexHigh || j == indexLow)) {
            emit(LD, j, RA);
            state[RA].regSP = 0;
        }

        /* make sure that d and e are available */
        if (state[RE].regSP != 0 || state[RD].regSP != 0)
            if (indexLow != RE && baseLow != RE) { /* mark all registers free */
                int ia, ib, ic;
                if (indexLow)
                    state[indexLow].regSP = 0;
                if (baseLow)
                    state[baseLow].regSP = 0;
                genreg(2, &ia, &ib);
                state[ia].regSP = 1; // mark as not free so as to not reallocate
                genreg(2, &ic, &ib);
                state[ia].regSP = 0; // back to free

                /* all regs are cleared except base and index, if allocated. */
                if (indexLow)
                    state[indexLow].regSP = sp;
                if (baseLow)
                    state[baseLow].regSP = sp - 1;
            }

        /* if not literal 1 or -1, use DAD else use INX or DCX */
        int op;
        if (regAlloc[sp].litv != 1 && regAlloc[sp].litv != 0xffff) {
            /* if the index is constant, and the base an address variable, */
            /* double the literal value at compile time */
            if (regAlloc[sp].litv >= 0 && prec != 1) {
                regAlloc[sp].litv *= 2;
                prec = 1; // pretend byte variable (saves a DAD)
            }
            loadv(sp, regAlloc[sp].litv >= 0 ? 3 : 0);

            //  if the index was already in the registers, may have to extend precision to address.
            indexLow  = REGLOW(regAlloc[sp].assignment);
            indexHigh = REGHIGH(regAlloc[sp].assignment);
            if (indexLow != 0 && indexHigh == 0) {
                indexHigh = indexLow - 1;
                emit(LD, indexHigh, 0);
            }
            op = DAD;
        } else {
            op        = regAlloc[sp].litv == 1 ? INCX : DCX;
            indexHigh = RH;
        }

        pop(1);                 /* pop the index.  base is now at top */
        loadv(sp, 5);           /* load the base into the h and l registers */
        emit(op, indexHigh, 0); /* add the base and index */

        if (prec != 1)
            emit(op, indexHigh, 0); /* and add index again if base is an address variable. */
        emit(XCHG, 0, 0);           /* XCHG here and remove with peephole optimization later */
        prec = regAlloc[sp].prec;   // get back precision of base
        pop(1);
        regAlloc[++sp]     = newReg(REGPAIR(RD, RE), prec, 0, -1);
        state[RH].regValue = state[RL].regValue = -1;
        state[RD].regSP = state[RE].regSP = sp;
    }
}
