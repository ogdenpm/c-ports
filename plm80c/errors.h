/****************************************************************************
 *  errors.h: part of the C port of Intel's ISIS-II plm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#pragma once
/* Error codes */
#define ERR1   1   /* INVALID PL/M-80 CHARACTER */
#define ERR2   2   /* UNPRINTABLE ASCII CHARACTER */
#define ERR3   3   /* IDENTIFIER, STRING, OR NUMBER TOO LONG, TRUNCATED */
#define ERR4   4   /* ILLEGAL NUMERIC CONSTANT TYPE */
#define ERR5   5   /* INVALID CHARACTER IN NUMERIC CONSTANT */
#define ERR6   6   /* ILLEGAL MACRO REFERENCE, RECURSIVE EXPANSION */
#define ERR7   7   /* LIMIT EXCEEDED: MACROS NESTED TOO DEEPLY */
#define ERR8   8   /* INVALID CONTROL FORMAT */
#define ERR9   9   /* INVALID CONTROL */
#define ERR10  10  /* ILLEGAL USE OF PRIMARY CONTROL AFTER NON-CONTROL LINE */
#define ERR11  11  /* MISSING CONTROL PARAMETER */
#define ERR12  12  /* INVALID CONTROL PARAMETER */
#define ERR13  13  /* LIMIT EXCEEDED: INCLUDE NESTING */
#define ERR14  14  /* INVALID CONTROL FORMAT, INCLUDE NOT LAST CONTROL */
#define ERR15  15  /* MISSING INCLUDE CONTROL PARAMETER */
#define ERR16  16  /* ILLEGAL PRINT CONTROL */
#define ERR17  17  /* INVALID PATH-NAME */
#define ERR18  18  /* INVALID MULTIPLE LABELS AS MODULE NAMES */
#define ERR19  19  /* INVALID LABEL IN MODULE WITHOUT MAIN PROGRAM */
#define ERR20  20  /* MISMATCHED IDENTIFIER AT END OF BLOCK */
#define ERR21  21  /* MISSING PROCEDURE NAME */
#define ERR22  22  /* INVALID MULTIPLE LABELS AS PROCEDURE NAMES */
#define ERR23  23  /* INVALID LABELLED END IN EXTERNAL PROCEDURE */
#define ERR24  24  /* INVALID STATEMENT IN EXTERNAL PROCEDURE */
#define ERR25  25  /* UNDECLARED PARAMETER */
#define ERR26  26  /* INVALID DECLARATION, STATEMENT OUT OF PLACE */
#define ERR27  27  /* LIMIT EXCEEDED: NUMBER OF DO BLOCKS */
#define ERR28  28  /* MISSING 'THEN' */
#define ERR29  29  /* ILLEGAL STATEMENT */
#define ERR30  30  /* LIMIT EXCEEDED: NUMBER OF LABELS ON STATEMENT */
#define ERR31  31  /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
#define ERR32  32  /* INVALID SYNTAX, TEXT IGNORED UNTIL ';' */
#define ERR33  33  /* DUPLICATE LABEL DECLARATION */
#define ERR34  34  /* DUPLICATE PROCEDURE DECLARATION */
#define ERR35  35  /* LIMIT EXCEEDED: NUMBER OF PROCEDURES */
#define ERR36  36  /* MISSING PARAMETER */
#define ERR37  37  /* MISSING ') ' AT END OF PARAMETER LIST */
#define ERR38  38  /* DUPLICATE PARAMETER NAME */
#define ERR39  39  /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
#define ERR40  40  /* DUPLICATE ATTRIBUTE */
#define ERR41  41  /* CONFLICTING ATTRIBUTE */
#define ERR42  42  /* INVALID INTERRUPT VALUE */
#define ERR43  43  /* MISSING INTERRUPT VALUE */
#define ERR44  44  /* ILLEGAL ATTRIBUTE, 'INTERRUPT' WITH PARAMETERS */
#define ERR45  45  /* ILLEGAL ATTRIBUTE, 'INTERRUPT' WITH TYPED PROCEDURE */
#define ERR46  46  /* ILLEGAL USE OF LABEL */
#define ERR47  47  /* MISSING ') ' AT END OF FACTORED DECLARATION */
#define ERR48  48  /* ILLEGAL DECLARATION STATEMENT SYNTAX */
#define ERR49  49  /* LIMIT EXCEEDED: NUMBER OF ITEMS IN FACTORED DECLARE */
#define ERR50  50  /* INVALID ATTRIBUTES FOR BASE */
#define ERR51  51  /* INVALID BASE, SUBSCRIPTING ILLEGAL */
#define ERR52  52  /* INVALID BASE, MEMBER OF BASED STRUCTURE OR ARRAY OF STRUCTURES */
#define ERR53  53  /* INVALID STRUCTURE MEMBER IN BASE */
#define ERR54  54  /* UNDECLARED BASE */
#define ERR55  55  /* UNDECLARED STRUCTURE MEMBER IN BASE */
#define ERR56  56  /* INVALID MACRO TEXT, NOT A STRING CONSTANT */
#define ERR57  57  /* INVALID DIMENSION, ZERO ILLEGAL */
#define ERR58  58  /* INVALID STAR DIMENSION IN FACTORED DECLARATION */
#define ERR59  59  /* ILLEGAL DIMENSION ATTRIBUTE */
#define ERR60  60  /* MISSING ') ' AT END OF DIMENSION */
#define ERR61  61  /* MISSING TYPE */
#define ERR62  62  /* INVALID STAR DIMENSION WITH 'STRUCTURE' OR 'EXTERNAL' */
#define ERR63  63  /* INVALID DIMENSION WITH THIS ATTRIBUTE */
#define ERR64  64  /* MISSING STRUCTURE MEMBERS */
#define ERR65  65  /* MISSING ') ' AT END OF STRUCTURE MEMBER LIST */
#define ERR66  66  /* INVALID STRUCTURE MEMBER, NOT AN IDENTIFIER */
#define ERR67  67  /* DUPLICATE STRUCTURE MEMBER NAME */
#define ERR68  68  /* LIMIT EXCEEDED: NUMBER OF STRUCTURE MEMBERS */
#define ERR69  69  /* INVALID STAR DIMENSION WITH STRUCTURE MEMBER */
#define ERR70  70  /* INVALID MEMBER TYPE, 'STRUCTURE' ILLEGAL */
#define ERR71  71  /* INVALID MEMBER TYPE, 'LABEL' ILLEGAL */
#define ERR72  72  /* MISSING TYPE FOR STRUCTURE MEMBER */
#define ERR73  73  /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
#define ERR74  74  /* INVALID STAR DIMENSION, NOT WITH 'DATA' OR 'INITIAL' */
#define ERR75  75  /* MISSING ARGUMENT OF 'AT' , 'DATA' , OR 'INITIAL' */
#define ERR76  76  /* CONFLICTING ATTRIBUTE WITH PARAMETER */
#define ERR77  77  /* INVALID PARAMETER DECLARATION, BASE ILLEGAL */
#define ERR78  78  /* DUPLICATE DECLARATION */
#define ERR79  79  /* ILLEGAL PARAMETER TYPE, NOT BYTE OR ADDRESS */
#define ERR80  80  /* INVALID DECLARATION, LABEL MAY NOT BE BASED */
#define ERR81  81  /* CONFLICTING ATTRIBUTE WITH 'BASE' */
#define ERR82  82  /* INVALID SYNTAX, MISMATCHED '(' */
#define ERR83  83  /* LIMIT EXCEEDED: DYNAMIC STORAGE */
#define ERR84  84  /* LIMIT EXCEEDED: BLOCK NESTING */
#define ERR85  85  /* LONG STRING ASSUMED CLOSED AT NEXT SEMICOLON OR QUOTE */
#define ERR86  86  /* LIMIT EXCEEDED: SOURCE LINE LENGTH */
#define ERR87  87  /* MISSING 'END' , END-OF-FILE ENCOUNTERED */
#define ERR88  88  /* INVALID PROCEDURE NESTING, ILLEGAL IN REENTRANT PROCEDURE */
#define ERR89  89  /* MISSING 'DO' FOR MODULE */
#define ERR90  90  /* MISSING NAME FOR MODULE */
#define ERR91  91  /* ILLEGAL PAGELENGTH CONTROL VALUE */
#define ERR92  92  /* ILLEGAL PAGEWIDTH CONTROL VALUE */
#define ERR93  93  /* MISSING 'DO' FOR 'END' , 'END' IGNORED */
#define ERR94  94  /* ILLEGAL CONSTANT, VALUE > 65535 */
#define ERR95  95  /* ILLEGAL RESPECIFICATION OF PRIMARY CONTROL IGNORED */
#define ERR96  96  /* COMPILER ERROR: SCOPE STACK UNDERFLOW */
#define ERR97  97  /* COMPILER ERROR: PARSE STACK UNDERFLOW */
#define ERR98  98  /* INCLUDE FILE IS NOT A DISKETTE FILE */
#define ERR99  99  /* ?? unused */
#define ERR100 100 /* INVALID STRING CONSTANT IN EXPRESSION */
#define ERR101 101 /* INVALID ITEM FOLLOWS DOT OPERATOR */
#define ERR102 102 /* MISSING PRIMARY OPERAND */
#define ERR103 103 /* MISSING ') ' AT END OF SUBEXPRESSION */
#define ERR104 104 /* ILLEGAL PROCEDURE INVOCATION WITH DOT OPERATOR */
#define ERR105 105 /* UNDECLARED IDENTIFIER */
#define ERR106 106 /* INVALID INPUT/OUTPUT PORT NUMBER */
#define ERR107 107 /* ILLEGAL INPUT/OUTPUT PORT NUMBER, NOT NUMERIC CONSTANT */
#define ERR108 108 /* MISSING ') ' AFTER INPUT/OUTPUT PORT NUMBER */
#define ERR109 109 /* MISSING INPUT/OUTPUT PORT NUMBER */
#define ERR110 110 /* INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE */
#define ERR111 111 /* INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER */
#define ERR112 112 /* UNDECLARED STRUCTURE MEMBER */
#define ERR113 113 /* MISSING ') ' AT END OF ARGUMENT LIST */
#define ERR114 114 /* INVALID SUBSCRIPT, MULTIPLE SUBSCRIPTS ILLEGAL */
#define ERR115 115 /* MISSING ') ' AT END OF SUBSCRIPT */
#define ERR116 116 /* MISSING '=' IN ASSIGNMENT STATEMENT */
#define ERR117 117 /* MISSING PROCEDURE NAME IN CALL STATEMENT */
#define ERR118 118 /* INVALID INDIRECT CALL, IDENTIFIER NOT AN ADDRESS SCALAR */
#define ERR119 119 /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
#define ERR120 120 /* LIMIT EXCEEDED: EXPRESSION TOO COMPLEX */
#define ERR121 121 /* LIMIT EXCEEDED: EXPRESSION TOO COMPLEX */
#define ERR122 122 /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
#define ERR123 123 /* INVALID DOT OPERAND, BUILT-IN PROCEDURE ILLEGAL */
#define ERR124 124 /* MISSING ARGUMENTS FOR BUILT-IN PROCEDURE */
#define ERR125 125 /* ILLEGAL ARGUMENT FOR BUILT-IN PROCEDURE */
#define ERR126 126 /* MISSING ') ' AFTER BUILT-IN PROCEDURE ARGUMENT LIST */
#define ERR127 127 /* INVALID SUBSCRIPT ON NON-ARRAY */
#define ERR128 128 /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
#define ERR129 129 /* ILLEGAL 'CALL' WITH TYPED PROCEDURE */
#define ERR130 130 /* ILLEGAL REFERENCE TO OUTPUT FUNCTION */
#define ERR131 131 /* ILLEGAL REFERENCE TO UNTYPED PROCEDURE */
#define ERR132 132 /* ILLEGAL USE OF LABEL */
#define ERR133 133 /* ILLEGAL REFERENCE TO UNSUBSCRIPTED ARRAY */
#define ERR134 134 /* ILLEGAL REFERENCE TO UNSUBSCRIPTED MEMBER ARRAY */
#define ERR135 135 /* ILLEGAL REFERENCE TO AN UNQUALIFIED STRUCTURE */
#define ERR136 136 /* INVALID RETURN FOR UNTYPED PROCEDURE, VALUE ILLEGAL */
#define ERR137 137 /* MISSING VALUE IN RETURN FOR TYPED PROCEDURE */
#define ERR138 138 /* MISSING INDEX VARIABLE */
#define ERR139 139 /* INVALID INDEX VARIABLE TYPE, NOT BYTE OR ADDRESS */
#define ERR140 140 /* MISSING '=' FOLLOWING INDEX VARIABLE */
#define ERR141 141 /* MISSING 'TO' CLAUSE */
#define ERR142 142 /* MISSING IDENTIFIER FOLLOWING GOTO */
#define ERR143 143 /* INVALID REFERENCE FOLLOWING GOTO, NOT A LABEL */
#define ERR144 144 /* INVALID GOTO LABEL, NOT AT LOCAL OR MODULE LEVEL */
#define ERR145 145 /* MISSING 'TO' FOLLOWING 'GO' */
#define ERR146 146 /* MISSING ') ' AFTER 'AT' RESTRICTED EXPRESSION */
#define ERR147 147 /* MISSING IDENTIFIER FOLLOWING DOT OPERATOR */
#define ERR148 148 /* INVALID QUALIFICATION IN RESTRICTED REFERENCE */
#define ERR149 149 /* INVALID SUBSCRIPTING IN RESTRICTED REFERENCE */
#define ERR150 150 /* MISSING ') ' AT END OF RESTRICTED SUBSCRIPT */
#define ERR151 151 /* INVALID OPERAND IN RESTRICTED EXPRESSION */
#define ERR152 152 /* MISSING ') ' AFTER CONSTANT LIST */
#define ERR153 153 /* INVALID NUMBER OF ARGUMENTS IN CALL, TOO MANY */
#define ERR154 154 /* INVALID NUMBER OF ARGUMENTS IN CALL, TOO FEW */
#define ERR155 155 /* INVALID RETURN IN MAIN PROGRAM */
#define ERR156 156 /* MISSING RETURN STATEMENT IN TYPED PROCEDURE */
#define ERR157 157 /* INVALID ARGUMENT, ARRAY REQUIRED FOR LENGTH OR LAST */
#define ERR158 158 /* INVALID DOT OPERAND, LABEL ILLEGAL */
#define ERR159 159 /* COMPILER ERROR: PARSE STACK UNDERFLOW */
#define ERR160 160 /* COMPILER ERROR: OPERAND STACK UNDERFLOW */
#define ERR161 161 /* COMPILER ERROR: ILLEGAL OPERAND STACK EXCHANGE */
#define ERR162 162 /* COMPILER ERROR: OPERATOR STACK UNDERFLOW */
#define ERR163 163 /* COMPILER ERROR: GENERATION FAILURE */
#define ERR164 164 /* COMPILER ERROR: SCOPE STACK OVERFLOW */
#define ERR165 165 /* COMPILER ERROR: SCOPE STACK UNDERFLOW */
#define ERR166 166 /* COMPILER ERROR: CONTROL STACK OVERFLOW */
#define ERR167 167 /* COMPILER ERROR: CONTROL STACK UNDERFLOW */
#define ERR168 168 /* COMPILER ERROR: BRANCH MISSING IN 'IF' STATEMENT */
#define ERR169 169 /* ILLEGAL FORWARD CALL */
#define ERR170 170 /* ILLEGAL RECURSIVE CALL */
#define ERR171 171 /* INVALID USE OF DELIMITER OR RESERVED WORD IN EXPRESSION */
#define ERR172 172 /* INVALID LABEL: UNDEFINED */
#define ERR173 173 /* INVALID LEFT SIDE OF ASSIGNMENT: VARIABLE DECLARED WITH DATA ATTRIBUTE */
#define ERR174 174 /* INVALID NULL PROCEDURE */
#define ERR175 175 /* unused */
#define ERR176 176 /* INVALID INTVECTOR INTERVAL VALUE */
#define ERR177 177 /* INVALID INTVECTOR LOCATION VALUE */
#define ERR178                                                                                     \
    178 /* INVALID 'AT' RESTRICTED REFERENCE, EXTERNAL ATTRIBUTE CONFLICTS WITH PUBLIC ATTRIBUTE   \
         */
#define ERR179    179 /* OUTER 'IF' MAY NOT HAVE AN 'ELSE' PART */
#define ERR180    180 /* MISSING OR INVALID CONDITIONAL COMPILATION PARAMETER */
#define ERR181    181 /* MISSING OR INVALID CONDITIONAL COMPILATION CONSTANT */
#define ERR182    182 /* MISPLACED ELSE OR ELSEIF OPTION */
#define ERR183    183 /* MISPLACED ENDIF OPTION */
#define ERR184    184 /* CONDITIONAL COMPILATION PARAMETER NAME TOO LONG */
#define ERR185    185 /* MISSING OPERATOR IN CONDITIONAL COMPILATION EXPRESSION */
#define ERR186    186 /* INVALID CONDITIONAL COMPILATION CONSTANT, TOO LARGE */
#define ERR187    187 /* LIMIT EXCEEDED: NUMBER OF SAVE LEVELS > 5 */
#define ERR188    188 /* MISPLACED RESTORE OPTION */
#define ERR189    189 /* NULL STRING NOT ALLOWED */
#define ERR200    200 /* LIMIT EXCEEDED: STATEMENT SIZE */
#define ERR201    201 /* INVALID DO CASE BLOCK, AT LEAST ONE CASE REQUIRED */
#define ERR202    202 /* LIMIT EXCEEDED: NUMBER OF ACTIVE CASES */
#define ERR203    203 /* LIMIT EXCEEDED: NESTING OF TYPED PROCEDURE CALLS */
#define ERR204    204 /* LIMIT EXCEEDED: NUMBER OF ACTIVE PROCEDURES AND DO CASE GROUPS */
#define ERR205    205 /* ILLEGAL NESTING OF BLOCKS, ENDS NOT BALANCED */
#define ERR206    206 /* LIMIT EXCEEDED: CODE SEGMENT SIZE */
#define ERR207    207 /* LIMIT EXCEEDED: SEGMENT SIZE */
#define ERR208    208 /* LIMIT EXCEEDED: STRUCTURE SIZE */
#define ERR209    209 /* ILLEGAL INITIALIZATION OF MORE SPACE THAN DECLARED */
#define ERR210    210 /* ILLEGAL INITIALIZATION OF A BYTE TO A VALUE > 255 */
#define ERR211    211 /* INVALID IDENTIFIER IN 'AT' RESTRICTED REFERENCE */
#define ERR212    212 /* INVALID RESTRICTED REFERENCE IN 'AT' , BASE ILLEGAL */
#define ERR213    213 /* UNDEFINED RESTRICTED REFERENCE IN 'AT' */
#define ERR214    214 /* COMPILER ERROR: OPERAND CANNOT BE TRANSFORMED */
#define ERR215    215 /* COMPILER ERROR: EOF READ IN FINAL ASSEMBLY */
#define ERR216    216 /* COMPILER ERROR: BAD LABEL ADDRESS */
#define ERR217    217 /* ILLEGAL INITIALIZATION OF AN EXTERNAL VARIABLE */
#define ERR218    218 /* ILLEGAL SUCCESSIVE USES OF RELATIONAL OPERATORS */
#define ERR219    219 /* LIMIT EXCEEDED: NUMBER OF EXTERNALS > 255 */
#define IOERR_254 254 /* ATTEMPT TO READ PAS EOF */
