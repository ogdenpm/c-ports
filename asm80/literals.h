/****************************************************************************
 *  literals.h: part of the C port of Intel's ISIS-II asm80                 *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

// vim:ts=4:expandtab:shiftwidth=4:
/* character classes */
enum {
    CC_BAD = 0,
    CC_WS,
    CC_SEMI,
    CC_COLON,
    CC_EOL,
    CC_PUN,
    CC_DOLLAR,
    CC_QUOTE,
    CC_DIG,
    CC_LET,
    CC_MAC,
    CC_ESC,
    CC_BAR
};

#define TT_ID      0
#define TT_NUM     2
#define TT_STR     4

#define EOLCH      '\n'
#define ESC        0x1B
#define FF         '\f'
#define LF         '\n'
#define TAB        '\t'
#define MACROEOB   0xFE
#define MACROPARAM 0x80

#define MAXSYMSIZE 31
#define VERYWIDE   4096 // crazy page width

enum {
    O_ID = 1,
    O_TARGET,
    O_LABEL,
    O_SET,
    O_EQU,
    O_REF,
    TT_EQU,
    TT_SET,
    O_NAME,
    O_STRING,
    O_DATA,
    O_NUMBER,
    O_PARAM
};
enum { K_SPECIAL = 5, K_REGNAME = 7, K_SP, O_SETEQUNAME = 100 };

/* yyTypes */
enum {
    VALUE = 0,
    EOL,
    LPAREN,
    RPAREN,
    STAR,
    PLUS,
    COMMA,
    MINUS,
    UPLUS,
    SLASH,
    UMINUS,
    EQ,
    LT,
    LE,
    GT,
    GE,
    NE,
    NOT,
    AND,
    OR,
    XOR,
    MOD,
    SHL,
    SHR,
    HIGH,
    LOW,
    DB,
    DW,
    DS,
    EQU,
    SET,
    ORG,
    END,
    IF,
    ELSE,
    ENDIF,
    LXI,
    REG16,
    LDSTAX,
    ARITH,
    IMM8,
    MVI,
    INRDCR,
    MOV,
    IMM16,
    SINGLE,
    RST,
    ASEG,
    CSEG,
    DSEG,
    PUBLIC,
    EXTRN,
    NAME,
    STKLN,
    MACRO,
    MACROARG,
    ENDM,
    EXITM,
    MACRONAME,
    IRP,
    IRPC,
    ITERPARAM,
    REPT,
    LOCAL,
    NULVAL,
    NUL
};

/* relocatable record types */
#define OMF_MODHDR   2
#define OMF_MODEND   4
#define OMF_CONTENT  6
#define OMF_EOF      0xe
#define OMF_LOCALS   0x12
#define OMF_PUBLICS  0x16
#define OMF_EXTNAMES 0x18
#define OMF_EXTREF   0x20
#define OMF_RELOC    0x22
#define OMF_INTERSEG 0x24

/* seek modes */
#define SEEKTELL     0
#define SEEKBACK     1
#define SEEKABS      2
#define SEEKFWD      3
#define SEEKEND      4

/* segments */
#define SEG_ABS      0
#define SEG_CODE     1
#define SEG_DATA     2
#define SEG_STACK    3
#define SEG_MEMORY   4

/* stack table Ids */
#define TID_KEYWORD  0
#define TID_SYMBOL   1
#define TID_MACRO    2

#define UF_SEGMASK   7
#define UF_RLOW      8
#define UF_RHIGH     0x10
#define UF_RBOTH     0x18
#define UF_PUBLIC    0x20
#define UF_EXTRN     0x40

/* file open modes */
#define READ_MODE    1
#define WRITE_MODE   2
#define UPDATE_MODE  3

#define XREF_DEF     0
#define XREF_REF     1
#define XREF_FIN     2
#define XREF_EOF     3

#define M_MACRO      0
#define M_IRP        1
#define M_IRPC       2
#define M_REPT       3
#define M_INVOKE     4
#define M_BODY       5

/* runtime errors */
#define RTE_STACK    0
#define RTE_TABLE    1
#define RTE_CMDLINE  2
#define RTE_EOF      3 // no longer used
#define RTE_FILE     4

/* label usage */
#define L_SETEQU     1
#define L_TARGET     2
#define L_REF        0