/****************************************************************************
 *  plm.h: part of the C port of Intel's ISIS-II plm80                      *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "errors.h"
#include "int.h"
#include "vfile.h"
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#ifdef __GNUC__
#define trunc    _trunc
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

#define VERSION "V4.0"

typedef uint8_t *pointer;
typedef uint16_t *wpointer;
typedef uint16_t offset_t;
typedef uint16_t index_t;

#define High(n)     ((n) >> 8)
#define Low(n)      ((n) & 0xff)

#define QUOTE       '\''
#define ISISEOF     0x81

#define MAXSTRING   4096
#define MAXSYM      2500 // same as pl/m-386
#define MAXINFO     3000 // allow for symbol reuse
#define MAXCASE     1000
#define MAXMEMBER   33
#define MAXFACTORED 33
#define MAXXREF     4000
#define MAXINCLUDES 40

enum { Left, Right }; // left right indexes
/* flags */
enum {
    F_PUBLIC    = (1 << 0),
    F_EXTERNAL  = (1 << 1),
    F_BASED     = (1 << 2),
    F_INITIAL   = (1 << 3),
    F_REENTRANT = (1 << 4),
    F_DATA      = (1 << 5),
    F_INTERRUPT = (1 << 6),
    F_AT        = (1 << 7),
    F_ARRAY     = (1 << 8),
    F_STARDIM   = (1 << 9),
    F_PARAMETER = (1 << 10),
    F_MEMBER    = (1 << 11),
    F_LABEL     = (1 << 12),
    F_AUTOMATIC = (1 << 13),
    F_PACKED    = (1 << 14),
    F_ABSOLUTE  = (1 << 15),
    F_MEMORY    = (1 << 16),
    F_DECLARED  = (1 << 17),
    F_DEFINED   = (1 << 18),
    F_MODGOTO   = (1 << 19)
};

/* token info types */
// 10, 12, 18, 18, 18, 22, 11, 10, 8, 9
enum {
    LIT_T     = 0,
    LABEL_T   = 1,
    BYTE_T    = 2,
    ADDRESS_T = 3,
    STRUCT_T  = 4,
    PROC_T    = 5,
    BUILTIN_T = 6,
    MACRO_T   = 7,
    UNK_T     = 8,
    CONDVAR_T = 9
};

enum {
    BYTE_A       = 0,
    ADDRESS_A    = 1,
    BYTEIND_A    = 2,
    ADDRESSIND_A = 3,
    STRUCT_A     = 8,
    LABEL_A      = 9,
    LIT_A        = 12
};

/* character classes */
enum {
    CC_DIGIT     = 2,
    CC_HEXCHAR   = 3,
    CC_ALPHA     = 4,
    CC_PLUS      = 5,
    CC_MINUS     = 6,
    CC_STAR      = 7,
    CC_SLASH     = 8,
    CC_LPAREN    = 9,
    CC_RPAREN    = 10,
    CC_COMMA     = 11,
    CC_COLON     = 12,
    CC_SEMICOLON = 13,
    CC_QUOTE     = 14,
    CC_PERIOD    = 15,
    CC_EQUALS    = 16,
    CC_LESS      = 17,
    CC_GREATER   = 18,
    CC_WSPACE    = 19,
    CC_DOLLAR    = 20,
    CC_INVALID   = 21,
    CC_NONPRINT  = 22,
    CC_NEWLINE   = 23,
    CC_UNDERBAR  = 24
};

/* start statement codes */
enum {
    S_STATEMENT = 0,
    S_SEMICOLON,
    S_CALL,
    S_DECLARE,
    S_DISABLE,
    S_DO,
    S_ENABLE,
    S_END,
    S_GO,
    S_GOTO,
    S_HALT,
    S_IF,
    S_PROCEDURE,
    S_RETURN
};

enum { DO_PROC = 0, SIMPLE_DO, DO_WHILE, DO_CASE, DO_ITERATIVE };


// operand info type
enum { OP_IM8 = 8, OP_IM16, OP_STK, OP_VAR, OP_AUTO, OP_DIRECT = 16};
/* standard structures */

typedef struct {
    uint8_t len;
    char str[1];
} pstr_t;

#define _pstr_t(name, size)                                                                        \
    struct {                                                                                       \
        uint8_t len;                                                                               \
        char str[size];                                                                            \
    } name

typedef struct {
    FILE *fp;
    char const *sNam;
    char const *fNam;
} file_t;

typedef struct _sym {
    struct _sym *link;
    struct _info *infoChain;
    pstr_t const *name;
} sym_t;

/*
    Unlike the original PL/M info items are allocated the same storage space
    this allows an dynamic array to be used. As this version also uses native OS
    memory allocation, the array also avoids the hidden information that would be
    used per malloc, which for 64 bit architecture would have bumped the memory used for
    small items considerably.
*/
typedef struct _info {
    uint8_t type;
    sym_t *sym;
    uint16_t scope;
    struct _xref *xref; // previously reused scope, now unique to allow immediate build
    union {
        struct _info *ilink;
        uint16_t linkVal;
        uint16_t addr;
    };
    union {
        pstr_t const *lit; // converted to string;
        struct {
            union {
                uint32_t flag;
                uint8_t condFlag;
                uint8_t builtinId;
            };
            uint16_t extId;
            union {
                uint16_t dim;
                uint16_t codeSize;
            };
            union {
                struct _info *baseInfo;
                uint16_t baseVal;
                uint16_t stackUsage;
            };
            union {
                struct _info *parent;
                uint16_t totalSize;
            };
            uint8_t returnType;
            uint8_t intno;
            uint8_t paramCnt;
            uint8_t procId;
        };
    };
} info_t;

typedef struct {
    info_t *info;
    uint16_t codeSize;
    uint16_t wB4B0;
    uint16_t stackSize;
    uint8_t extProcId;
    uint8_t next;
    index_t activeGrpCnt; // could have used union
} blk_t;

typedef struct _xref {
    struct _xref *next;
    uint16_t line;
} xref_t;

struct _linfo { // allows type to be read and keep alignment
    uint16_t lineCnt;
    uint16_t stmtCnt;
    uint16_t blkCnt;
};
typedef struct {
    uint8_t type;
    uint16_t lineCnt;
    uint16_t stmtCnt;
    uint16_t blkCnt;
} linfo_t;

typedef struct {
    uint8_t type;
    union {
        uint16_t dataw[3];
        struct {
            uint16_t len;
            uint8_t str[MAXSTRING];
        };
    };
} tx1item_t;

typedef struct {
    uint8_t icode;
    uint8_t right;
    union {
        uint16_t nodeIdx;
        uint16_t infoIdx;
        uint16_t t2Cnt;
        uint16_t val;
    };
} operand_t;

typedef struct {
    uint8_t type;
    index_t left;
    index_t right;
    index_t extra;
    uint8_t exprAttr;
    uint8_t exprLoc;
    uint16_t cnt;
} tx2_t;

// record offsets
// common
#define REC_TYPE     0
#define REC_LEN      1
#define REC_DATA     3

// MODEND (4)
#define MODEND_TYPE  3
#define MODEND_SEG   4
#define MODEND_OFF   5

// CONTENT (6)
#define CONTENT_SEG  3
#define CONTENT_OFF  4
#define CONTENT_DATA 6

#define FIXUP_SEG    3
#define S_ABS        0
#define S_CODE       1
#define S_DATA       2
#define S_STACK      3
#define S_MEMORY     4
#define FIX_HILO     3

typedef struct {
    index_t infoIdx;
    uint16_t subscript, memberSubscript, val;
} var_t;

typedef struct {
    uint16_t num;
    offset_t info;
    uint16_t stmt;
} err_t;
extern char *cmdTextP;

// array sizes
#define EXPRSTACKSIZE 100

extern offset_t MEMORY;

/* plm main */

/* File(main.plm) */
/* main.plm,plm0a.plm,plm1a.plm.plm6b.plm */

// the longjmp buffer
extern jmp_buf exception;

/* plmc.plm */
extern uint8_t verNo[];

/* plm0A.plm */
extern uint8_t cClass[];
extern uint16_t curBlkCnt;
extern uint16_t curScope;
#define DOBLKCNT 0 // indexes
#define PROCID   1

extern info_t *macroInfo;
extern uint16_t curStmtCnt;
extern uint16_t doBlkCnt;
extern uint16_t ifDepth;
extern uint8_t *inChrP; // has to be pointer as it accesses data outside info/symbol space
extern bool isNonCtrlLine;
extern sym_t *stmtStartSymbol;
extern uint8_t stmtStartToken;
extern uint8_t nextCh;
extern uint8_t startLexCode;
extern uint8_t lineBuf[];
#define MAXLINE 255
extern bool lineInfoToWrite;
extern uint16_t macroDepth;
typedef struct {
    uint8_t *text;
    info_t *macroInfo;
} macro_t;

extern macro_t macroPtrs[6];
extern sym_t *markedSym;
extern bool skippingCOND;
extern uint16_t stateSP;
extern uint16_t stateStack[];
extern uint16_t stmtLabelCnt;
extern sym_t *stmtLabels[];
extern uint8_t stmtStartCode;
extern uint8_t tToLMap[];
extern uint16_t wTokenLen; // used for string and lit
extern uint8_t tokenStr[];

extern uint8_t tokenType;
extern uint16_t tokenVal;
extern bool yyAgain;

/* plm0A.plm,main1.plm */
extern info_t *curProc;
extern linfo_t linfo;

/* main2 */
extern blk_t blk[];

/* plm0A.plm,plm3a.plm,pdata4.plm */

/* plm0b.plm, plm0c.asm*/

extern bool trunc;

/* plm0e.plm */

/* plm0f.plm */
extern uint16_t curState;
extern bool endSeen;

/* File(plm0h.plm) */

/* plm overlay 1 */
/* main1.plm */
extern bool tx2LinfoPending;
extern uint16_t curStmtNum;
extern uint16_t markedStSP;
extern bool regetTx1Item;
extern uint16_t stmtT2Cnt;
extern uint8_t tx1ICode;
extern uint8_t tx1Attr;
extern tx1item_t tx1Item;

extern var_t var;

/* main1.plm,plm3a.plm */

/* plm1a.plm */
extern uint8_t tx1ToTx2Map[];
extern uint8_t lexHandlerIdxTable[];
extern uint8_t iToTx2Map[];
extern uint8_t precedence[];
extern uint8_t builtinsMap[];
extern operand_t operandStack[];

extern uint16_t operandSP;
extern uint16_t operatorSP;
extern uint16_t operatorStack[];
extern uint16_t parseSP;
extern uint16_t parseStack[];
extern operand_t tree[];
extern uint16_t stSP;

/* plm overlay 2 */
/* main2.plm */
typedef struct {
    uint8_t contents;      // What TX2 node is in each register
    uint8_t dataType;      // Data type attribute
    uint16_t stackOffset;  // Stack/memory offset
    uint16_t storageClass; // Storage flags
    uint8_t offset;        // Offset adjustment
    bool isDirect;         // Direct value flag
} RegisterState;

extern RegisterState registerState[];
extern uint8_t savedRegisterCount;
extern uint8_t exprRegisterCount;
extern uint8_t exprAttr[];
extern uint8_t exprLoc[];
extern uint8_t curExprLoc[];
extern uint8_t operandComplexity[];
extern uint8_t operandRegisterCost[];
extern uint8_t operandCategory[];
extern uint8_t operandFragmentType[];
extern uint8_t iCodeArgsIndex[];
extern uint8_t stackRegisterAttrs[];
extern uint8_t stackNodeContents[];
// extern uint8_t bC1BD;
extern uint8_t tx2qNxt;
extern uint8_t nodeControlFlags;
extern uint8_t selectedOperatorIdx;
extern uint8_t iCodeArgsIdx;
extern uint8_t fragLen;
extern uint8_t bC209[];
extern uint8_t blkOverCnt;
extern uint8_t activeGrpCnt;
extern bool registerHasValue[];
extern bool registerInExpression[];
extern bool registerNeedsSave[];
extern bool registerWasSaved[];
extern bool returnGenerated;
extern bool nextReturnState;
extern bool conflictMode;
extern bool invertComparison;
extern uint8_t fragment[];
extern uint8_t cfrag1;
extern uint8_t curExtProcId;
extern uint8_t curNodeType;
extern bool eofSeen;
extern uint16_t codeSize;
extern uint8_t procCallDepth;
extern uint8_t blkId;

extern tx2_t tx2[255];
extern uint8_t tx2qEnd;
extern uint8_t tx2qp;
extern uint16_t nodeTypeToAttribute[];
extern uint16_t callStackDepth[];
extern uint16_t callStackBase[];
extern uint16_t currentStackDepth;
extern uint16_t stackUsage;
extern uint16_t localVariableSize;
extern uint16_t currentFragmentCode;
extern uint16_t iCodeArgs[5];

/* plm2a.plm */
extern uint8_t helperModBase[];
extern uint8_t fragControlBits[];
extern uint8_t helperIndexMap[];
extern uint8_t fragmentHelperIndex[];
extern uint8_t helperMap[][11];
extern uint8_t helperGroup[];
extern uint8_t noteTypeToHelperGroup[];
extern uint8_t fragmentArgTypes[];
extern uint8_t fragmentCodeLength[];
extern uint8_t fragmentOpcodeTable[];
extern uint8_t fragmentOpcodeIndex[];
extern uint8_t operatorResultType[];
extern uint8_t opcodeEncodingMap[];
extern uint8_t loadOpAttributes[];
extern uint8_t loadOpTargetReg[];
extern uint8_t exprComplexity[];
extern uint8_t exprRegisterCost[];
extern uint8_t typeConversionTable[][16];
extern uint8_t nodeExprCategory[];
extern uint8_t loadOpFragment[];
extern uint8_t peepholePatterns[];
extern uint8_t optimisationStepTable[];
extern uint8_t constFoldRules[];
extern uint8_t nodeControlMap[];
extern uint8_t optimisationStep2Map[];
extern uint8_t optimisationStep1Map[];
extern uint8_t step2ActionTable[];
extern uint8_t lookupResultAttr[];
extern uint8_t lookupResultLoc[];
extern uint8_t attrLocLookupTable[][11];
extern uint8_t unused[];
extern uint16_t operatorPrecedence[];
extern uint16_t operatorTypeRules[];
extern uint16_t commonConstants[];

/* plm3a.plm */
extern uint8_t modHelperIdCnt[];
extern uint8_t modHelperId[];
extern uint8_t helperCodeLen[];
extern uint8_t printOrObj;
extern uint8_t recLocals[];
extern uint8_t recPublicAbs[];
extern uint8_t recPublicCode[];
extern uint8_t recPublicData[];
extern uint8_t recPublicMemory[];
extern uint8_t recExternals[];
extern uint8_t recModHdr[];
extern uint8_t recInitContent[];
extern uint8_t recExec[];
extern uint16_t curOffset;

/* plm3a.plm,pdata4.plm */
extern uint8_t recExtFixup[];
extern uint8_t recSelfFixup[];
extern uint8_t recCodeFixup[];
extern uint8_t recDataFixup[];
extern uint8_t recMemoryFixup[];
extern uint8_t recLineNum[];
extern uint8_t recEnd[];
//

/* pdata4.plm */
extern uint8_t b9692;
extern uint8_t b969C;
extern uint8_t b969D;
extern char valPStr[];
// extern uint8_t b96B1[];
extern uint8_t fixupType;
extern uint16_t baseAddr;
extern bool morePass4;
extern bool linePrefixChecked;
extern bool linePrefixEmitted;
extern uint8_t cfCode;
extern char commentPStr[];
extern uint16_t curExtId;
extern uint16_t blkCnt;
extern uint8_t fixupSelector;
// extern uint8_t endHelperId; now local var
extern uint8_t helperId;
// extern uint8_t helperModId; now local var
typedef struct {
    uint8_t len;
    char str[81];
} line_t;
extern line_t line;
extern char locPStr[];
extern err_t errData;
extern uint16_t lstLineLen;
extern char lstLine[];
extern uint8_t opByteCnt;
extern uint8_t opBytes[];

extern uint16_t stmtNo;
extern pstr_t *sValAry[];
extern uint16_t lineNo;
extern uint16_t locLabelNum;
extern uint16_t wValAry[];

/* pdata4.plm,pdata6.plm */
extern bool codeOn;
extern uint16_t stmtCnt;
extern bool listing;
extern bool listOff;

/* plm4a.plm */
extern uint8_t modHelperIdCnt[];
extern uint8_t modHelperId[];
extern uint8_t fragControlBits[];
extern uint8_t helperIndexMap[];
extern uint8_t fragmentHelperIndex[];
extern uint8_t helperGroup[];
extern uint8_t noteTypeToHelperGroup[];
extern uint8_t b4602[];
extern uint8_t b473D[];
extern uint8_t b475E[];
extern uint8_t b4774[];
extern uint8_t b478A[];
extern uint8_t helperLen[];
extern uint8_t codeTable[];
extern char const *opcodes[];
extern char const *regStr[];
extern uint8_t regNo[];
extern uint8_t stackOrigin[];
extern char const *regPairStr[];
extern uint8_t regPairNo[];
extern uint16_t instuctionSeq[];
extern uint16_t helperStart[];
extern uint16_t codeSeq[];

/* main5.plm */
extern uint8_t groupingChar;
extern uint8_t maxSymLen;

/* pdata6.plm */
extern bool moreLines;
extern uint16_t lineCnt;
extern uint16_t blkCnt;
extern uint16_t stmtNo;

/* files in common dir */
/* friendly names for the controls */
#define PRINT    controls[0]
#define XREF     controls[1]
#define SYMBOLS  controls[2]
#define DEBUG    controls[3]
#define PAGING   controls[4]
#define OBJECT   controls[5]
#define OPTIMIZE controls[6]
#define IXREF    controls[7]
#define DEPEND   controls[8]

/* data.plm */
extern vfile_t atf;
extern uint8_t wrapMarkerCol;
extern uint8_t wrapTextCol;
extern uint8_t skipCnt;
extern uint16_t scopeSP;
extern uint8_t col;
extern uint8_t controls[];
extern uint16_t csegSize;
extern info_t *info;
extern sym_t *curSym;
extern char DATE[];
extern uint16_t DATELEN;
extern uint16_t dsegSize;
extern uint16_t fatalCode;
extern bool hasErrors;
extern sym_t *hashTab[];
extern bool haveModuleLevelUnit;
extern uint16_t helperAddr[];
extern uint16_t intVecLoc;
extern uint8_t intVecNum;
extern file_t ixiFile;
extern char *ixiFileName;
extern char *depFileName;

extern uint16_t LEFTMARGIN;
extern uint16_t linesRead;
extern uint8_t linLft;
extern uint16_t localLabelCnt;
extern uint16_t *localLabels;
extern file_t lstFile;
extern char *lstFileName;
extern bool isList;
extern uint8_t margin;
extern file_t objFile;
extern char *objFileName;
extern bool moreCmdLine;
extern uint8_t PAGELEN;
extern uint16_t pageNo;

extern uint16_t scopeChains[];
extern uint16_t procCnt;
extern info_t *procInfo[];
extern uint16_t programErrCnt;
extern uint8_t PWIDTH;
extern file_t srcFil;
extern uint16_t srcFileIdx;
extern file_t srcFileTable[]; /* 6 * (8 words fNam, blkNum, bytNum) */
extern uint8_t srcStemLen;
extern uint8_t srcStemName[];
extern bool standAlone;
extern offset_t startCmdLineP;
extern char TITLE[];
extern uint16_t TITLELEN;
extern uint8_t tWidth;
extern vfile_t utf1;
extern vfile_t utf2;
extern bool afterEOF;
extern char version[];
extern uint8_t *procIds;
extern offset_t w3822;
extern uint16_t cmdLineCaptured;

extern index_t symCnt;
extern sym_t symtab[];

extern info_t infotab[];
extern info_t *topInfo;

extern info_t **topDict;
extern info_t *dicttab[];

extern index_t topCase;
extern index_t casetab[];

extern xref_t xreftab[];

extern char const *includes[];
extern uint16_t includeCnt;

inline index_t ToIdx(info_t *p) {
    return p ? (index_t)(p - infotab) : 0;
}

inline info_t *FromIdx(index_t n) {
    return n ? infotab + n : NULL;
}

/* creati.c */
void CreateInfo(uint16_t scope, uint8_t type, sym_t *sym);

/* endcom.c */
void EndCompile(void);

/* fatal.c */
void Fatal(char const *str);

/* fi.c */
bool FindInfo(void);
void AdvNxtInfo(void);
void FindMemberInfo(void);
bool FindScopedInfo(uint16_t scope);

/* lookup.c */
uint8_t Hash(pstr_t *pstr);
void Lookup(pstr_t *pstr);

/* lstinf.c */
void LstModuleInfo(void);

/* lstsup.c */
void NewLineLst(void);
void TabLst(uint8_t tabTo);
void DotLst(uint8_t tabTo);
void EjectNext(void);
void SetMarkerInfo(uint8_t markerCol, uint8_t textCol);
void SetStartAndTabW(uint8_t startCol, uint8_t width);
void SetSkipLst(uint8_t cnt);
void lstStr(char const *str);
void lstStrCh(char const *str, int endch);
void lprintf(char const *fmt, ...);
void lstPstr(pstr_t *ps);
char const *hexfmt(uint8_t digits, uint16_t val);

/* main.c */
void Start(void);

/* main0.c */
void unwindInclude(void);
uint16_t Start0(void);

/* main1.c */
uint16_t Start1(void);

/* main2.c */
uint16_t Start2(void);

/* main3.c */
uint16_t putWord(pointer buf, uint16_t val);
uint16_t getWord(pointer buf);
uint16_t Start3(void);

/* main4.c */
uint16_t Start4(void);

/* main5.c */
void LoadDictionary(void);
int CmpSym(void const *a, void const *b);
void SortDictionary(void);
void PrintRefs(void);
void CreateIxrefFile(void);
void ProcessXref(void);
uint16_t Start5(void);

/* main6.c */
uint16_t Start6(void);

/* page.c */
void NewPgl(void);
void NlLead(void);

/* plm0a.c */
void Wr1LineInfo(void);
void Wr1Buf(void const *buf, uint16_t len);
void Wr1Byte(uint8_t v);
void Wr1Word(uint16_t v);
void Wr1Info(info_t *inf);
void Rd1Buf(void *buf, uint16_t len);
uint8_t Rd1Byte(void);
uint16_t Rd1Word(void);
void Wr1SyntaxError(uint16_t err);
void Wr1TokenErrorAt(uint16_t err);
void Wr1TokenError(uint16_t err, sym_t *sym);
_Noreturn void Lexfatal(uint16_t err);
void PushBlock(uint16_t idAndLevel);
void PopBlock(void);
void Wr1LexToken(void);
void Wr1XrefUse(void);
void Wr1XrefDef(void);

/* plm0b.c */
void ParseControlLine(char *pch);
void GNxtCh(void);

/* plm0e.c */
void Yylex(void);
void SetYyAgain(void);
bool YylexMatch(uint8_t token);
bool YylexNotMatch(uint8_t token);
void ParseExpresion(uint8_t endTok);
void ParseDeclareNames(void);
void ParseDeclareElementList(void);
void ParseProcStmt(void);

/* plm0f.c */
void ParseProgram(void);

/* plm0g.c */
pstr_t const *CreateLit(uint16_t wTokenLen, char const *str);

/* plm1a.c */
void fatal_ov1(uint8_t err);
void OptWrXrf(void);
void Wr2Buf(void *buf, uint16_t len);
void Wr2Byte(uint8_t v);
void Wr2Word(uint16_t v);
void Rd2Buf(void *buf, uint16_t len);
uint8_t Rd2Byte(void);
uint16_t Rd2Word(void);
uint16_t Wr2Item(uint8_t type, void *buf, uint16_t len);
uint16_t Wr2Simple(uint8_t type);
uint16_t Wr2Leaf(uint8_t type, uint16_t arg2w);
uint16_t Wr2Node(uint8_t type, uint16_t lhsRel, uint16_t rhsRel);
uint16_t Wr2ArgNode(uint8_t type, uint16_t relBC, uint16_t relDE, uint16_t extRel);
uint16_t CvtToRel(uint16_t arg1w);
void MapLToT2(void);
void Wr2Error(uint16_t errCode);
void Wr2TokError(uint16_t errCode);
void UngetTx1Item(void);
void RdTx1Item(void);

/* plm1b.c */
void GetTx1Item(void);
bool MatchTx1Item(uint8_t arg1b);
bool NotMatchTx1Item(uint8_t arg1b);
bool MatchTx2AuxFlag(uint8_t arg1b);
void RecoverRPOrEndExpr(void);
void ResyncRParen(void);
void ExpectRParen(uint8_t arg1b);
void ChkIdentifier(void);
void ChkStructureMember(void);
void GetVariable(void);
void WrAtBuf(void const *buf, uint16_t cnt);
void WrAtByte(uint8_t arg1b);
void WrAtWord(uint16_t arg1w);

/* plm1c.c */
void RestrictedExpression(void);
uint16_t InitialValueList(offset_t infoOffset);
void ResetStacks(void);
void PushParse(uint16_t arg1w);
uint16_t PopParse(void);
void PopOperand(void);
void PushSimpleOperand(uint8_t icode, uint16_t val);
void MoveOperandToTree(void);
void PushOperator(uint8_t arg1b);
uint16_t PopOperator(void);
void PushComplexOperand(uint8_t icode, uint8_t argCnt);
void ApplyOperator(void);
void FixupBased(info_t *varInfo);
void PushVariable(void);
void ChkTypedProcedure(void);
uint8_t GetCallArgCnt(void);
void ParseSizing(uint8_t arg1b);
void MkIndexNode(void);
void ParsePortNum(uint8_t arg1b);
void ChkIllegalCall(void);
uint8_t IsLValue(uint16_t sp);
void ConstantList(void);

/* plm1d.c */

void ExpressionStateMachine(void);

/* plm1e.c */
bool parseAssignment(void);
uint8_t ParseCall(void);
void Expression(void);
uint16_t SerialiseParse(uint16_t arg1w);
void Initialization(void);
void ParseLexItems(void);

/* plm1f.c */
uint16_t GetElementSize(info_t *pInfo);
int32_t RdAtByte(void);
int32_t RdAtWord(void);
void RdAtHdr(void);
void RdAtData(void);
void AllocateVars(void);

/* plm2a.c */
void WrFragData(void);
void PutTx1Byte(uint8_t arg1b);
void PutTx1Word(uint16_t arg1w);
void EncodeFragData(uint8_t arg1b);
void EmitTopItem(void);
void Tx2SyntaxError(uint8_t arg1b);
uint8_t GetFragmentOperandType(uint8_t arg1b);
void MoveTx2(uint8_t src, uint8_t dst);
uint8_t IndirectAddr(uint8_t arg1b);
void AdjustStackOnReturn(uint16_t arg1w);
bool EnterBlk(void);
bool ExitBlk(void);
void HandleFatalError(uint16_t err);
void AnalyzeRegisterUsage(void);
void SaveRegisterToStack(uint8_t arg1b);
void RestoreRegisterFromStack(uint8_t arg1b);
void PushRegisterToStack(uint8_t arg1b);
void SaveOrRedirectRegister(uint8_t arg1b);
void InvalidateRegistersByMask(uint8_t arg1b);
void GenerateReturnSequence(void);
void CreateConstantOrIdNode(uint16_t val, info_t *pInfo, uint8_t exprAttr, uint8_t exprLoc);
void GetOperandValue(uint8_t arg1b, wpointer arg2wP, wpointer arg3wP);
void DecrementExprRefs(void);
void UpdateExpressionLookup(uint8_t side);
void EncodeOperandInfo(uint8_t arg1b);
void EncodeExpressionOperand(uint8_t arg1b);
void AdjustRegisterOffset(uint8_t arg1b);
void PopRegisterFromStack(uint8_t arg1b);
void GetTx2Item(void);
void MergeRedundantNodes(void);
void FillTx2Queue(void);
void SetFirstStatementEnd(void);

/* plm2b.c */
void DeRelStmt(void);

/* plm2c.c */
void ProcessAssignmentNode(void);
void OptimiseStmtNodes(void);

/* plm2d.c */
void FoldConstantExpr(void);
void OptimiseExpression(void);

/* plm2e.c */
void AllocateRegister(void);
void GenerateOperandCode(void);
void GenerateFragmentCode(void);

/* plm2f.c */
void GenerateExpressionCode(void);
void c_procedure(void);

/* plm2g.c */
void FindParamInfo(uint8_t arg1b);
void MoveParametersToMemory(void);
void LoadEffectiveAddressToHL(uint16_t arg1w);
void AllocateLocalVariables(uint16_t arg1w);
void IncrementHL(void);
void EmitRegisterB(uint8_t arg1b);
void EmitRegisterD(uint8_t arg1b);
void CopyParameterFromHL(void);
void LoadDEFromMemory(void);
void PushParametersToStack(void);
void GenerateProcedureEntry(void);
void ProcessSpecialNodes(void);

/* plm2h.c */
void GenerateOperatorCode(void);
void GenerateCallCode(void);
void GenerateIndirectCallCode(void);
void BeginMoveOperation(void);
void EnterCaseBlock(void);
void ExitCaseBlock(void);
void ExitProcedure(void);
void GenerateLengthBuiltin(uint8_t arg1b);
void GenerateSizeBuiltin(void);
void BeginProcedureCall(void);
void GenerateStatementCode(void);

/* plm3a.c */
void FlushRecGrp(void);
void RecAddName(pointer recP, uint8_t offset, pstr_t const *pstr);
void ExtendChk(pointer arg1wP, uint16_t arg2w, uint8_t arg3b);
uint16_t CalcMaxStack(void);

void p3Error(uint16_t errNum, info_t *tokInfo, uint16_t stmt);
void Sub_49F9(void);

/* plm3b.c */
void WriteRec(pointer rec, uint8_t fixed);
void RecAddByte(pointer rec, uint8_t offset, uint8_t val);
void RecAddWord(pointer rec, uint8_t offset, uint16_t val);

/* plm4a.c */
void Sub_54BA(void);

/* plm4b.c */
void FlushRecs(void);
void AddWrdDisp(pstr_t *pstr, uint16_t arg2w);
void EmitLinePrefix(void);
void EmitStatementNo(void);
void EmitLabel(char const *label);
char const *FindErrStr(void);
void EmitError(void);
void fatal_ov46(uint8_t arg1b);
void ListInstruction(void);
void GetSourceLine(void);

/* plm4c.c */
pstr_t *pstrcpy(pstr_t *dst, pstr_t const *src);
pstr_t *pcstrcpy(pstr_t *dst, char const *src);
pstr_t *pstrcat(pstr_t *dst, pstr_t const *src);
pstr_t *pcstrcat(pstr_t *dst, char const *src);
void EmitCodeSeq(uint16_t codeIdx, uint8_t len);
void Sub_668B(void);

/* plm6a.c */
void MiscControl(vfile_t *txFile);
void T2Phase6(void);

/* plma.c */
void SignOnAndGetSourceName(void);

/* plmb.c */
void InitKeywordsAndBuiltins(void);
void SetPageNo(uint16_t v);

/* plmfile.c */
void CloseF(file_t *fileP);
void OpenF(file_t *fileP, char const *sNam, char const *fNam, char *access);

/* prints.c */
void PrintStr(char const *str, uint8_t len);

/* putlst.c */
void lstc(uint8_t ch);

/* symtab.c */
pstr_t const *pstrdup(pstr_t const *ps);
bool pstrequ(pstr_t const *ps, pstr_t const *pt);
sym_t *newSymbol(pstr_t const *ps);
void newInfo(uint8_t type);
void newDict(info_t *info);
index_t newCase(index_t val);
xref_t *newXref(xref_t *xrefNext, uint16_t line);
int newInclude(char const *fname);

/* vfile.c */
void vfReset(vfile_t *vf);
void vfWbuf(vfile_t *vf, void const *buf, uint32_t len);
void vfWbyte(vfile_t *vf, uint8_t val);
void vfWword(vfile_t *vf, uint16_t val);
void vfRewind(vfile_t *vf);
uint32_t vfRbuf(vfile_t *vf, void *buf, uint16_t len);
int32_t vfRbyte(vfile_t *vf);
int32_t vfRword(vfile_t *vf);
void dump(vfile_t *vf, char const *fname);

/* wrclst.c */
void WrLstC(char ch);

void DumpT1Stream();