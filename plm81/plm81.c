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
      the pl/m compiler is intended to be written in ansi standard
      fortran - iv, and thus it should be possible to compile and
      execute this program on any machine which supports this fortran
      standard.  both pass-1 and pass-2, however, assume the host
      machine word size is at least 31 bits, excluding the sign bit
      (i.e., 32 bits if the sign is included).

      the implementor may find it necessary to change the source program
      in order to account for system dependencies.  these changes are
      as follows

      1)   the fortran logical unit numbers for various devices
           may have to be changed in the 'gnc' and 'writel' subrou-
           tines (see the file definitions below).

       2)   the host machine may not have the pl/m 52 character set
             0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$=./()+-'*,<>:;
           (the last 15 special characters are
           dollar,  equal,  period,  slash, left paren,
           right paren, plus,   minus,  quote, asterisk,
           comma, less-than, greater-than, colon, semi-colon)
           in this case: case  it is necessary to change the 'otran' vector in
           block data to a character set which the host machine supports

       3)   the computed go to in 'synth' may be too long for some
           compilers.  if you get a compilation error, break the
           'go to' into two sections.

       4)  the host fortran system may have a limitation on the number
           of contiguous comment records (e.g. s/360 level g). if so,
           intersperse the declaration statements integer i1000, integer
           i1001, etc., as necessary to break up the length of comments.
           the symbols i1xxx are reserved for this purpose.

      there are a number of compiler parameters which may have to
      be changed for your installation.  these parameters are defined
      below (see 'scanner commands'), and the corresponding default
      values are set following their definition.  for example, the
                    $rightmargin = i
      parameter determines the right margin of the input source line.
      the parameter is set externally by a single line starting with
      '$r' in columns one and two (the remaining characters up to
      the '=' are ignored).  the internal compiler representation
      of the character 'r' is 29 (see character codes below), and thus
      the value of the $rightmargin parameter corresponds to element 29
      of the 'contrl' vector.

       1)  the parameters $t, $p, $w, $i, $o, and $r
          control the operating mode of pl/m.  for batch processing,
          assuming 132 character (or larger) print line and 80 charac-
          ter card image, the parameters should default as follows
                  $terminal   =  0
                  $print      =  1
                  $width      = 132
                  $input      =  2
                  $output     =  2
                  $rightmargin= 80
          note that it may be desirable to leave $r=72 to allow room
          for an 8-digit sequence number in columns 73-80 of the pl/m
          source card.

      2)  for interactive processing, assuming a console with width
          of 72 characters (e.g., a tty), these parameters should
          default as follows
                  $terminal   =  1
                  $print      =  1
                  $width      = 72
                  $input      =  1
                  $output     =  1
                  $rightmargin= 72

      3)  the characteristics of the intermediate language files
          produced by pass-1 are governed by the $j, $k, $u, $v, and
          $y parameters.  these parameters correspond to the destination
          and width of the intermediate code file ($j and $k), and
          destination and width of the intermediate symbol table ($u
          and $v).  some fortran systems delete the leading character
          of the files produced by other fortran programs.  the $y
          parameter can be used to pad extra blanks at the beginning of
          the intermediate files if this becomes a problem on the host
          system.

          under normal circumstances, these parameters will not
          have to be changed.  in any case: case  experiment with various
          values of the $ parameters by setting them externally be-
          fore actually changing the defaults.

      the implementor may also wish to increase or decrease the size
      of pass-1 or pass-2 tables.  the tables in pass-1 which may be
      changed in size are 'macros' and 'symbol' which correspond to
      the areas which hold 'literally' definitions and program symbols
      and attributes, respectively.  it is impossible to provide an
      exact formula which relates the number of symbols held by either
      of these tables to the table length, since table space is dy-
      namically allocated according to symbol name length and number
      of attributes required for the particular symbol.

      1)  in the case of the macros table: case  the length is related to the
          total number of characters in the macro names plus the total
          number of characters in the macro definitions - at the deep-
          est block level during compilation.  to change the macro
          table size, alter all occurrences of

                           macros(3000)

          in each subroutine to macros(n), where n represents the new
          integer constant size.  in addition, the 'data' statement
          block data (last program segment) must be changed for the
          macro parameters based upon the constant value n to

             data macros /n*0/, curmac /n+1/, maxmac /n/,
            1    mactop /1/

      the macros table size (n above) must never exceed 4094

      2)  if the implementor wishes to increase or decrease the size
          of the symbol table, then all occurrences of

                            symbol(6000)

          must be changed to symbol(m), where m is the desired integer
          constant size.  the 'data' statements for symbol table para-
          meters must also be altered as described in the corresponding
          comment in block data.  in particular, the last item  of
          the data statement for 'symbol' fills the uninitialized por-
          tion of the table with zeroes, and hence must be the evaluation
          of the element
                             (m-120)*0

          (it is currently (6000-120)*0 = 5880*0).  the data statement
          for maxsym and symabs must be changed to initialize these
          variables to the value m.  (In no case should m be made
          greated than 32000.)

      good luck...


     all input records are 120 characters or less.  all
     output records are 132 characters or less.
     the fortran unit numbers can be changed in the
     subroutines gnc, cmptm and writel (these are the only oc-
     currences of references to these units).

      0 1 2 3 4 5 6 7 8 9
      0 0 0 0 0 0 0 0 1 1
      2 3 4 5 6 7 8 9 0 1

      $ = . / ( ) + - ' * , < > : ;
      3 3 4 4 4 4 4 4 4 4 4 4 5 5 5
      8 9 0 1 2 3 4 5 6 7 8 9 0 1 2

      A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
      1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
      2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7

    seqno              sub/func name
    15410000      subroutine exitb
    16300000      integer function lookup(iv)
    17270000      integer function enter(infov)
    18050000      subroutine dumpsy
    20030000      subroutine recov
    20420000      logical function stack(q)
    20930000      logical function prok(prd)
    21550000      subroutine reduce
    22100000      subroutine cloop
    22740000      subroutine prsym(cc,sym)
    23120000      integer function getc1(i,j)
    23330000      subroutine scan
    25280000      integer function wrdata(sy)
    26460000      subroutine dumpch
    26960000      subroutine synth(prod,sym)
    36310000      integer function gnc(q)
    37980000      subroutine writel(nspace)
    38520000      function icon(i)        no longer used
    38710000      subroutine decibp
    38850000      subroutine conv(prec)
    39020000      function imin(i,j)
    39090000      subroutine form(cc,chars,start,finish)
    39370000      subroutine conout(cc,k,n,base)
    39690000      subroutine pad(cc,chr,i)
    39800000      subroutine stackc(i)
    39950000      subroutine enterb
    40180000      subroutine dumpin
    40880000      subroutine error(i,level)
    41320000      integer function shr(i,j)
    41360000      integer function shl(i,j)
    41400000      integer function right(i,j)
    41440000      subroutine sdump
    41670000      subroutine redpr(prod,sym)
    41900000      subroutine emit(val,typ)
*/
/*************************************************************************/
#include "istring.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _MSC_VER
#include <io.h>
#endif
#include <showVersion.h>

#ifdef _WIN32
#define DIRSEP "/\\:"
#else
#include <limits.h>
#define _MAX_PATH PATH_MAX
#define DIRSEP    "/"
#endif
#define LOWBYTE(n)    ((n) & 0xff)
#define HIGHBYTE(n)   LOWBYTE((n) >> 8)
#define LOWNIBBLE(n)  ((n) & 0x0f)
#define HIGHNIBBLE(n) LOWNIBBLE((n) >> 4)

FILE *files[20];

#define ASIZE(n) (sizeof(n) / sizeof(n[0]))

#define ZPAD     0
/* stacks */
#define MSTACK   75
#define MVAR     1024
int sp;
int mp = 0;
// int mpp1 = 1;
int vartop = 1;
uint8_t pstack[MSTACK + 1];
struct {
    int loc : 12, len : 12;
} var[MSTACK + 1];
// int var[MSTACK + 1];
int varc[MVAR + 1];
int fixv[MSTACK + 1];
int fixc[MSTACK + 1];
// int prmask[5+1];
bool failsf = false;
bool compil = true;

/*
 * The original code for inline data stored definitions of online from top of symbol array downwards
 * to allow for symbol to be replaced by a structure a separate array is used
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

/* sym */
/*     the '48' used in block initialization and in symbol table*/
/*     initialization is derived from the program 'symcs' which*/
/*     builds the initial symbol table.  if this number changes, be*/
/*     sure to alter 'block', 'blksym', 'symtop', and 'symcnt'.*/
/*     two arrays, sym1 and sym2, are equivalenced over the*/
/*     symbol table array in order to limit the number of*/
/*     continuation cards in symbol table initialization*/
/*     below.  the lengths of sym1 and sym2, therefore, must*/
/*     total the length of the symbol table.  currently, these*/
/*     arrays are declared as follows*/

/*         sym1(60) + sym2(5940) = symbol(6000)*/

/*     if you increase (decrease) the size of symbol, you must*/
/*     increase (decrease) the size of sym2 as well.*/

/*     note also that the remaining entries of the symbol*/
/*     table are set to zero at the end of the data statement*/
/*     for sym2.  currently, this is accomplished with the last*/
/*     entry in the data statement*/

/*                   5880*0*/

/*     again, if you change the size of symbol, you must*/
/*     also change this last entry.  if for example, you alter*/
/*     the size of symbol to 3000, the last entry 1880*0 becomes*/

/*                   2880*0*/
#define SYMABS 10000

/*
Symbol format
    i - 2 (opt) => hash << 16 + index to next symbol in chain
    i - 1 => symbolNo << 16 + index ptr to next symbol
    i => # ints used for symbol << 12 + symbol length    (negated when not in scope)
    i+1 => info => length << 8 + prec << 4 + type    (negated for based)
    i+2... => symbol ints

  prec = 3 if user-defined outer block LABEL
  prec = 4 if user-defined LABEL not in outer block
  prec = 5 if compiler-generated LABEL

*/
enum { P_LABEL, P_BYTE, P_ADDRESS, P_INLINE };      // Data prec values
enum { OuterLabel = 3, LocalLabel, CompilerLabel }; // LABEL prec values

/*
Symbol table below initial entries are shown below
Note the index refers to the location of the symbol no information - see above
# index    symbol     hash      value   prec    type
1    2    CARRY         83      0       1       1
2    7    ZERO          104     0       1       1
4    17   PARITY        114     0       1       1
5    23   MEMORY        122     0       1       1
6    29   STACKPTR      13      0       2       1
7    35   ROL           59      2       1       2
8    40   ROR           124     2       1       2
9    45   SHL           82      2       1       2
10   50   SHR           20      2       1       2
11   55   SCL           2       2       1       2
12   60   SCR           67      2       1       2
13   65   TIME          82      1       0       2    chains to SHL
14   70   HIGH          113     1       1       2
15   75   LOW           109     1       1       2
16   80   INPUT         81      1       1       2
17   85   OUTPUT        50      1       1       2
18   91   LENGTH        17      1       1       2
19   97   LAST          26      1       1       2
20   102  MOVE          29      3       0       2
21   107  DOUBLE        9       1       2       2
22   113  DEC           7       1       1       2
3    12   SIGN          77      0       1       1
*/

/* types */
enum { VARB = 1, INTR, PROC, LABEL, LITER, NUMBER };

#define MkInfo(packed, prec, type) (((int)(packed) << 8) | ((prec) << 4) | (type))
struct {
    char *name;
    int32_t info;
} builtins[] = {
    { "CARRY", MkInfo(0, 1, VARB) },  { "ZERO", MkInfo(0, 1, VARB) },
    { "SIGN", MkInfo(0, 1, VARB) },   { "PARITY", MkInfo(0, 1, VARB) },
    { "MEMORY", MkInfo(0, 1, VARB) }, { "STACKPTR", MkInfo(0, 2, VARB) },
    { "ROL", MkInfo(2, 1, INTR) },    { "ROR", MkInfo(2, 1, INTR) },
    { "SHL", MkInfo(2, 1, INTR) },    { "SHR", MkInfo(2, 1, INTR) },
    { "SCL", MkInfo(2, 1, INTR) },    { "SCR", MkInfo(2, 1, INTR) },
    { "TIME", MkInfo(1, 0, INTR) },   { "HIGH", MkInfo(1, 1, INTR) },
    { "LOW", MkInfo(1, 1, INTR) },    { "INPUT", MkInfo(1, 1, INTR) },
    { "OUTPUT", MkInfo(1, 1, INTR) }, { "LENGTH", MkInfo(1, 1, INTR) },
    { "LAST", MkInfo(1, 1, INTR) },   { "MOVE", MkInfo(3, 0, INTR) },
    { "DOUBLE", MkInfo(1, 2, INTR) }, { "DEC", MkInfo(1, 1, INTR) },
    { "", -MkInfo(0, 0, VARB) } // keeps block tracking aligned with original

};

int symbol[SYMABS + 1];
int symtop = 1;
int symcnt = 0;

/* in anticipation of symbols being converted into an array of structure the use of
   the top of the symbol table as an assignment list is factored out */
#define MAXASSIGN 256      /* max number of assignments in a block */
int assign[MAXASSIGN + 1]; /* assignment symbol indexes */

/* some inline definitions to help make the code more readable */
#define SymInts(symIdx) ((symIdx) + 2)
#define Info(symIdx)    ((symIdx) + 1) /* index to symbol info word */
#define Sym(symIdx)     ((symIdx))     /* index to symbol sizing word */
#define Id(symIdx)      ((symIdx) - 1) /* index to symbol id word */
#define Hash(symIdx)    ((symIdx) - 2) /* index to symbol hash word */

inline unsigned iabs(int a) {
    return a < 0 ? -a : a;
}

inline int id_num(int i) {
    return symbol[Id(i)] >> 16;
}

inline int id_next(int i) {
    return symbol[Id(i)] & 0xffff;
}
inline int info_len(int i) {
    return iabs(symbol[Info(i)]) >> 8;
}
inline int info_prec(int i) {
    return (iabs(symbol[Info(i)]) >> 4) & 0xf;
}
inline int info_type(int i) {
    return iabs(symbol[Info(i)]) & 0xf;
}

inline int hash_hcode(int i) {
    return symbol[Hash(i)] >> 16; /* hash value */
}

inline int hash_next(int i) {
    return symbol[Hash(i)] & 0xffff; /* next symbol in chain */
}

inline sym_len(int i) {
    return iabs(symbol[Sym(i)]) & 0xfff;
}

inline sym_ints(int i) {
    return iabs(symbol[Sym(i)]) >> 12; /* number of ints used for symbol */
}

inline int symHash(int i) {
    return i >> 16;
}
inline int symChainNext(int i) {
    return i & 0xffff;
}
inline int symId(int i) {
    return i >> 16;
}
inline int symNext(int i) {
    return i & 0xffff;
}
inline int symInts(int i) {
    return i >> 12;
}
inline int symSymLen(int i) {
    return i % (1 << 12);
}
inline int symInfoLen(int i) {
    return i >> 8;
}
inline int symPrec(int i) {
    return (i >> 4) % 16;
}
inline int symType(int i) {
    return iabs(i) & 0xf;
}
char b32Digit(int ch);

int symloc;
int symlen;
int acnt = 0;
/* cntrl */
int contrl[26];
/* _data */
enum { OPR = 0, ADR, VLU, DEF, LIT, LIN };
// clang-format off
enum {
    NOP = 0,
    ADD, ADC, SUB, SBC, MUL, DIV, REM, NEG, AND, IOR,
    XOR, NOT, EQL, LSS, GTR, NEQ, LEQ, GEQ, INX, TRA,
    TRC, PRO, RET, STO, STD, XCH, DEL, DAT, LOD, BIF,
    INC, CSE, END, ENB, ENP, HAL, RTL, RTR, SFL, SFR,
    HIV, LOV, CVA, ORG, DRT, ENA, DIS, AX1, AX2, AX3
};
// clang-format on

const int VERS = 40;
/* inter */
int intpro[8 + 1];
/* macro */

/* syntax */
/*     syntax analyzer tables*/
// vindx contains the start index of the first token of a given length
const uint8_t vindx[] = { 0, 1, 14, 20, 26, 35, 39, 41, 45, 47, 50 };
// clang-format off
const char *tokens[]  = {
    /* 0 */ "null",
    /* 1 */ ";", ")", "(", ",", ":", "=", "<", ">", "+", "-", "*", "/", ".",
    /*14 */ "IF", "DO", "GO", "TO", "OR", "BY",
    /*20 */ "EOF", "END", "XOR", "AND", "NOT", "MOD",
    /*26 */ "HALT", "THEN", "ELSE", "CASE", "CALL", "GOTO", "DATA", "BYTE", "PLUS",
    /*35 */ "LABEL", "BASED", "MINUS", "WHILE",
    /*39 */ "ENABLE", "RETURN", "DISABLE", "DECLARE", "ADDRESS", "INITIAL",
    /*45 */ "<NUMBER>", "<STRING>",
    /*47 */ "INTERRUPT", "PROCEDURE", "LITERALLY",
    /*50 */ "<IDENTIFIER>",
    /*51 */ "<TO>", "<BY>",
    /*53 */ "<TYPE>", "<TERM>",
    /*55 */ "<GROUP>", "<WHILE>", "<GO TO>",
    /*58 */ "<ENDING>", "<PROGRAM>",
    /*60 */ "<REPLACE>", "<PRIMARY>",
    /*62 */ "<VARIABLE>", "<CONSTANT>", "<RELATION>",
    /*65 */ "<STATEMENT>", "<IF CLAUSE>", "<TRUE PART>", "<DATA LIST>", "<DATA HEAD>", "<LEFT PART>",
    /*71 */ "<ASSIGNMENT>", "<EXPRESSION>", "<GROUP HEAD>", "<BOUND HEAD>",
    /*75 */ "<IF STATEMENT>", "<WHILE CLAUSE>", "<INITIAL LIST>", "<INITIAL HEAD>",
    /*79 */ "<CASE SELECTOR>", "<VARIABLE NAME>", "<CONSTANT HEAD>",
    /*82 */ "<STATEMENT LIST>", "<CALL STATEMENT>", "<PROCEDURE HEAD>",
    /*85 */ "<PROCEDURE NAME>", "<PARAMETER LIST>", "<PARAMETER HEAD>",
    /*88 */ "<BASED VARIABLE>", "<LOGICAL FACTOR>", "<SUBSCRIPT HEAD>",
    /*91 */ "<BASIC STATEMENT>", "<GO TO STATEMENT>", "<STEP DEFINITION>",
    /*94 */ "<IDENTIFIER LIST>", "<LOGICAL PRIMARY>",
    /*96 */ "<RETURN STATEMENT>", "<LABEL DEFINITION>", "<TYPE DECLARATION>",
    /*99 */ "<ITERATION CONTROL>", "<LOGICAL SECONDARY>",
    /*101*/ "<LOGICAL EXPRESSION>",
    /*102*/ "<DECLARATION ELEMENT>",
    /*103*/ "<PROCEDURE DEFINITION>",
    /*104*/ "<DECLARATION STATEMENT>", "<ARITHMETIC EXPRESSION>",
    /*106*/ "<IDENTIFIER SPECIFICATION>"
};
/* token ids */
enum {
    SEMIV = 1, DOV = 15,   EOFILE = 20, ENDV = 21,   CALLV = 30, DECL = 42, NUMBV = 45,
    STRV = 46, PROCV = 48, IDENTV = 50, GROUPV = 55, STMTV = 65, SLISTV = 82
};
// clang-format on

const uint8_t c1[][13] = {
    /*   1 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x0A, 0x20, 0xA2, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*   2 */ { 0xA8, 0xAA, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0xEC, 0x08, 0xC0, 0x02, 0x00 },
    /*   3 */ { 0xC0, 0x00, 0x30, 0x0C, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x30 },
    /*   4 */ { 0xC0, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x28, 0x30 },
    /*   5 */ { 0x08, 0x10, 0x00, 0xA0, 0x02, 0x08, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x22 },
    /*   6 */ { 0xC0, 0x00, 0x30, 0x0C, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x30 },
    /*   7 */ { 0x80, 0x10, 0x21, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*   8 */ { 0x80, 0x10, 0x20, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*   9 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  10 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  11 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  12 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  13 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  14 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  15 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x10 },
    /*  16 */ { 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  17 */ { 0x80, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*  18 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  19 */ { 0x80, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*  20 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x00, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  21 */ { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  22 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  23 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  24 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  25 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  26 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  27 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x00, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  28 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x00, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  29 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  30 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  31 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x20 },
    /*  32 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  33 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  34 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  35 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  36 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 },
    /*  37 */ { 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  38 */ { 0x80, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20 },
    /*  39 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  40 */ { 0x48, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  41 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  42 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  43 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  44 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  45 */ { 0x3C, 0xA6, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  46 */ { 0x28, 0xA2, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  47 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 },
    /*  48 */ { 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x80, 0x80, 0x00 },
    /*  49 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 },
    /*  50 */ { 0xF8, 0xAF, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0xA9, 0x09, 0x80, 0x00, 0x04 },
    /*  51 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  52 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  53 */ { 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  54 */ { 0x28, 0xA2, 0x6A, 0x01, 0xA8, 0xA0, 0x84, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  55 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  56 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  57 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x10 },
    /*  58 */ { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  59 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  60 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  61 */ { 0x28, 0xA2, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  62 */ { 0x28, 0xB7, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  63 */ { 0x38, 0xA3, 0xAA, 0x02, 0xA8, 0xA0, 0x88, 0x00, 0x20, 0x08, 0x00, 0x00, 0x00 },
    /*  64 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  65 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x0A, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  66 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x00, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  67 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x00, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  68 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  69 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00 },
    /*  70 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  71 */ { 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  72 */ { 0x18, 0x01, 0x00, 0x00, 0x44, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  73 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x04, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  74 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 },
    /*  75 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x0A, 0x20, 0xA0, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  76 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  77 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
    /*  78 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00 },
    /*  79 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  80 */ { 0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x80, 0x00, 0x00 },
    /*  81 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00 },
    /*  82 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x06, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  83 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  84 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x00, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x10 },
    /*  85 */ { 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x40, 0x40, 0x00 },
    /*  86 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x40, 0x00, 0x00 },
    /*  87 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  88 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  89 */ { 0x28, 0x02, 0x00, 0x00, 0xA8, 0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  90 */ { 0x40, 0x00, 0x10, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10 },
    /*  91 */ { 0x08, 0x00, 0x00, 0xA0, 0x02, 0x0A, 0x20, 0xA3, 0x00, 0x80, 0x2A, 0x08, 0x20 },
    /*  92 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  93 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  94 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 },
    /*  95 */ { 0x28, 0x02, 0x00, 0x00, 0xA8, 0xA0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  96 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*  97 */ { 0x04, 0x00, 0x00, 0x50, 0x01, 0x04, 0x10, 0x50, 0x00, 0x40, 0x15, 0x04, 0x11 },
    /*  98 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
    /*  99 */ { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 100 */ { 0x28, 0x02, 0x00, 0x00, 0xA8, 0xA0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 101 */ { 0x28, 0x02, 0x00, 0x00, 0x98, 0x10, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 102 */ { 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 103 */ { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 104 */ { 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 105 */ { 0x28, 0x52, 0x15, 0x00, 0xA8, 0xA0, 0x80, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00 },
    /* 106 */ { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x40, 0x00, 0x00 },
    /* 107 */ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

#define TRI(a, b, option) (((a) << 16) + ((b) << 8) + option)
#define PAIR(a, b)        (((a) << 8) + (b))

const int c1tri[] = {
    TRI(3, 3, 3),   TRI(3, 3, 10),   TRI(3, 3, 13),  TRI(3, 3, 24),  TRI(3, 3, 45),  TRI(3, 3, 46),
    TRI(3, 3, 50),  TRI(3, 50, 3),   TRI(5, 6, 3),   TRI(5, 6, 10),  TRI(5, 6, 13),  TRI(5, 6, 24),
    TRI(5, 6, 45),  TRI(5, 6, 46),   TRI(5, 6, 50),  TRI(6, 3, 3),   TRI(6, 3, 10),  TRI(6, 3, 13),
    TRI(6, 3, 24),  TRI(6, 3, 45),   TRI(6, 3, 46),  TRI(6, 3, 50),  TRI(6, 50, 3),  TRI(9, 3, 3),
    TRI(9, 3, 10),  TRI(9, 3, 13),   TRI(9, 3, 24),  TRI(9, 3, 45),  TRI(9, 3, 46),  TRI(9, 3, 50),
    TRI(9, 50, 3),  TRI(10, 3, 3),   TRI(10, 3, 10), TRI(10, 3, 13), TRI(10, 3, 24), TRI(10, 3, 45),
    TRI(10, 3, 46), TRI(10, 3, 50),  TRI(10, 50, 3), TRI(11, 3, 3),  TRI(11, 3, 10), TRI(11, 3, 13),
    TRI(11, 3, 24), TRI(11, 3, 45),  TRI(11, 3, 46), TRI(11, 3, 50), TRI(11, 50, 3), TRI(12, 3, 3),
    TRI(12, 3, 10), TRI(12, 3, 13),  TRI(12, 3, 24), TRI(12, 3, 45), TRI(12, 3, 46), TRI(12, 3, 50),
    TRI(12, 50, 3), TRI(13, 50, 3),  TRI(14, 3, 3),  TRI(14, 3, 10), TRI(14, 3, 13), TRI(14, 3, 24),
    TRI(14, 3, 45), TRI(14, 3, 46),  TRI(14, 3, 50), TRI(14, 50, 3), TRI(15, 50, 3), TRI(15, 62, 6),
    TRI(18, 3, 3),  TRI(18, 3, 10),  TRI(18, 3, 13), TRI(18, 3, 24), TRI(18, 3, 45), TRI(18, 3, 46),
    TRI(18, 3, 50), TRI(18, 50, 3),  TRI(20, 50, 3), TRI(20, 50, 5), TRI(20, 62, 4), TRI(20, 62, 6),
    TRI(20, 71, 1), TRI(22, 3, 3),   TRI(22, 3, 10), TRI(22, 3, 13), TRI(22, 3, 24), TRI(22, 3, 45),
    TRI(22, 3, 46), TRI(22, 3, 50),  TRI(22, 50, 3), TRI(23, 3, 3),  TRI(23, 3, 10), TRI(23, 3, 13),
    TRI(23, 3, 24), TRI(23, 3, 45),  TRI(23, 3, 46), TRI(23, 3, 50), TRI(23, 50, 3), TRI(24, 3, 3),
    TRI(24, 3, 10), TRI(24, 3, 13),  TRI(24, 3, 24), TRI(24, 3, 45), TRI(24, 3, 46), TRI(24, 3, 50),
    TRI(24, 50, 3), TRI(25, 3, 3),   TRI(25, 3, 10), TRI(25, 3, 13), TRI(25, 3, 24), TRI(25, 3, 45),
    TRI(25, 3, 46), TRI(25, 3, 50),  TRI(25, 50, 3), TRI(29, 3, 3),  TRI(29, 3, 10), TRI(29, 3, 13),
    TRI(29, 3, 24), TRI(29, 3, 45),  TRI(29, 3, 46), TRI(29, 3, 50), TRI(29, 50, 3), TRI(30, 50, 3),
    TRI(34, 3, 3),  TRI(34, 3, 10),  TRI(34, 3, 13), TRI(34, 3, 24), TRI(34, 3, 45), TRI(34, 3, 46),
    TRI(34, 3, 50), TRI(34, 50, 3),  TRI(37, 3, 3),  TRI(37, 3, 10), TRI(37, 3, 13), TRI(37, 3, 24),
    TRI(37, 3, 45), TRI(37, 3, 46),  TRI(37, 3, 50), TRI(37, 50, 3), TRI(40, 3, 3),  TRI(40, 3, 10),
    TRI(40, 3, 13), TRI(40, 3, 24),  TRI(40, 3, 45), TRI(40, 3, 46), TRI(40, 3, 50), TRI(40, 50, 3),
    TRI(45, 2, 33), TRI(45, 2, 35),  TRI(45, 2, 43), TRI(47, 45, 1), TRI(51, 3, 3),  TRI(51, 3, 10),
    TRI(51, 3, 13), TRI(51, 3, 24),  TRI(51, 3, 45), TRI(51, 3, 46), TRI(51, 3, 50), TRI(51, 50, 3),
    TRI(52, 3, 3),  TRI(52, 3, 10),  TRI(52, 3, 13), TRI(52, 3, 24), TRI(52, 3, 45), TRI(52, 3, 46),
    TRI(52, 3, 50), TRI(52, 50, 3),  TRI(56, 3, 3),  TRI(56, 3, 10), TRI(56, 3, 13), TRI(56, 3, 24),
    TRI(56, 3, 45), TRI(56, 3, 46),  TRI(56, 3, 50), TRI(56, 50, 3), TRI(60, 3, 3),  TRI(60, 3, 10),
    TRI(60, 3, 13), TRI(60, 3, 24),  TRI(60, 3, 45), TRI(60, 3, 46), TRI(60, 3, 50), TRI(60, 50, 3),
    TRI(64, 3, 3),  TRI(64, 3, 10),  TRI(64, 3, 13), TRI(64, 3, 24), TRI(64, 3, 45), TRI(64, 3, 46),
    TRI(64, 3, 50), TRI(64, 50, 3),  TRI(66, 50, 3), TRI(66, 50, 5), TRI(66, 62, 4), TRI(66, 62, 6),
    TRI(66, 71, 1), TRI(66, 91, 28), TRI(67, 50, 3), TRI(67, 50, 5), TRI(67, 62, 4), TRI(67, 62, 6),
    TRI(67, 71, 1), TRI(69, 63, 2),  TRI(69, 63, 4), TRI(70, 50, 3), TRI(70, 62, 4), TRI(70, 62, 6),
    TRI(73, 50, 3), TRI(73, 50, 5),  TRI(73, 62, 4), TRI(73, 62, 6), TRI(73, 71, 1), TRI(74, 45, 2),
    TRI(78, 63, 2), TRI(78, 63, 4),  TRI(81, 63, 2), TRI(81, 63, 4), TRI(82, 50, 3), TRI(82, 50, 5),
    TRI(82, 62, 4), TRI(82, 62, 6),  TRI(82, 71, 1), TRI(84, 50, 3), TRI(84, 50, 5), TRI(84, 62, 4),
    TRI(84, 62, 6), TRI(84, 71, 1),  TRI(85, 53, 1), TRI(86, 53, 1), TRI(87, 50, 2), TRI(87, 50, 4),
    TRI(90, 3, 3),  TRI(90, 3, 10),  TRI(90, 3, 13), TRI(90, 3, 24), TRI(90, 3, 45), TRI(90, 3, 46),
    TRI(90, 3, 50), TRI(90, 50, 3),  TRI(97, 50, 3), TRI(97, 50, 5), TRI(97, 62, 4), TRI(97, 62, 6),
    TRI(97, 71, 1), TRI(104, 4, 3),  TRI(104, 4, 50)
};

// clang-format off
const int prtb[] = {
    0,               TRI(85, 86, 53), TRI(85, 47, 45), PAIR(85, 53),  PAIR(85, 86),
    PAIR(15, 93),    PAIR(15, 76),    PAIR(15, 79),    85,            15,         
    71,              55,              103,             96,            83,         
    92,              104,             26,              39,            41,         
    0,               PAIR(69, 63),    PAIR(78, 63),    PAIR(87, 50),  PAIR(94, 80),
    PAIR(81, 63),    PAIR(3, 72),     PAIR(90, 72),    32,            106,         
    44,              13,              50,              0,             0,         
    PAIR(87, 50),    PAIR(69, 63),    PAIR(94, 80),    PAIR(78, 63),  PAIR(81, 63),
    PAIR(90, 72),    62,              50,              45,            7,         
    8,               0,               0,               0,             7,         
    0,               16,              0,               0,             0,         
    PAIR(14, 72),    91,              0,               0,             0,         
    50,              0,               0,               0,             57,         
    0,               PAIR(50, 49),    0,               97,            21,         
    57,              88,              0,               0,             TRI(74, 45, 2),
    106,             PAIR(105, 9),    PAIR(105, 10),   PAIR(105, 34), PAIR(105, 37),
    10,              0,               PAIR(84, 82),    97,            73,         
    PAIR(54, 11),    PAIR(54, 12),    PAIR(54, 25),    0,             30,         
    13,              0,               13,              0,             PAIR(66, 67),
    82,              73,              66,              0,             50,         
    70,              TRI(51, 72, 52), PAIR(62, 60),    51,            56,         
    29,              40,              97,              0,             98,         
    0,               0,               PAIR(101, 18),   PAIR(101, 22), 0,         
    97,              0,               24,              0,             0,         
    TRI(62, 60, 72), PAIR(89, 23),    0,               TRI(62, 5, 6), 0,         
    PAIR(104, 4),    42,              PAIR(105, 64),   PAIR(0, 0)
};
// clang-format on
const uint8_t prdtb[] = { 0,   38,  39,  36,  37,  25,  26,  27,  35,  24,  6,   7,   8,   9,   10,
                          11,  12,  13,  14,  15,  16,  61,  78,  41,  72,  114, 117, 121, 62,  70,
                          79,  118, 122, 42,  73,  43,  63,  74,  80,  119, 123, 84,  47,  48,  100,
                          101, 96,  83,  97,  99,  98,  54,  126, 127, 44,  21,  22,  55,  67,  69,
                          77,  128, 49,  68,  53,  125, 59,  124, 40,  45,  52,  76,  75,  120, 65,
                          64,  103, 104, 105, 106, 107, 102, 34,  46,  23,  109, 110, 111, 108, 51,
                          116, 115, 113, 112, 19,  3,   28,  18,  2,   60,  82,  31,  81,  30,  32,
                          33,  50,  20,  5,   66,  71,  1,   88,  89,  87,  17,  4,   93,  92,  58,
                          29,  91,  90,  86,  85,  57,  56,  95,  94 };

const uint8_t hdtb[]  = { 0,  84,  84,  84,  84,  73,  73,  73,  84,  73,  91, 91, 91,  91,  91,
                          91, 91,  91,  91,  91,  91,  68,  77,  86,  106, 61, 61, 62,  69,  74,
                          78, 81,  90,  87,  94,  87,  69,  94,  78,  81,  90, 70, 97,  97,  64,
                          64, 64,  60,  64,  64,  64,  57,  51,  52,  58,  66, 67, 57,  53,  53,
                          88, 56,  96,  53,  92,  63,  102, 63,  85,  58,  92, 80, 80,  62,  98,
                          98, 105, 105, 105, 105, 105, 105, 103, 58,  55,  54, 54, 54,  54,  83,
                          61, 61,  61,  61,  75,  82,  73,  75,  82,  102, 71, 99, 71,  99,  76,
                          79, 96,  75,  65,  98,  106, 59,  101, 101, 101, 91, 65, 100, 100, 102,
                          93, 89,  89,  72,  72,  104, 104, 95,  95 };

const uint8_t prlen[] = { 0, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 3,
                          3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 3, 3, 3, 3, 3, 3, 2, 2, 2,
                          2, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 3, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1,
                          3, 1, 2, 2, 2, 2, 1, 1, 4, 2, 3, 3, 3, 3, 2, 1, 3, 2, 2, 3, 3, 3,
                          1, 2, 2, 1, 2, 1, 3, 2, 2, 2, 1, 2, 2, 4, 3, 2, 2, 2, 2, 2, 1, 2,
                          1, 1, 3, 3, 1, 2, 1, 2, 1, 1, 4, 3, 1, 4, 1, 3, 2, 3, 1 };

const uint8_t contc[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const uint8_t leftc[] = { ZPAD, 105, 4, 42, 94, 85 };

const uint8_t lefti[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 5,
                          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

const uint8_t prind[] = { 1,   21,  28,  35,  42,  44,  48,  49,  51,  51,  51,  51,  51,  51,
                          51,  51,  51,  53,  53,  54,  54,  55,  55,  55,  55,  55,  55,  56,
                          57,  57,  57,  58,  58,  59,  59,  60,  61,  61,  62,  62,  63,  63,
                          63,  64,  64,  66,  68,  68,  69,  69,  74,  74,  74,  76,  82,  82,
                          82,  82,  85,  85,  85,  89,  92,  94,  94,  99,  99,  99,  100, 100,
                          100, 101, 107, 107, 107, 109, 109, 110, 110, 110, 111, 111, 112, 112,
                          112, 112, 112, 112, 112, 115, 115, 117, 117, 117, 117, 119, 119, 119,
                          120, 121, 123, 125, 127, 127, 127, 129, 129 };

int const nt          = 50;

#define PACK   4 // number of packed chars per int
#define MAXBLK 30
int token = 0;
/* blk */
int block[MAXBLK + 1] = { ZPAD, 1, 120 };
struct {
    unsigned type : 2;
    unsigned clause : 14;
    unsigned label : 16;
} dopar[MAXBLK + 1];

inline doHead(int slot, int type, int clause) {
    dopar[slot].type   = type;
    dopar[slot].clause = clause & 0x3fff;
    dopar[slot].label  = clause >> 14;
}

enum { DO_GROUP, DO_ITER, DO_WHILE, DO_CASE };

int macblk[MAXBLK + 1];
int curblk = 2;
int blksym = 120;
int proctp[MAXBLK + 1];
/* files */
char obuff[132 + 1];

int ibp  = 81;
int obp  = 0;
int inSP = 0;
FILE *instk[7 + 1];

const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

/* scanc */
int acclen;
#define MAXSTR 4096
char accum[MAXSTR + 1];
int accumIVal = 0;
enum { EOFLAG = 1, IDENT, NUMB, SPECL, STR };
const int CONT = 1;
/* hash */

int hentry[128];

int hcode;

/* function declarations */
int main(int argc, char **argv);
void exitb();
int lookup(const int iv);
int enter(int Info);
void dumpsy();
void recov();
bool stack(/*const int q */);
bool prok(const int prd);
void reduce();
void cloop();
// void prsym(const int cc, const int sym);
int getc1(int i, int j);
void scan();
int wrdata(const int sy);
void dumpch();
void synth(const int prod, const int symm);
int gnc();
void parseOptions(uint8_t *s);
void decibp();
int conv(int radix);
int imin(const int i, const int j);
void stackc(char *fname);
void enterb();
void dumpin();
// void error(const int i, const int level);
void error(char const *fmt, ...);
void fatal(char *fmt, ...);
// int shr(const int i, const int j);
// int shl(const int i, const int j);
int right(const int i, const int j);
void sdump();
void redpr(const int prod, const int sym);
void emit(const int val, const int typ);
void cmpuse();
void install();

int errorCnt;
#define C_ANALYZE  contrl['A' - 'A'] // action/reduction trace
#define C_BYPASS   contrl['B' - 'A'] // bypass stack dump on error
#define lineCnt    contrl['C' - 'A'] // line count start value
#define C_DELETE   contrl['D' - 'A'] // max listing line length
#define C_GENERATE contrl['G' - 'A'] // show intermediate code generation
#define C_MEMORY   contrl['M' - 'A'] // dump symbol table (0 = no, 1 = yes)
#define C_PRINT    contrl['P' - 'A'] // controls printing on lst file
#define C_SYMBOLS  contrl['S' - 'A'] // control symbol dump
#define C_UPPER    contrl['U' - 'A'] // convert string text to upper case
#define C_VWIDTH   contrl['V' - 'A'] // symbol file line width
#define C_WIDTH    contrl['W' - 'A'] // listing file width


/*
 file management is changed from the original cross compiler
 If the user specifies a file, its name prefix is used to create other files, otherwise the
 input is assumed to be from stdin and the other files have a prefix "stdin"
 srcFp   - uses specfied file, adding .plm if there is no extent
 polFp   - creates prefix.pol
 symFp   - creates prefix.sym
 lstFp   - creates prefix.lst

 To support include files $I=filename can be used with the previous file being stacked
 Note a .plm extension is used if no extension is given

*/
FILE *srcFp;
FILE *polFp;
FILE *symFp;
FILE *lstFp;
char path[_MAX_PATH];
char *plmFile;
int autoId = 100;

char *makeFileName(char const *fname, char const *ext, bool force) {

    char *s;
    strcpy(path, fname);
    if ((s = strrchr(path, '.')) && !strpbrk(s, DIRSEP)) {
        if (force)
            strcpy(s, ext);
    } else
        strcat(path, ext);
    return path;
}

void openfiles(char *srcFile) {

    if (!(srcFp = fopen(makeFileName(srcFile, ".plm", false), "rt"))) {
        fprintf(stderr, "can't open source file %s\n", path);
        exit(1);
    }

    if (!(polFp = fopen(makeFileName(srcFile, ".pol", true), "wb"))) {
        fprintf(stderr, "can't create pol file %s\n", path);
        exit(1);
    }
    if (!(symFp = fopen(makeFileName(srcFile, ".sym", true), "wt"))) {
        fprintf(stderr, "can't create symbol file %s\n", path);
        exit(1);
    }
    if (!(lstFp = fopen(makeFileName(srcFile, ".lst", true), "wt"))) {
        fprintf(stderr, "can't create listing file %s\n", path);
        exit(1);
    }
}

void closefiles() {
    while (inSP > 0) {
        fclose(srcFp);
        srcFp = instk[inSP--];
    }
    if (srcFp != stdin)
        fclose(srcFp);
    fclose(polFp);
    fclose(symFp);
    fclose(lstFp);
}

int main(int argc, char **argv) {

    CHK_SHOW_VERSION(argc, argv);

    if (argc != 2 || strcmp(argv[1], "-h") == 0) {
        printf("\nUsage: plm81 -v | -V  | -h | srcfile\n"
               "Where\n"
               "-v/-V      provide version infomation\n"
               "-h         shows this help\n"
               "srcfile    is the source file, of the form prefix.ext e.g. m80.plm\n"
               "           intermediate files prefix.lst, prefix.pol, prefix.sym are created\n"
               "           if no srcfile does not have an extension, .plm is added\n");
        exit(0);
    }

    for (int i = 0; i < 26; i++)
        contrl[i] = -1;
    errorCnt   = 0;
    C_ANALYZE  = 0;
    C_BYPASS   = 1;
    lineCnt    = 0;
    C_DELETE   = 132; /* changed from original 120 */
    C_GENERATE = 0;
    C_MEMORY   = 1; // dump symbol table (0 = no, 1 = yes)
    C_PRINT    = 1; // print on 1 else off
    C_SYMBOLS  = 0;
    C_UPPER    = 0;
    C_VWIDTH   = 72;
    C_WIDTH    = 132; /* changed from original 132 */

    openfiles(argv[1]);

    install();

    time_t now;
    time(&now);
    fprintf(lstFp, "\n         pl/m-8080  version 4.0 - %s\n", ctime(&now));

    sp         = 4;
    pstack[sp] = EOFILE;

    scan();
    cloop();
    emit(NOP, OPR); // mark end

  
    char cnt[8] = "NO";
    if (errorCnt)
        sprintf(cnt, "%d", errorCnt);
    fprintf(lstFp, "\n\n%s PROGRAM ERROR%s\n\n", cnt, errorCnt != 1 ? "S" : "");
    printf("%s PROGRAM ERROR%s\n\n", cnt, errorCnt != 1 ? "S" : "");

    dumpsy();

    dumpch();
    dumpin();
    closefiles();
    cmpuse();
    return errorCnt;
}

void dumpsym(int idx) {
    static char *types[]  = { "VARB", "INTR", "PROC", "LABEL", "LITER", "NUMBER" };
    static char *labels[] = { "OuterLabel", "LocalLabel", "CompilerLabel" };

    fprintf(stderr, "Symbol %s%d at %d:", symbol[idx] < 0 ? "-" : "", id_num(idx), idx);
    int ints = sym_ints(idx);
    int len  = sym_len(idx);
    int type = info_type(idx);
    int prec = info_prec(idx);
    int cnt  = 0;
    switch (type) {
    case VARB:
    case LITER:
    case PROC:
    case LABEL:
    case INTR:
        if (ints) {

            fputs(" '", stderr);
            for (int i = 0; i < ints; i++) {
                int packed = symbol[SymInts(idx) + i];
                for (int j = 0; j < PACK && cnt++ < len; j++)
                    putc((packed >> (24 - j * 8)) & 0xff, stderr);
            }
            fputs("', ", stderr);
        } else
            fprintf(stderr, " ints=%d, len=%d, ", ints, len);

        if (type != LABEL || prec < OuterLabel || prec > CompilerLabel)
            fprintf(stderr, "type=%s prec=%d ilen=%d", types[type - VARB], prec, info_len(idx));
        else
            fprintf(stderr, "type=%s ilen=%d", labels[prec - OuterLabel], info_len(idx));
        if (symbol[Info(idx)] < 0)
            fprintf(stderr, ", BASED(%d)", symbol[SymInts(idx) + ints]);
        break;
    case NUMBER:
        fprintf(stderr, " ints=%d, len=%d, ", ints, len);
        fprintf(stderr, ", type=NUMBER, prec=%d, val=%d", prec, info_len(idx));
        break;
    default:
        fprintf(stderr, " ints=%d, len=%d, type=UNKNOWN(%d), prec=%d, ilen=%d", ints, len, type,
                prec, info_len(idx));

        break;
    }
    if (ints)
        fprintf(stderr, ", hcode=%d, next=%d", hash_hcode(idx), hash_next(idx));
    putc('\n', stderr);
}

void exitb() {

    /*     goes through here upon block exit */
    /*      global tables */
    if (curblk > 2) {
        int i      = block[curblk];
        /* de-allocate those macro definitions whose scope we are leaving,
         * and check if any of these are currently in expansion.
         */
        dropMacro(macblk[curblk--]);

        for (int symIdx = symbol[symtop]; symIdx >= i; symIdx = id_next(symIdx)) {
            symIdx++; // adjust to symbol true base as chain is at -1

            if (symbol[Sym(symIdx)] >= 0) { // in scope
                int type = info_type(symIdx);

                if (type < LITER) {
                    if (type == VARB || type == LABEL) {
                        if (info_prec(symIdx) == 0) {
                            if (type == LABEL && curblk > 1) // labels may be non local
                                continue;                    // only fail if not defined at all
                            char name[64];
                            char *s = name;
                            int n   = sym_ints(symIdx);
                            if (n) {
                                for (int j = 0; j < n; j++) {
                                    int packed = symbol[SymInts(symIdx) + j];
                                    for (int i = 0; i < PACK; i++)
                                        *s++ = (packed >> (24 - i * 8)) & 0xff;
                                }
                            }
                            *s = '\0';
                            error("1: undefined symbol '%s'", name);
                        }
                    }
                    symbol[Sym(symIdx)] = -symbol[Sym(symIdx)]; // negate length fields
                                                                // to mark out of scope
                    /* remove from hash chain if on one*/

                    if (sym_ints(symIdx) > 0) { // has a hash chain
                        /* find match on the entry */
                        int hcode = hash_hcode(symIdx);
                        int next  = hash_next(symIdx);
                        int n     = hentry[hcode];
                        if (n == Hash(symIdx)) /* this entry is directly connected */
                            hentry[hcode] = next;
                        else {
                            int np;
                            while ((np = hash_next(n + 2)) != Hash(symIdx))
                                n = np;
                            symbol[n] = (hcode << 16) + next;
                        }
                    }
                }
            }
        }
        blksym = block[curblk];
    }
}

int packVarc(int loc, int len) {
    /*     pack varc into varc[loc] */
    /*     varc is in internal format */
    /*     varc[loc] is the start of the packed data */
    /*     len is the number of characters to pack */
    /* convert varc to internal format */
    int dstIdx = 0;
    int shift  = PACK * 8;
    int m      = 0;
   
    for (int srcIdx = 0; srcIdx < len; srcIdx++) {
        if ((shift -= 8) < 0) {
            varc[loc + dstIdx++] = m;
            m                    = 0;
            shift                = PACK * 8 - 8;
        }
        m += (varc[loc + srcIdx] << shift);
    }
    varc[loc + dstIdx] = m;
    /*     varc is now in packed form ready for lookup */
    /*     compute hash code (reduce numbers mod 127, use first 5 chars of */
    /*     identifiers and strings ) */
    return varc[loc]; // first packed word is used for hash code
}

int lookup(const int iv) {
    /*     jp is identifier, m is variable, LABEL, or procedure. */
    /*     syntax analyzer tables */
    /*      global tables */

    symlen = var[iv].len;
    symloc = var[iv].loc;

    hcode  = (pstack[iv] == NUMBV ? fixc[iv] : packVarc(symloc, symlen)) % 127 + 1; // hash code

    /*     hcode is in the range 1 to 127 */
    // hentry items point to hash chain, symbol base is +2 from this
    int intCnt = (symlen - 1) / PACK + 1;

    for (int symIdx = hentry[hcode] + 2; symIdx > 2; symIdx = hash_next(symIdx) + 2) {

        if (pstack[iv] == NUMBV) {
            if (info_type(symIdx) > LITER && info_len(symIdx) == fixv[iv])
                return symIdx;
        } else if (symbol[symIdx] > 0 && sym_len(symIdx) == symlen) {
            if (memcmp(&varc[symloc], &symbol[SymInts(symIdx)], intCnt * sizeof(varc[0])) == 0) {
                /*     make sure the types match. */
                if ((pstack[iv] == STRV && info_type(symIdx) == LITER) ||
                    (pstack[iv] == IDENTV && info_type(symIdx) < LITER))
                    return symIdx;
            }
        }
    }

    return 0;
}

int enter(int info) {
    bool hasHash = info >= 0; // if info is negative, no hash code
    /*     syntax analyzer tables */

    /*      global tables */
    /*      enter assumes a previous call to lookup (either that, or set up */
    /*      the values of symloc and symlen in the varc array). */
    /*         also set-up hash code value (see lookup), if necessary */
    int symIdx = symtop;
    int prev   = symbol[symIdx];
    int intCnt = 0;

    if (hasHash) {
        intCnt = (symlen - 1) / PACK + 1;
        symtop += intCnt + 4;
        symIdx++;
    } else {
        /*     entry with no external name */
        info = -info;
        symtop += 3;
    }

    if (symtop > SYMABS) {
        symIdx = hasHash ? 1 : 0;
        symtop = symIdx + intCnt + 3;
        fatal("2: pass-1 symbol table overflow");
    }

    symbol[symtop]       = symIdx++;
    symbol[Id(symIdx)]   = (++symcnt << 16) + prev;
    symbol[Sym(symIdx)]  = (intCnt << 12) + symlen;
    symbol[Info(symIdx)] = info;
    for (int j = 0; j < intCnt; j++)
        symbol[SymInts(symIdx) + j] = varc[symloc + j];
    /*     compute hash table entry */
    /*     fix collision chain */
    if (hasHash) {
        symbol[Hash(symIdx)] = (hcode << 16) + hentry[hcode];
        hentry[hcode]        = Hash(symIdx);
    }
    return symIdx;
}

void install() {
    symloc = 0;
    for (int i = 0; i < ASIZE(builtins); i++) {
        for (symlen = 0; builtins[i].name[symlen]; symlen++)
            varc[symlen] = builtins[i].name[symlen];
        hcode = packVarc(0, symlen) % 127 + 1; // hash code
        enter(builtins[i].info);
    }
}

void putSym(char ch) {
    static char symline[120];
    static uint8_t pos;
    if (ch != '\n')
        symline[pos++] = ch;
    if (ch == '\n' || pos >= C_VWIDTH) {
        while (pos && symline[pos - 1] == ' ') // remove trailing spaces
            pos--;
        fprintf(symFp, " %.*s\n", pos, symline);
        pos = 0;
    }
}

void putSymStr(const char *s) {
    while (*s) {
        putSym(*s);
        s++;
    }
}

void putSymInt(int n, int width) {
    do {
        int digit = n % 32;                                  // convert to base 32
        putSym(digit < 10 ? '0' + digit : 'A' + digit - 10); // convert to ASCII
        width--;
    } while (n /= 32);
    while (width-- > 0)
        putSym('0'); // pad with zeros
}

void putSymHex2(int n, bool tag) {
    putSym(digits[HIGHNIBBLE(n) + (tag ? 16 : 0)]);
    putSym(digits[LOWNIBBLE(n)]);
}

void dumpsy() {
    int lp, ic, mc, ip, it, i, j, k, n;
    /*      global tables */
#if 0
    for (i = symbol[it = symtop]; i > 0; i = id_next((it = i) + 1)) {
        /*     quick check for zero length name */
        dumpsym(i + 1);
    }
#endif
    ic = C_SYMBOLS;
    if (ic != 0) {
        if (ic > 1)
            fprintf(lstFp, "\nSYMBOL  ADDR WDS CHRS   LENGTH PR TY");
        for (i = symbol[it = symtop]; i > 0; i = id_next((it = i) + 1)) {
            /*     quick check for zero length name */
            int symIdx = i + 1;
            if (ic >= 2 || sym_len(symIdx) > 0)
                fprintf(lstFp, "S%05d", id_num(symIdx));

            if (ic >= 2) {
                fprintf(lstFp, "%c ", symbol[symIdx] < 0 ? '*' : ' '); // * if now out of scope
                fprintf(lstFp, "%04d %3d %4d ", symIdx, sym_ints(symIdx), sym_len(symIdx));
                fprintf(lstFp, "%c ", symbol[Info(symIdx)] < 0 ? 'B' : 'R'); // based or regular
                fprintf(lstFp, "%06d%3d%3d", info_len(symIdx), info_prec(symIdx),
                        info_type(symIdx));
            }
            putc(' ', lstFp);
            ip = Info(symIdx);
            n  = sym_ints(symIdx);
            if (n != 0) { // has size
                mc       = sym_len(symIdx);
                int type = info_type(symIdx);
                if (type == LITER)
                    putc('\'', lstFp);
                for (int i = 1; i <= n; i++) {
                    int packed = symbol[i + ip];
                    for (lp = 0; lp < PACK && mc-- > 0; lp++)
                        putc((packed >> (24 - lp * 8)) & 0xff, lstFp);
                }
                if (type == LITER)
                    putc('\'', lstFp);
            }
            ip += n;
            if (ic >= 2)
                while (++ip < it) {
                    k = symbol[ip];
                    fprintf(lstFp, " %c%08XH", (k < 0) ? '-' : ' ', iabs(k));
                }
            putc('\n', lstFp);
        }
        putc('\n', lstFp);
    }

    /*     write the interrupt procedure names */
    putSym('/');
    for (int intProcNum = 1; intProcNum <= 8; intProcNum++) {
        int intProcAddr = intpro[intProcNum]; /* interrupt procedure address */
        if (intProcAddr > 0) {                /* write intnumber symbolnum (4 base-32 digits) */
            putSym(intProcNum + '0' - 1);
            putSymInt(intProcAddr, 3);
            putSym('/');
        }
    }
    putSymStr("/\n");

    /*     reverse the symbol table pointers */
    /*     set the length field of compiler-generated labels to 1 */

    i         = symtop;
    j         = symbol[i]; // will have no symbol #
    symbol[i] = 0;
    while (j != 0) {
        /* note j is the symbol - 1 location. See symbol format */
        /* set length to 1 and prec to 5 (for comp generated labels) */
        if (info_type(j + 1) == LABEL && info_len(j + 1) == 0)
            symbol[Info(j + 1)] = MkInfo(1, CompilerLabel, LABEL);
        int m     = symbol[j];
        symbol[j] = i;
        i         = j;
        j         = m & 0xffff; // strip the symbol #
    }

    putSym('/');
    for (int j = 2; symbol[j]; j = symbol[j]) {
        /* note j is the symbol - 1 location. See symbol format */
        int symIdx = j + 1;
        int lp     = symbol[Info(symIdx)]; /* sym info */
        if (lp < 0) {
            lp = -lp;
            putSym('-');
        } else
            putSym(' ');
        putSymInt(lp, 0); // print the Info word

        if (symbol[Info(symIdx)] < 0) {
            lp = symbol[SymInts(symIdx) + sym_ints(symIdx)];
            if (lp < 0) {
                lp = -lp;
                putSym('-');
            } else
                putSym(' ');
            putSymInt(lp, 0); // print the length
        }
        putSym('/');
    }

    putSym('/');
    putSym('\n');
}

void recov() {
    for (;;) {
        /*     find something solid in the text */
        if (token == DECL || token == PROCV || token == ENDV || token == DOV || token == SEMIV ||
            token == EOFILE)
            for (;;) {
                /*     and in the stack */
                int top = pstack[sp];
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
                vartop = var[sp--].loc;
            }
        scan();
    }
}

// simple compare for bsearch
int intcmp(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

bool stack(/*const int q */) {
    int j;

    for (;;) {
        ;
        switch (getc1(pstack[sp], token)) {
        case 0:
            /*     illegal symbol pair */
            error("3: invalid PL/M statement. %s & %s not allowed together", tokens[pstack[sp]],
                     tokens[token]);
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
            j = TRI(pstack[sp - 1], pstack[sp], token);
            return bsearch(&j, c1tri, ASIZE(c1tri), sizeof(c1tri[0]), intcmp) != NULL;
        }
    }
}

bool prok(const int prd) {
    int lp, i, j, k, len;
    /*      context check of equal or imbedded right parts */
    switch (contc[prd]) {
    case 0:
        /*     no check required */
        return true;
    case 2:
        /*     left context check */
        k   = hdtb[prd] - nt;
        i   = pstack[sp - prlen[prd]];
        len = lefti[k - 1] + 1;
        lp  = lefti[k];
        if (len <= lp)
            for (j = len; j <= lp; j++)
                if (leftc[j] == i)
                    return true;
        break;
    default:
        assert(0); // will force alert shouldn't happen
    }
    return false;
}

void reduce() {
    /*     pack stack top */
    int j   = TRI(pstack[sp - 3], pstack[sp - 2], pstack[sp - 1]);

    int top = pstack[sp];

    for (int prd = prind[top - 1]; prd < prind[top]; prd++) {
        int m = right(j, 8 * (prlen[prd] - 1)); // m = significant items to reduce by
        if (m == prtb[prd] && prok(prd)) {
            mp = sp - prlen[prd] + 1;     // mp -> lowest item on stack
            synth(prdtb[prd], hdtb[prd]); // do the action
            sp         = mp;              // do the reduce
            pstack[sp] = hdtb[prd];
            vartop     = var[sp].loc;
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
            if (++sp >= MSTACK) {
                fatal("5: pass-1 parse stack overflow");
                break;
            }
            pstack[sp] = token;
            /*     insert accum into varc here */
            if (token == NUMBV)
                fixv[sp] = accumIVal;

            var[sp].loc = vartop;
            // for (;;) {
            for (int j = 0; j < acclen; j++) {
                varc[vartop] = accum[j];
                if (++vartop > MVAR) {
                    fatal("7: pass-1 table overflow");
                    vartop = 1;
                }
            }
            /* string constants may be continued over several buffers full */
            //     if (token != STRV || stype != CONT)
            //          break;
            //    scan();
            //}
            int i       = vartop - var[sp].loc;
            var[sp].len = i < 0 ? 1 : i;
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

void scan() {

    /*      global tables */
    /*     scan finds the next entity in the input stream */
    /*     the resulting item is placed into accum (of length */
    /*     acclen).  token is set to  the item found */
    bool warned = false;

    for (;;) {
        failsf = true;
        int ch;
        while (isspace(ch = gnc()))
            ;
        acclen = 0;
        if (ch == EOF) {
            token = EOFILE;
            return;
        }
        if (isdigit(ch)) { /*     number */
            token = NUMBV;
            for (;;) {
                if (acclen < MAXSTR)
                    accum[acclen++] = ch;
                else {
                    accum[acclen - 1] = ch; // allows for radix test
                    if (!warned) {
                        warned = true;
                        error("xx: numeric string too long");
                    }
                }
                while ((ch = gnc()) == '$') // gobble $ in number
                    ;
                ch = toupper(ch);
                if (!isxdigit(ch)) // not a hex digit
                    break;
            }
            /*     check radix */
            uint8_t radix;
            if (ch == 'H') // H
                radix = 16;
            else if (ch == 'Q' || ch == 'O') // Q or O
                radix = 8;
            else {
                if (accum[acclen - 1] == 'B') { // B
                    radix = 2;
                    acclen--;
                } else {
                    radix = 10;
                    if (accum[acclen - 1] == 'D') // D
                        acclen--;
                }
                decibp();
            }
            accumIVal = 0;
            if (warned)
                return;
            for (int i = 0; i < acclen; i++) {
                uint8_t digit = accum[i];
                digit         = isdigit(digit) ? digit - '0' : digit - 'A' + 10;
                if (digit >= radix || (accumIVal = accumIVal * radix + digit) >= 0x10000) {
                    error("6: number conversion error");
                    accumIVal = 0;
                    break;
                }
            }
            return;
        } else if (isalpha(ch)) { // alpha character
            token = IDENTV;       /*     identifier */
            while (isalnum(ch)) {
                if (acclen < 32)
                    accum[acclen++] = toupper(ch);
                while ((ch = gnc()) == '$') // gobble up $ in name
                    ;
            }
            decibp();
        } else if (!isalnum(ch)) { // non alpha numeric
            if (ch == '\'') {      // quote
                token = STRV;
                for (;;) {
                    while ((ch = gnc()) == '\n') // ignore nl
                        ;
                    if (ch == '\'') {
                        while ((ch = gnc()) == '\n') // allow '' to straddle line break
                            ;
                        if (ch != '\'') // nope wasn't '' so we have all of the string
                            break;
                    }
                    if (ch < ' ')
                        ch = ' ';
                    if (acclen < MAXSTR)
                        accum[acclen++] = C_UPPER ? toupper(ch) : ch; // stuff char (double quote reduced to single)
                    else if (!warned) {
                        error("xx: string too long, possibly missing closing quote");
                        warned = true;
                    }
                }
                decibp(); // backup one
                return;
            } else {
                acclen   = 1;
                accum[0] = ch;
                token    = 0;
                if (ch == '/') { // look for comment
                    if ((ch = gnc()) != '*')
                        decibp();
                    else { // got a comment
                        while (ch != '/') {
                            /*     comment found */
                            while (ch != EOF && (ch = gnc()) != '*')
                                ;
                            while (ch == '*')
                                ch = gnc();
                            if (ch == EOF) {
                                token = EOFILE;
                                return;
                            }
                        }
                        continue; // comment skipped try again
                    }
                }
            }
        }
        if (acclen < sizeof(vindx) - 1) {
            /*     search for token in vocabulary */
            for (int i = vindx[acclen]; i < vindx[acclen + 1]; i++) {
                if (memcmp(accum, tokens[i], acclen) == 0) {
                    token = i;
                    return;
                }
            }
        }
        // if we get here it is not a keyword/keysymbol
        // if its an identifier and not used as a macro return it
        if (token == IDENTV && !useMacro(acclen, accum))
            return;
        // if its a single character then its spurious
        if (token == 0) {
            error("8: illegal symbol");
        }
    }
}

int wrdata(const int sy) {
    /*     if sy is negative, the call comes from synth -- data is inserted */
    /*     inline by calling LIT with each byte value. */

    /*     if sy is positive, the call comes from dumpin -- */
    /*     wrdata writes data into the output file from symbol at location */
    /*     'sy'  each byte value is written as a pair of base 32 digits. */
    /*     the high order bit of the first digit is 1, and all remaining high */
    /*     order digits are zero. the value returned by wrdata is the total */
    /*     number of bytes written. */
    /*      global tables */
    uint32_t symIdx = iabs(sy);

    /*     check precision of value */
    /*     set dflag to true if we are dumping a variable or LABEL name */
    int type   = info_type(symIdx);
    bool dflag = type == LABEL || type == VARB || type == PROC;

    int len    = info_prec(symIdx);
    if (len <= 2 && !dflag) { // single or double byte constant
        uint16_t n = info_len(symIdx);
        if (len) {
            if (sy < 0) {
                if (len == 2)
                    emit(HIGHBYTE(n), LIT);
                emit(LOWBYTE(n), LIT);
            } else {
                if (len == 2)
                    putSymHex2(HIGHBYTE(n), true);
                putSymHex2(LOWBYTE(n), len != 2);
            }
        }
    } else {

        len = sym_len(symIdx);
        uint8_t unpacked[5];

        for (int lp = 0; lp < len; lp++) {
            if (lp % PACK == 0) {
                uint32_t m = symbol[SymInts(symIdx) + lp / PACK];
                for (int i = PACK - 1; i >= 0; i--) {
                    unpacked[i] = m & 0xff; // unpack the 6 bits
                    m >>= 8;                // shift right
                }
            }
            if (dflag) /*     write out the variable or LABEL name */
                putSym(unpacked[lp % PACK]);
            else {
                uint8_t i = unpacked[lp % PACK];
                /*    write out both hex values */
                if (sy < 0) /*     emit string data inline */
                    emit(i, LIT);
                else
                    putSymHex2(i, lp == 0);
            }
        }
    }
    return len;
}

void dumpch() {
    /*     dump the symbolic names for the simulator */

    putSym('/');
    if (C_MEMORY) {
        int i = 2;
        for (int symNumber = 1; i; symNumber++, i = symbol[i]) {
            int symIdx = i + 1;
            if (symbol[Info(symIdx)] >= 0) {
                int type = info_type(symIdx);
                if (type == LABEL || type == VARB || type == PROC) {
                    /* check if real symbol */
                    if (sym_ints(symIdx)) {
                        /* write symbol number */
                        putSymInt(symNumber, 3);
                        /* now write the string */
                        wrdata(symIdx);
                        putSym('/');
                    }
                }
            }
        }
    }
    putSymStr("/\n");
}

void procHead(int ptype, int plist, int rtype) {
    proctp[curblk]       = ptype; // 1 untyped, 2 typed
    int symIdx           = fixv[mp];
    symbol[Info(symIdx)] = MkInfo(plist, rtype, PROC);
    int node             = enter(-MkInfo(0, LocalLabel, LABEL));
    fixv[mp] += node << 15;
    emit(id_num(node), VLU);
    emit(TRA, OPR);
    emit(id_num(symIdx), DEF);
    return;
}

void synth(const int prod, const int symm) {
    int ip, i, j, k, len, length;
    int symIdx;

    /*    mp == left ,  sp == right */

    /*      global tables */
    if (C_ANALYZE)
        redpr(prod, symm);
    switch (prod) {
    case 1: // <PROGRAM> ::= <STATEMENT LIST>
        if (mp != 5)
            error("10: invalid program. Possibly missing END");
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
                emit(id_num(symIdx), ADR);
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
        i = fixv[mp];
        emit(id_num(i), DEF);
        symbol[Info(i)] = MkInfo(0, LocalLabel, LABEL);
        return;
    case 20: // <IF STATEMENT> ::= <LABEL DEFINITION> <IF STATEMENT>
        return;
    case 21: // <IF CLAUSE> ::= IF <EXPRESSION> THEN
        i = enter(-MkInfo(0, 0, LABEL));
        emit(id_num(i), VLU);
        emit(TRC, OPR);
        fixv[mp] = i;
        return;
    case 22: // <TRUE PART> ::= <BASIC STATEMENT> ELSE
        i = enter(-MkInfo(0, 0, LABEL));
        emit(id_num(i), VLU);
        emit(TRA, OPR);
        j            = fixv[mp - 1];
        fixv[mp - 1] = i;
        i            = j;
        emit(id_num(i), DEF);
        symbol[Info(i)] = MkInfo(0, LocalLabel, LABEL);
        return;
    case 23: // <GROUP> ::= <GROUP HEAD> <ENDING>
        if (fixv[sp] > 0)
            error("12: improper yse of identifier following END");
        else if (fixc[sp] < 0)
            fixc[mp] = 0;

        switch (dopar[curblk + 1].type) {
        case DO_GROUP:
            emit(END, OPR);
            return;
        case DO_ITER:
            /*     end of iterative statement */
            k = fixv[mp];
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
                emit(id_num(symIdx), DEF);
                int endJmp = info_len(symIdx);
                symbol[Info(symIdx)] &= 0xff;
                /*     m is symbol number of LABEL at end of jump table */
                emit(CSE, OPR);
                /*     define the jump table */
                /*     reverse the LABEL list */
                int pIdx = 0, qIdx;
                for (symIdx = dopar[curblk + 1].label; symIdx; pIdx = symIdx, symIdx = qIdx) {
                    qIdx = info_len(symIdx);
                    symbol[Info(symIdx)] &= 0xff; // clear the length field
                    symbol[Info(symIdx)] += (pIdx << 8);
                }

                do { /* emit list starting at pIdx */
                    qIdx               = info_len(pIdx);
                    symbol[Info(pIdx)] = MkInfo(0, LocalLabel, LABEL);
                    if (qIdx) {
                        emit(id_num(pIdx), VLU);
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
        doHead(curblk, DO_ITER, fixv[mp + 1]);
        return;
    case 26: // <GROUP HEAD> ::= DO <WHILE CLAUSE> ';'
        enterb();
        doHead(curblk, DO_WHILE, fixv[mp + 1]);
        return;
    case 27: // <GROUP HEAD> ::= DO <CASE SELECTOR> ';'
        enterb();
        k = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        /*     k is LABEL after case jump table */
        symIdx = enter(-MkInfo(k, LocalLabel, LABEL));
        emit(id_num(symIdx), VLU);
        emit(AX1, OPR);
        doHead(curblk, DO_CASE, symIdx);
        symIdx              = enter(-MkInfo(0, LocalLabel, LABEL));
        dopar[curblk].label = symIdx;
        emit(id_num(symIdx), DEF);
        return;
    case 28:                                 // <GROUP HEAD> ::= <GROUP HEAD> <STATEMENT>
        if (dopar[curblk].type == DO_CASE) { // case stmt
            emit(info_len(dopar[curblk].clause), VLU);
            emit(TRA, OPR);
            symIdx              = enter(-MkInfo(dopar[curblk].label, LocalLabel, LABEL));
            dopar[curblk].label = symIdx;
            emit(id_num(symIdx), DEF);
        }
        return;
    case 29: // <STEP DEFINITION> ::= <VARIABLE> <REPLACE> <EXPRESSION> <ITERATION CONTROL>
        j = fixv[mp + 3];
        /*     place <variable> symbol number into do slot */
        fixv[mp - 1] = j >= 0 ? 0 : fixv[mp];
        fixv[mp]     = iabs(j);
        return;
    case 30: // <ITERATION CONTROL> ::= <TO> <EXPRESSION>
        emit(LEQ, OPR);
        i = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        emit(i, VLU);
        emit(TRC, OPR);
        fixv[mp] = -((fixv[mp] << 14) + i);
        return;
    case 31: // <ITERATION CONTROL> ::= <TO> <EXPRESSION> <BY> <EXPRESSION>
        i = fixv[mp - 3];
        /*     i = symbol number of indexing variable */
        emit(i, VLU);
        emit(ADD, OPR);
        emit(i, ADR);
        emit(STD, OPR);
        /*     branch to compare */
        i = fixv[mp + 2];
        emit((i >> 14), VLU);
        emit(TRA, OPR);
        /*     define beginning of statements */
        emit(right(i, 14), DEF);
        return;
    case 32: // <WHILE CLAUSE> ::= <WHILE> <EXPRESSION>
        i        = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        fixv[mp] = (fixv[mp] << 14) + i;
        /*     (back branch number/end loop number) */
        emit(i, VLU);
        emit(TRC, OPR);
        return;
    case 33: // <CASE SELECTOR> ::= CASE <EXPRESSION>
        return;
    case 34: // <PROCEDURE DEFINITION> ::= <PROCEDURE HEAD> <STATEMENT LIST> <ENDING>
        k = (fixv[mp] >> 15);
        j = fixv[sp];
        if (j < 0)
            j = -j + 1;
        if (j != 0 && right(fixv[mp], 15) != j)
            error("13: identifier following END does not match");
        emit(END, OPR);
        /*     emit a ret just in case he forgot it */
        emit(DRT, OPR);
        emit(id_num(k), DEF);
        return;
    case 35: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> ';'
        procHead(1, 0, 0);
        return;
    case 36: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> <TYPE> ';'
        procHead(2, 0, fixv[sp - 1]);
        return;
    case 37: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> <PARAMETER LIST> ';'
        procHead(1, fixv[mp + 1], 0);
        return;
    case 38: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> <PARAMETER LIST> <TYPE> ';'
        procHead(2, fixv[mp + 1], fixv[sp - 1]);
        return;
    case 39: // <PROCEDURE HEAD> ::= <PROCEDURE NAME> INTERRUPT <NUMBER> ';'
        /*     get symbol number */
        i = id_num(fixv[mp]);
        /*     get interrupt number */
        j = fixv[sp - 1];
        if (j > 7)
            error("39: invalid interrupt number");
        else {
            k = intpro[++j];
            if (k <= 0)
                intpro[j] = i;
            else
                error("40: duplicate interrupt procedure");
        }
        procHead(1, 0, 0);
        return;
    case 40: // <PROCEDURE NAME> ::= <LABEL DEFINITION> PROCEDURE
             /* check for numeric label */
        if (fixc[mp] >= 0) {
            fixv[mp] = fixc[mp];
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
        enter(MkInfo(0, 0, VARB));
        fixv[mp]++;
        return;
    case 42: // <PARAMETER HEAD> ::= '(' Left context check(<PROCEDURE NAME>)
        fixv[mp] = 0;
        return;
    case 44: // <ENDING> ::= END
        exitb();
        fixv[mp] = 0;
        return;
    case 45: // <ENDING> ::= END <IDENTIFIER>
        exitb();
        if (!(symIdx = lookup(sp)))
            error("15: identifier following END not found");
        fixv[mp] = symIdx;
        return;
    case 46: // <ENDING> ::= <LABEL DEFINITION> <ENDING>
        fixv[mp] = fixv[sp];
        return;
    case 47: // <LABEL DEFINITION> ::= <IDENTIFIER> ':'
        symIdx = lookup(mp);
        if (symIdx < blksym)

            /*         prec = 3 if user-defined outer block LABEL */
            /*         prec = 4 if user-defined LABEL not in outer block */
            /*         prec = 5 if compiler-generated LABEL */
            symIdx = enter(MkInfo(0, curblk == 2 ? OuterLabel : LocalLabel, LABEL));
        else {
            if (info_prec(symIdx)) {
                error("16: duplicate LABEL definition in block");
                symbol[Info(symIdx)] &= ~(0xF << 4); // clear the prec bits
            }
            // set the prec bits
            symbol[Info(symIdx)] += MkInfo(0, curblk == 2 ? OuterLabel : LocalLabel, 0);
        }
        fixv[mp] = symIdx;
        /* indicate that this is an identifier label */
        fixc[mp] = -1;
        if (token != PROCV)
            emit(id_num(symIdx), DEF);
        return;
    case 48: // <LABEL DEFINITION> ::= <NUMBER> ':'
        k = fixv[mp];
        if (k > 65535)
            error("17: numeric LABEL too big");
        else {
            if ((symIdx = lookup(mp)) == 0)
                /*     enter number */
                symIdx = enter(MkInfo(k, k > 255 ? 2 : 1, NUMBER));

            /* indicate that this is a numeric label */
            fixc[mp] = symIdx;
            emit(id_num(symIdx), VLU);
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
        if ((symIdx = fixv[sp]) == 0)
            return;
        if (symIdx > 0) {
            int type = info_type(symIdx);
            emit(id_num(symIdx), ADR);
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
            symIdx = enter(MkInfo(0, 0, LABEL));
        j = info_type(symIdx);
        if (j == LABEL || j == VARB) {
            /*     increment the reference counter (use length field) */
            if (j == LABEL)
                symbol[Info(symIdx)] += MkInfo(1, 0, 0);
            emit(id_num(symIdx), VLU);
            emit(TRA, OPR);
        } else
            error("19: invalid destination for GOTO");
        return;
    case 53: // <GO TO STATEMENT> ::= <GO TO> <NUMBER>
        k = fixv[sp];
        if (k > 65535) {
            error("17: numeric LABEL too big");
        } else {
            if ((symIdx = lookup(sp)) == 0)
                /*     enter number */
                symIdx = enter(MkInfo(k, k > 255 ? 2 : 1, NUMBER));
            emit(id_num(symIdx), VLU);
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
        if (!defMacro(var[mp].len, &varc[var[mp].loc], var[sp].len, &varc[var[sp].loc]))
            fatal("20: macro table overflow");
        return;
    case 60: // <DECLARATION ELEMENT> ::= <IDENTIFIER> <DATA LIST>
        i   = fixv[mp] + 1;
        j   = fixv[mp + 1];
        len = right(j, 16);
        symbol[i] += (len << 8); // PMO
        emit(DAT, OPR);
        emit((j >> 16), DEF);
        return;
    case 61: // <DATA LIST> ::= <DATA HEAD> <CONSTANT> ')'
    case 63: // <DATA HEAD> ::= <DATA HEAD> <CONSTANT> ','
        fixv[mp] += wrdata(-fixv[mp + 1]);
        return;
    case 62: // <DATA HEAD> ::= DATA '('
        j = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        emit(j, VLU);
        emit(TRA, OPR);
        fixv[mp] = (j << 16);
        if (lookup(mp - 1) > blksym)
            error("22: duplicate variable declaration");
        /*     set precision of inline data to 3 */
        symIdx       = enter(MkInfo(0, P_INLINE, VARB));
        fixv[mp - 1] = symIdx;
        emit(DAT, OPR);
        emit(id_num(symIdx), DEF);
        return;
    case 64: // <TYPE DECLARATION> ::= <IDENTIFIER SPECIFICATION> <TYPE>
    case 65: // <TYPE DECLARATION> ::= <BOUND HEAD> <NUMBER> ')' <TYPE>
        length = prod == 64 ? 1 : fixv[mp + 1]; // dimension

        k      = fixv[sp]; // item type
        do {
            int symIdx = idList[--idListSP];
            ip         = symbol[Info(symIdx)];
            if (k == P_LABEL) {
                if (ip != VARB)
                    error("21: duplicate variable or LABEL definition");
                ip = LABEL;
            }
            symbol[Info(symIdx)] = MkInfo(length, k, symType(ip));
            if (ip < 0)
                symbol[Info(symIdx)] = -symbol[Info(symIdx)];
        } while (idListSP);
        fixv[mp] = idList[0];
        return;
    case 66: // <TYPE DECLARATION> ::= <TYPE DECLARATION> <INITIAL LIST>
        return;
    case 67: // <TYPE> ::= BYTE
        fixv[mp] = P_BYTE;
        return;
    case 68: // <TYPE> ::= ADDRESS
        fixv[mp] = P_ADDRESS;
        return;
    case 69: // <TYPE> ::= LABEL
        fixv[mp] = P_LABEL;
        return;
    case 70: // <BOUND HEAD> ::= <IDENTIFIER SPECIFICATION> '('
        return;
    case 71:                           // <IDENTIFIER SPECIFICATION> ::= <VARIABLE NAME>
        idList[idListSP++] = fixv[mp]; // single so no need to check for overflow
        return;
    case 72: // <IDENTIFIER SPECIFICATION> ::= <IDENTIFIER LIST> <VARIABLE NAME> ')'
    case 74: // <IDENTIFIER LIST> ::= <IDENTIFIER LIST> <VARIABLE NAME> ','
        if (idListSP < MAXIDLIST)
            idList[idListSP++] = fixv[mp + 1];
        else
            fatal("23: pass-1 factored list overflow");

        return;
    case 73: // <IDENTIFIER LIST> ::= '('
        return;
    case 75: // <VARIABLE NAME> ::= <IDENTIFIER> Left context check(',' | DECLARE |
             // <IDENTIFIER LIST>)
    case 77: // <BASED VARIABLE> ::= <IDENTIFIER> BASED
        symIdx = lookup(mp);
        if (symIdx <= blksym)
            symIdx = enter(MkInfo(0, 0, VARB));
        else if ((symbol[Info(symIdx)] & 0xff) != VARB)
            error("24: invalid variable identifier");
        fixv[mp] = symIdx;
        return;
    case 76: // <VARIABLE NAME> ::= <BASED VARIABLE> <IDENTIFIER>
        if (symtop >= SYMABS)
            fatal("25: pass-1 symbol table overflow");
        else {
            i = fixv[mp];
            symbol[symtop + 1] =
                symbol[symtop]; // copy up location of top symbol (stored in symbol[symtop]
            j      = symtop++;
            symIdx = lookup(sp);
            if (symIdx == 0)
                symIdx = enter(MkInfo(0, 0, VARB));
            else if (info_type(symIdx) != VARB) {
                error("26: badly formed BASED variable declaration");
                return;
            }

            symbol[j]       = id_num(symIdx);   // stuff the symId of the base var
            symbol[Info(i)] = -symbol[Info(i)]; // mark the info as based
        }
        return;
        /*
         * Modified from original code
         * separate space for initial data, now built upwards see notes
         * just before defintition of initialData variable
         */

    case 79: // <INITIAL HEAD> ::= INITIAL '('
        if (initialDataSP < MAXINITIAL) {
            initialData[initialDataSP] = id_num(fixv[mp - 1]) << 15;
            fixv[mp]                   = initialDataSP++;
        } else
            fatal("23: pass-1 initial data table overflow");
        return;
    case 78: // <INITIAL LIST> ::= <INITIAL HEAD> <CONSTANT> ')'
    case 80: // <INITIAL HEAD> ::= <INITIAL HEAD> <CONSTANT> ','
        if (initialDataSP < MAXINITIAL) {
            initialData[fixv[mp]]++; // bump the count
            initialData[initialDataSP++] =
                (id_num(fixv[mp + 1]) << 16) + fixv[mp + 1]; // store the value
        } else
            fatal("23: pass-1 initial data table overflow");
        return;

    case 81: // <ASSIGNMENT> ::= <VARIABLE> <REPLACE> <EXPRESSION>
    case 82: // <ASSIGNMENT> ::= <LEFT PART> <ASSIGNMENT>
        if (++acnt > MAXASSIGN) {
            fatal("50: too many chained assignments");
            acnt = 0;
        } else {
            assign[acnt] = fixv[mp];
            /*      check for procedure on lhs of assignment. */
            /*     ****note that this is dependent on symbol number of output=17**** */
            if (fixv[mp] == 0 && fixc[mp] != 17)
                error("41: procedure on left-hand side of an assignment");
        }
        return;
    case 83: // <REPLACE> ::= '='
    case 84: // <LEFT PART> ::= <VARIABLE> ','
    case 85: // <EXPRESSION> ::= <LOGICAL EXPRESSION>
        return;
    case 86: // <EXPRESSION> ::= <VARIABLE> ':' '=' <LOGICAL EXPRESSION>
        symIdx = fixv[mp];
        if (fixv[mp] == 0)
            error("41: procedure on left-hand side of an assignment");
        else if (symIdx < 0) // PMO added 'else' as symIdx = 0 will cause memory error
            emit(XCH, OPR);
        else
            emit(id_num(symIdx), ADR);

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
        emit(fixv[mp + 1], OPR);
        return;
    case 96: // <RELATION> ::= '=' Left context check(<ARITHMETIC EXPRESSION>)
        fixv[mp] = EQL;
        return;
    case 97: // <RELATION> ::= '<'
        fixv[mp] = LSS;
        return;
    case 98: // <RELATION> ::= '>'
        fixv[mp] = GTR;
        return;
    case 99: // <RELATION> ::= '<' '>'
        fixv[mp] = NEQ;
        return;
    case 100: // <RELATION> ::= '<' '='
        fixv[mp] = LEQ;
        return;
    case 101: // <RELATION> ::= '>' '='
        fixv[mp] = GEQ;
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
        symIdx = fixv[mp];
        emit(id_num(symIdx), VLU);
        return;
    case 113: // <PRIMARY> ::= '.' <CONSTANT>
        i        = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        fixv[mp] = i;
        emit(i, VLU);
        emit(TRA, OPR);
        emit(DAT, OPR);
        emit(0, DEF);
        /*     drop through to next production */
    case 114: // <PRIMARY> ::= <CONSTANT HEAD> <CONSTANT> ')'
        wrdata(-fixv[mp + 1]);
        emit(DAT, OPR);
        emit(fixv[mp], DEF);
        return;
    case 115: // <PRIMARY> ::= <VARIABLE>
        symIdx = fixv[mp];
        if (symIdx > 0) {
            /*     simple variable */
            emit(id_num(symIdx), VLU);
            int type = info_type(symIdx);
            if (type == PROC)
                emit(PRO, OPR);
            else if (type == INTR)
                emit(BIF, OPR);
        } else if (symIdx != 0)
            /*     subscripted variable */
            emit(LOD, OPR);
        return;
    case 116: // <PRIMARY> ::= '.' <VARIABLE>
        symIdx = fixv[sp];
        if (symIdx > 0) {
            if (info_type(symIdx) == VARB) {
                emit(id_num(symIdx), ADR);
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
        symIdx   = enter(-MkInfo(0, LocalLabel, LABEL));
        i        = id_num(symIdx);
        fixv[mp] = i;
        emit(i, VLU);
        emit(TRA, OPR);
        emit(DAT, OPR);
        emit(0, DEF);
        return;
    case 119: // <CONSTANT HEAD> ::= <CONSTANT HEAD> <CONSTANT> ','
        wrdata(-fixv[mp + 1]);
        return;
    case 120: // <VARIABLE> ::= <IDENTIFIER>
        if ((symIdx = lookup(mp)) == 0) {
            error("29: undeclared variable");
            symIdx = enter(MkInfo(0, 0, VARB));
        }
        fixv[mp] = symIdx;
        j        = info_type(symIdx);
        if (j == LABEL)
            error("47: illegal use of a LABEL");
        if (j != PROC && j != INTR)
            return;
        if (info_len(symIdx) != 0)
            error("38: too few parameters");
        j = info_prec(symIdx);
        if (pstack[mp - 1] == CALLV && j != 0)
            error("42: attempted CALL of a typed procedure");
        if (pstack[mp - 1] != CALLV && j == 0)
            error("43: attempted use of untyped procedure as a function or variable");
        i        = id_num(symIdx);
        i        = ((i << 15) + i + 1);
        fixc[mp] = 0;
        emit(i >> 15, VLU);
        fixc[mp] = i >> 15;
        fixv[mp] = 0;
        emit(PRO, OPR);
        return;
    case 121: // <VARIABLE> ::= <SUBSCRIPT HEAD> <EXPRESSION> ')'
        i = fixv[mp];
        if (i >= 0) {
            fixv[mp] = -i;
            emit(INX, OPR);
            return;
        } else {
            i = -i;
            emit(right(i, 15), ADR);
            if (fixc[mp] != 1)
                emit(STD, OPR);
            if (iabs(fixc[mp]) == 0)
                error("37: too many actual parameters");
            if (iabs(fixc[mp]) > 1)
                error("38: too few parameters");
        }
        emit(i >> 15, VLU);
        fixc[mp] = i >> 15;
        fixv[mp] = 0;
        emit(PRO, OPR);
        return;
    case 122: // <SUBSCRIPT HEAD> ::= <IDENTIFIER> '('
        if ((symIdx = lookup(mp)) == 0) {
            error("30: subscripted variable or procedure call references undeclared "
                     "identifier");
            symIdx = enter(MkInfo(0, 0, VARB));
        }
        j = info_type(symIdx);
        if (j != VARB) {
            if (j != PROC && j != INTR)
                error("31: Identifier is improperly used as a procedure or subscripted "
                         "variable");
            else {
                fixc[mp] = info_len(symIdx);
                if (j == INTR)
                    fixc[mp] = -fixc[mp];
                j = info_prec(symIdx);
                /*     in the statements below, 30 is the token for 'call' */
                if (pstack[mp - 1] == 30 && j != 0)
                    error("42: attempted CALL of a typed procedure");
                if (pstack[mp - 1] != 30 && j == 0)
                    error("43: attempted use of untyped procedure as a function or variable");
                i        = id_num(symIdx);
                fixv[mp] = -((i << 15) + i + 1);
                return;
            }
        }
        fixv[mp] = symIdx;
        emit(id_num(symIdx), ADR);
        return;
    case 123: // <SUBSCRIPT HEAD> ::= <SUBSCRIPT HEAD> <EXPRESSION> ','
        i = -fixv[mp];
        if (i <= 0)
            error("32: too many subscripts. Only one allowed");
        else {
            fixv[mp] = -(i + 1);
            j        = right(i, 15);
            emit(j, ADR);
            if (fixc[mp] == 0)
                error("37: too many actual parameters");
            else {
                if (fixc[mp] != 2)
                    emit(STD, OPR);
                fixc[mp] += (fixc[mp] < 0) ? 1 : -1;
            }
        }
        return;
    case 124: // <CONSTANT> ::= <STRING>
        /*     may wish to treat this string as a constant later */
        len = var[sp].len;
        if (0 < len && len <= 2) {
            j = var[sp].loc;
            k = varc[j];
            if (len == 2)
                k = k * 256 + varc[j + 1];
        } else {
            len = 3;
            k   = 0;
        }
        if ((symIdx = lookup(sp)) == 0)
            symIdx = enter(MkInfo(k, len, LITER));
        fixv[mp] = symIdx;
        return;
    case 125: // <CONSTANT> ::= <NUMBER>
        if ((symIdx = lookup(sp)) == 0) {
            /*     enter number into symbol table */
            i      = fixv[mp];
            symIdx = enter(MkInfo(i, i > 255 ? 2 : 1, NUMBER));
        }
        fixv[mp] = symIdx;
        return;
    case 126: // <TO> ::= TO
        symIdx = fixv[mp - 3];
        if (symIdx <= 0) {
            error("33: interative DO index is invalid");
            fixv[mp] = 1;
        } else {
            i            = id_num(symIdx);
            fixv[mp - 3] = i;
            emit(i, ADR);
            emit(STD, OPR);
            j = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
            emit(j, DEF);
            fixv[mp] = j;
            emit(i, VLU);
        }
        return;
    case 127: // <BY> ::= BY
        emit(LEQ, OPR);
        i = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        /*     save symbol number at <to> (end loop number) */
        j            = fixv[mp - 2];
        fixv[mp - 2] = i;
        emit(i, VLU);
        emit(TRC, OPR);
        i        = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        fixv[mp] = (j << 14) + i;
        /*     <by> is (to number/statement number) */
        emit(i, VLU);
        emit(TRA, OPR);
        /*     now define by LABEL */
        i = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        /*     save by LABEL in <to> as branch back number */
        fixv[mp - 2] += i << 14;
        emit(i, DEF);
        return;
    case 128: // <WHILE> ::= WHILE
        i = id_num(enter(-MkInfo(0, LocalLabel, LABEL)));
        emit(i, DEF);
        fixv[mp] = i;
        return;
    }
    assert(0);
}

#define INMAX 256
static uint8_t ibuff[INMAX + 1];
char *ibptr = "";

int gnc(/*const int q */) {
    int ch;

    if ((ch = macGetc())) // get next character from macro if available
        return ch;
    /*     get next character from the input stream (or 0 if */
    /*     no character is found) */

    while (!*ibptr) {
        for (;;) {
            if ((ibptr = fgets((char *)ibuff, INMAX - 1, srcFp)))
                break; // got a line
            if (inSP < 1)
                return EOF; // no more input
            fclose(srcFp);
            srcFp = instk[inSP--];
        }
        emit(++lineCnt, LIN);
        if (!strchr(ibuff, '\n')) { // gobble up rest of line
            fprintf(lstFp, "WARNING: line %d truncated\n", lineCnt);
            while ((ch = getc(srcFp)) != '\n' && ch != EOF)
                ;
            strcat(ibuff, "\n"); // ensure line ends with newline
        }
        char *s;
        if ((s = strchr(ibuff, '\r')) && s[1] == '\n')
            strcpy(s, "\n");
        else if (!strchr(ibuff, '\n'))
            strcat(ibuff, "\n"); // ensure line ends with newline

        if (C_PRINT != 0) {
            int column =
                fprintf(lstFp, "%05d%3d %c  ", (uint16_t)lineCnt, curblk - 1, inSP > 0 ? '=' : ' ');
            for (s = ibuff; *s != '\n'; s++, column++) {
                if (column == C_DELETE - 1 && s[1] != '\n')
                    column = fprintf(lstFp, "\\\n%12s", "");
                putc(*s, lstFp);
            }
            putc('\n', lstFp);
        }
        if (*ibptr == '$') {
            parseOptions(ibuff + 1); // parse the options
            ibptr = "";
        }
    }

    return *ibptr++;
}

void parseOptions(uint8_t *s) {

    while (*s && *s != '\n') {     // process all of line
        while (*s && !isalpha(*s)) // skip to option
            s++;
        if (!*s)
            return;
        int option = toupper(*s);
        while (isalpha(*s)) // skip over option long name
            s++;
        while (isspace(*s))
            s++;
        int val = contrl[option - 'A'];
        if (*s == '=') {
            while (isspace(*++s))
                ;
            if (option == 'I') {
                char *incName = s;
                while (*s && !isspace(*s))
                    s++;
                *s = 0;
                if (strlen(incName) == 0)
                    error("36: include file name missing");
                else
                    stackc(incName); // push include file on stack
                return;
            }
            val = 0;
            if (!isdigit(*s))
                error("51: missing value after = on $ control line");
            else
                while (isdigit(*s)) // get value
                    val = val * 10 + *s++ - '0';
        } else if (option == 'I') {
            error("36: include file name missing");
            return;
        } else if (val == 0 || val == 1)
            val = !val; // toggle the value
        else
            error("34: attempt to toggle non binary $ control");
        contrl[option - 'A'] = val;
    }
}

void decibp() {
    if (!macBackup())
        ibptr--;
}

int imin(const int i, const int j) {
    return i < j ? i : j;
}

void stackc(char *fname) {
    if (inSP >= 7)
        fatal("35: too many include files");
    else {
        FILE *incFp = fopen(makeFileName(fname, ".plm", false), "rt");
        if (!incFp)
            fprintf(stderr, "can't open include file %s. Skipping\n", fname);
        else {
            instk[++inSP] = srcFp;
            srcFp         = incFp;
        }
    }
}

char b32Digit(int ch) {
    /*     convert a byte to a base 32 digit */
    ch %= 32;
    return ch + (ch < 10 ? '0' : 'A' - 10);
}

void enterb() {
    /*     entry to block goes through here */
    curblk++;
    proctp[curblk] = proctp[curblk - 1];
    if (curblk > MAXBLK) {
        fatal("36: too many block levels");
        curblk = 1;
    }
    block[curblk] = symtop;
    doHead(curblk, 0, 0);
    /*     save the state of the macro definition table */
    macblk[curblk] = macdefSP;

    blksym         = symtop;
    return;
}

void dumpin() {
    /*     dump the initialization table */
    /*     wrdata(x) writes the data at location x in symbol table */
    /*     and returns the number of bytes written */
    if (C_SYMBOLS == 2) {

        for (int i = 0; i < initialDataSP;) {
            fprintf(lstFp, "\nSYMBOL S%05d =", initialData[i] >> 15);
            int col = 15;
            for (int jp = initialData[i++] & 0x7fff; jp > 0; jp--) {
                if (col + 7 > C_DELETE) {
                    fprintf(lstFp, "\n%.*s", 15, "");
                    col = 15;
                }
                /*         get the symbol number */
                col += fprintf(lstFp, " S%05d", initialData[i++] >> 16);
            }
        }
    }
    putc('\n', lstFp);
    /*     ready to write the initialization table */
    putSym('/');

    for (int i = 0; i < initialDataSP;) {
        putSymInt(initialData[i] >> 15, 3);
        for (int jp = initialData[i++] & 0x7fff; jp > 0; jp--)
            wrdata(initialData[i++] & 0xffff);
        putSym('/');
    }
    putSym('/');
    putSym('\n');

    return;
}

void fatal(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(lstFp, "Fatal Error: ");
    vfprintf(lstFp, fmt, args);
    fprintf(lstFp, "\nCompilation Terminated\n");
    fprintf(stderr, "Fatal Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\nCompilation Terminated\n");
    compil = false;
    va_end(args);
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


int right(const int i, const int j) {
    return i % (1 << j);
}

void showTopTokens(int start, int indent) {
    int col = indent;
    for (int i = start; i <= sp; i++) {
        if (col + strlen(tokens[pstack[i]]) > C_DELETE)
            col = fprintf(lstFp, "\n%.*s", indent, "") - 1;
        col += fprintf(lstFp, " %s ", tokens[pstack[i]]);
    }
    putc('\n', lstFp);
}

void sdump() {
    /*     check for stack dump bypass */
    if (C_BYPASS == 0) {
        int indent = fprintf(lstFp, "PARSE STACK:");
        showTopTokens(5, indent);
    }
}

void redpr(const int prod, const int sym) {
    int indent = fprintf(lstFp, "%5d  %s ::=", prod, tokens[sym]);
    showTopTokens(mp, indent);
}

void emit(const int val, const int typ) {

#define MAXPOL 30
    static int polish[MAXPOL + 1];
    static int polcnt     = 0;
    static char *polchr[] = { "OPR", "ADR", "VAL", "DEF", "LIT", "LIN" };
    static char *opcval[] = { "NOP", "ADD", "ADC", "SUB", "SBC", "MUL", "DIV", "REM", "NEG",
                              "AND", "IOR", "XOR", "NOT", "EQL", "LSS", "GTR", "NEQ", "LEQ",
                              "GEQ", "INX", "TRA", "TRC", "PRO", "RET", "STO", "STD", "XCH",
                              "DEL", "DAT", "LOD", "BIF", "INC", "CSE", "END", "ENB", "ENP",
                              "HAL", "RTL", "RTR", "SFL", "SFR", "HIV", "LOV", "CVA", "ORG",
                              "DRT", "ENA", "DIS", "AX1", "AX2", "AX3" };

    /*     typ      meaning */
    /*      0      operator */
    /*      1      load address */
    /*      2      load value */
    /*      3      define location */
    /*      4      literal value */
    /*      5      line number */
    /*      6      unused */
    /*      7        " */
    assert(val >= 0);

    if (C_GENERATE != 0) {
        fprintf(lstFp, "%5d %s ", polcnt, polchr[typ]);
        switch (typ) {
        case OPR:
            fprintf(lstFp, opcval[val]);
            break;
        case ADR:
        case VLU:
        case DEF:
            fprintf(lstFp, "S%05d", val);
            break;
        case LIT:
        case LIN:
            fprintf(lstFp, " %05d", val);
        }
        /*     now store the polish element in the polish array. */
        putc('\n', lstFp);
    }

    uint16_t pol = (val << 3) + typ;
    fwrite(&pol, sizeof(pol), 1, polFp); // write the polish element
    return;
}

void cmpuse() {
    printf("table usage in pass 1:\n");
    printf("symbol table - top=%-6d, loc=%-6d\n", symtop, symloc);
    printf("               len=%-6d, cnt=%-6d, abs=%-6d\n", symlen, symcnt, SYMABS);
    printf("macro table  - maxStr=%d, maxDef=%d\n", maxMacStr, maxMacDef);
}
