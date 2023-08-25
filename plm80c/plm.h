/****************************************************************************
 *  plm.h: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
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

typedef unsigned char byte;
typedef unsigned short word;
typedef byte *pointer;
typedef word *wpointer;
typedef word offset_t;
typedef byte leword[2]; // to support writing of intel OMF words
typedef uint16_t index_t;

#define High(n)   ((n) >> 8)
#define Low(n)    ((n)&0xff)
#define Ror(v, n) (((v) >> n) | ((v) << (8 - n)))
#define Rol(v, n)	(((v) << n) | (((v) >> (8 - n)))
#define Move(s, d, c) memcpy(d, s, c)
#define Length(str)   (sizeof(str) - 1)

#define SetInfo(v)    info = &infotab[infoIdx = v]

#define TAB           9
#define CR            '\r'
#define LF            '\n'
#define QUOTE         '\''
#define ISISEOF       0x81

#define MAXSTRING   4096
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


enum { BYTE_A = 0, ADDRESS_A = 1, STRUCT_A = 8, LABEL_A = 9, LIT_A = 12 };

/* character classes */
enum {
    CC_BINDIGIT  = 0,
    CC_OCTDIGIT  = 1,
    CC_DECDIGIT  = 2,
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
    S_IDENTIFIER = 0,
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

enum { DO_PROC = 0, DO_LOOP = 1, DO_WHILE = 2, DO_CASE = 3 };

/* standard structures */

typedef struct {
    byte len;
    char str[1];
} pstr_t;

#define _pstr_t(name, size)                                                                        \
    struct {                                                                                       \
        byte len;                                                                                  \
        char str[size];                                                                            \
    } name

typedef struct {
    FILE *fp;
    char const *sNam;
    char const *fNam;
} file_t;

/*
    Unlike the original PL/M info items are allocated the same storage space
    this allows an dynamic array to be used. As this version also uses native OS
    memory allocation, the array also avoids the hidden information that would be
    used per malloc, which for 64 bit architecture would have bumped the memory used for
    small items considerably.
*/
typedef struct {
    byte type;
    index_t sym;
    word scope;
    union {
        index_t ilink;
        word linkVal;
        word addr;
    };
    union {
        pstr_t const *lit; // converted to string;
        struct {
            union {
                uint32_t flag;
                byte condFlag;
                byte builtinId;
            };
            byte extId;
            union {
                word dim;
                word codeSize;
            };
            union {
                index_t baseOff;
                word baseVal;
                word stackUsage;
            };
            union {
                word parent;
                word totalSize;
            };
            byte returnType;
            byte intno;
            byte paramCnt;
            byte procId;
        };
    };
} info_t;

typedef struct {
    index_t link;
    index_t infoIdx;
    pstr_t const *name;
} sym_t;

typedef struct {
    index_t next;
    word line;
} xref_t;

typedef struct {
    byte type;
    struct _linfo { // allows type to be read and keep alignment
        word lineCnt;
        word stmtCnt;
        word blkCnt;
    };
} linfo_t;

typedef struct {
    byte type;
    union {
        word dataw[3];
        struct {
            word len;
            byte str[MAXSTRING];
        };
    };
} tx1item_t;

typedef struct {
    byte op1;
    byte op2;
    word info;
} eStack_t;


typedef struct {
    byte opc;
    index_t op1;
    index_t op2;
    index_t op3;
    byte aux1;
    byte aux2;
    word auxw;
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
    offset_t infoOffset;
    word arrayIndex, nestedArrayIndex, val;
} var_t;

typedef struct {
    word num;
    offset_t info;
    word stmt;
} err_t;
char *cmdTextP;

// array sizes
#define EXPRSTACKSIZE 100

extern offset_t MEMORY;

/* plm main */

/* File(main.plm) */
/* main.plm,plm0a.plm,plm1a.plm.plm6b.plm */

// the longjmp buffer
extern jmp_buf exception;

/* plmc.plm */
extern byte verNo[];

/* plm0A.plm */
extern byte cClass[];
extern word curBlkCnt;
extern word curScope;
#define DOBLKCNT 0 // indexes
#define PROCID   1

extern offset_t macroIdx;
extern word curStmtCnt;
extern word doBlkCnt;
extern word ifDepth;
extern char inbuf[];
extern byte *inChrP; // has to be pointer as it accesses data outside info/symbol space
extern bool isNonCtrlLine;
extern offset_t stmtStartSymbol;
extern byte stmtStartToken;
extern byte nextCh;
extern byte startLexCode;
extern byte lineBuf[];
#define MAXLINE 255
extern bool lineInfoToWrite;
extern word macroDepth;
typedef struct {
    byte *text;
    index_t macroIdx;
} macro_t;

extern macro_t macroPtrs[6];
extern offset_t markedSymbolP;
extern bool skippingCOND;
extern byte state;
extern word stateIdx;
extern word stateStack[];
extern word stmtLabelCnt;
extern offset_t stmtLabels[];
extern byte stmtStartCode;
extern byte tok2oprMap[];
extern word wTokenLen;  // used for string and lit
extern byte tokenStr[];

extern byte tokenType;
extern word tokenVal;
extern bool yyAgain;

/* plm0A.plm,main1.plm */
extern word procIdx;
extern linfo_t linfo;

/* plm0A.plm,plm3a.plm,pdata4.plm */
extern byte tx1Buf[];

/* plm0b.plm, plm0c.asm*/

extern bool trunc;

/* plm0e.plm */

/* plm0f.plm */
extern word curState;
extern bool endSeen;

/* File(plm0h.plm) */

/* plm overlay 1 */
/* main1.plm */
extern bool tx2LinfoPending;
extern byte b91C0;
extern word curStmtNum;
extern word markedStSP;
extern bool regetTx1Item;
extern word t2CntForStmt;
extern byte tx1Aux1;
extern byte tx1Aux2;
extern tx1item_t tx1Item;

extern var_t var;

/* main1.plm,plm3a.plm */

/* plm1a.plm */
extern byte tx1ToTx2Map[];
extern byte lexHandlerIdxTable[];
extern byte icodeToTx2Map[];
extern byte b4172[];
extern byte builtinsMap[];
extern eStack_t eStack[];

extern word exSP;
extern word operatorSP;
extern word operatorStack[];
extern word parseSP;
extern word parseStack[];
extern eStack_t sStack[];
extern word stSP;

/* plm overlay 2 */
/* main2.plm */
extern byte bC045[];
extern byte bC04E[];
extern byte bC0A8[];
extern byte bC0B1;
extern byte bC0B2;
extern byte bC0B3[];
extern byte bC0B5[];
extern byte bC0B7[];
extern byte bC0B9[];
extern byte bC0BB[];
extern byte bC0BD[];
extern byte bC0BF[];
extern byte bC0C1[];
extern byte bC0C3[];
extern byte bC140[];
extern byte bC1BD;
extern byte tx2qNxt;
extern byte bC1D2;
extern byte bC1D9;
extern byte bC1DB;
extern byte fragLen;
extern byte bC209[];
extern offset_t blkCurInfo[];
extern byte blkOverCnt;
extern byte blkSP;
extern bool boC057[];
extern bool boC060[];
extern bool boC069[];
extern bool boC072[];
extern bool boC07B[];
extern bool boC1CC;
extern bool boC1CD;
extern bool boC1D8;
extern bool boC20F;
extern byte fragment[];
extern byte cfrag1;
extern byte curExtProcId;
extern byte curOp;
extern bool eofSeen;
extern byte extProcId[];
extern byte padC1D3;
extern word pc;
extern byte procCallDepth;
extern byte procChainId;
extern byte procChainNext[];



extern tx2_t tx2[255];
extern byte tx2qEnd;
extern byte tx2qp;
extern word wAF54[];
extern word wB488[];
extern word wB4B0[];
extern word wB4D8[];
extern word wB528[];
extern word wB53C[];
extern word wC084[];
extern word wC096[];
extern word wC1C3;
extern word wC1C5;
extern word wC1C7;
extern word wC1D6;
extern word wC1DC[5];

/* plm2a.plm */
extern byte b3FCD[];
extern byte b4029[];
extern byte b4128[];
extern byte b413B[];
extern byte b418C[][11];
extern byte b425D[];
extern byte b4273[];
extern byte b42F9[];
extern byte b43F8[];
extern byte b44F7[];
extern byte b46EB[];
extern byte b499B[];
extern byte b4A21[];
extern byte b4C15[];
extern byte b4C2D[];
extern byte b4C45[];
extern byte b4CB4[];
extern byte b4D23[][16];
extern byte b4FA3[];
extern byte b5012[];
extern byte b5048[];
extern byte b50AD[];
extern byte b5112[];
extern byte b5124[];
extern byte b51E3[];
extern byte b5202[];
extern byte b5221[];
extern byte b528D[];
extern byte b52B5[];
extern byte b52DD[][11];
extern byte unused[];
extern word w48DF[];
extern word w493D[];
extern word w502A[];

/* plm3a.plm */
extern byte b42A8[];
extern byte b42D6[];
extern byte b4813[];
extern byte printOrObj;
extern byte recLocals[];
extern byte recPublicAbs[];
extern byte recPublicCode[];
extern byte recPublicData[];
extern byte recPublicMemory[];
extern byte recExternals[];
extern byte recModHdr[];
extern byte recInitContent[];
extern byte recExec[];
extern word curOffset;

/* plm3a.plm,pdata4.plm */
extern byte objBuf[];
extern byte recExtFixup[];
extern byte recSelfFixup[];
extern byte recCodeFixup[];
extern byte recDataFixup[];
extern byte recMemoryFixup[];
extern byte recLineNum[];
extern byte recEnd[];
//

/* pdata4.plm */
extern byte b9692;
extern byte b969C;
extern byte b969D;
extern char ps96B0[];
// extern byte b96B1[];
extern byte b96D6;
extern word baseAddr;
extern bool bo812B;
extern bool linePrefixChecked;
extern bool linePrefixEmitted;
extern byte cfCode;
extern char commentStr[];
extern byte curExtId;
extern word blkCnt;
extern byte dstRec;
// extern byte endHelperId; now local var
extern byte helperId;
// extern byte helperModId; now local var
extern char helperStr[]; // pstr
typedef struct {
    byte len;
    char str[81];
} line_t;
extern line_t line;
extern char locLabStr[];
extern err_t errData;
extern uint16_t lstLineLen;
extern char lstLine[];
extern byte opByteCnt;
extern byte opBytes[];

extern word stmtNo;
extern pstr_t *sValAry[];
extern word lineNo;
extern pstr_t *ps969E;
extern word locLabelNum;
extern word wValAry[];

/* pdata4.plm,pdata6.plm */
extern bool codeOn;
extern word stmtCnt;
extern bool listing;
extern bool listOff;
extern byte srcbuf[];

/* plm4a.plm */
extern byte b42A8[];
extern byte b42D6[];
extern byte b4029[];
extern byte b4128[];
extern byte b413B[];
extern byte b425D[];
extern byte b4273[];
extern byte b4602[];
extern byte b473D[];
extern byte b475E[];
extern byte b4774[];
extern byte b478A[];
extern byte b47A0[];
extern byte b4A03[];
extern byte b4A78[];
extern byte opcodes[];
extern byte regIdx[];
extern byte regNo[];
extern byte stackOrigin[];
extern byte regPairIdx[];
extern byte regPairNo[];
extern word w47C1[];
extern word w4919[];
extern word w506F[];

/* main5.plm */
extern byte groupingChar;
extern offset_t dictionaryP;
extern word dictCnt;
extern offset_t dictTopP;
extern byte maxSymLen;
extern offset_t xrefIdx;

/* pdata6.plm */
extern bool b7AD9;
extern bool b7AE4;
extern word lineCnt;
extern word blkCnt;
extern word stmtNo;

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
extern byte wrapMarkerCol;
extern byte wrapMarker;
extern byte wrapTextCol;
extern byte skipCnt;
extern word blockDepth;
extern byte col;
extern byte controls[];
extern word csegSize;
extern index_t infoIdx; // individually cast
extern info_t *info;
extern index_t curSym;
extern char DATE[];
extern word dsegSize;
extern byte fatalErrorCode;
extern bool hasErrors;
extern index_t hashTab[];
extern bool haveModuleLevelUnit;
extern word helpers[];
extern word intVecLoc;
extern byte intVecNum;
extern file_t ixiFile;
extern char *ixiFileName;
extern char *depFileName;

extern word LEFTMARGIN;
extern word linesRead;
extern byte linLft;
extern word localLabelCnt;
extern word *localLabels;
extern file_t lstFile;
extern char *lstFileName;
extern bool isList;
extern byte margin;
extern file_t objFile;
extern char *objFileName;
extern bool moreCmdLine;
extern byte PAGELEN;
extern word pageNo;

extern word procChains[];
extern word procCnt;
extern word procInfo[];
extern word programErrCnt;
extern byte PWIDTH;
extern file_t srcFil;
extern word srcFileIdx;
extern file_t srcFileTable[]; /* 6 * (8 words fNam, blkNum, bytNum) */
extern byte srcStemLen;
extern byte srcStemName[];
extern bool standAlone;
extern offset_t startCmdLineP;
extern char TITLE[];
extern byte TITLELEN;
extern byte tWidth;
extern vfile_t utf1;
extern vfile_t utf2;
extern bool afterEOF;
extern char version[];
extern byte *procIds;
extern offset_t w3822;
extern word cmdLineCaptured;
extern vfile_t xrff;

extern index_t symCnt;
extern sym_t *symtab;

extern index_t infoCnt;
extern info_t *infotab;

extern index_t dictCnt;
extern index_t *dicttab;

extern index_t caseCnt;
extern index_t *casetab;

extern index_t xrefCnt;
extern xref_t *xreftab;

extern char const **includes;
extern uint16_t includeCnt;


/* creati.c */
void CreateInfo(word scope, byte type, index_t sym);

/* endcom.c */
void EndCompile(void);

/* fatal.c */
void Fatal(char const *str);

/* fi.c */
bool FindInfo(void);
void AdvNxtInfo(void);
void FindMemberInfo(void);
bool FindScopedInfo(word scope);

/* io.c */
void Error(word ErrorNum);

/* lookup.c */
byte Hash(pstr_t *pstr);
void Lookup(pstr_t *pstr);

/* lstinf.c */
void LstModuleInfo(void);

/* lstsup.c */
void NewLineLst(void);
void TabLst(byte tabTo);
void DotLst(byte tabTo);
void EjectNext(void);
void SetMarkerInfo(byte markerCol, byte marker, byte textCol);
void SetStartAndTabW(byte startCol, byte width);
void SetSkipLst(byte cnt);
void lstStr(char const *str);
void lstStrCh(char const *str, int endch);
void lprintf(char const *fmt, ...);
void lstPstr(pstr_t *ps);
pstr_t const *hexfmt(byte digits, word val);

/* main.c */
void Start(void);
void usage(void);

/* main0.c */
void unwindInclude(void);
word Start0(void);

/* main1.c */
word Start1(void);

/* main2.c */
word Start2(void);

/* main3.c */
word putWord(pointer buf, word val);
word getWord(pointer buf);
word Start3(void);

/* main4.c */
word Start4(void);

/* main5.c */
void LoadDictionary(void);
int CmpSym(void const *a, void const *b);
void SortDictionary(void);
void PrepXref(void);
void PrintRefs(void);
void CreateIxrefFile(void);
void ProcessXref(void);
word Start5(void);

/* main6.c */
word Start6(void);

/* page.c */
void NewPgl(void);
void NlLead(void);

/* plm0a.c */
void Wr1LineInfo(void);
void Wr1Buf(void const *buf, word len);
void Wr1Byte(uint8_t v);
void Wr1Word(uint16_t v);
void Rd1Buf(void *buf, uint16_t len);
uint8_t Rd1Byte(void);
uint16_t Rd1Word(void);
void Wr1InfoOffset(offset_t addr);
void Wr1SyntaxError(byte err);
void Wr1TokenErrorAt(byte err);
void Wr1TokenError(byte err, offset_t symP);
_Noreturn void LexFatalError(byte err);
void PushBlock(word idAndLevel);
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
bool YylexMatch(byte token);
bool YylexNotMatch(byte token);
void ParseExpresion(byte endTok);
void ParseDeclareNames(void);
void ParseDeclareElementList(void);
void ProcProcStmt(void);

/* plm0f.c */
void ParseProgram(void);

/* plm0g.c */
pstr_t const *CreateLit(word wTokenLen, char const *str);

/* plm1a.c */
void FatalError_ov1(byte err);
void OptWrXrf(void);
void Wr2Buf(void *buf, word len);
void Wr2Byte(uint8_t v);
void Wr2Word(uint16_t v);
void Rd2Buf(void *buf, uint16_t len);
uint8_t Rd2Byte(void);
uint16_t Rd2Word(void);
void Wr2LineInfo(void);
void Wr2Item(uint8_t type, void *buf, uint16_t len);
word WrTx2Item(byte type);
word WrTx2Item1Arg(byte type, word arg2w);
word WrTx2Item2Arg(byte type, word arg2w, word arg3w);
word WrTx2Item3Arg(byte type, word arg2w, word arg3w, word arg4w);
word Sub_42EF(word arg1w);
void MapLToT2(void);
void WrTx2Error(byte arg1b);
void WrTx2ExtError(byte arg1b);
void SetRegetTx1Item(void);
void RdTx1Item(void);

/* plm1b.c */
void GetTx1Item(void);
bool MatchTx1Item(byte arg1b);
bool NotMatchTx1Item(byte arg1b);
bool MatchTx2AuxFlag(byte arg1b);
void RecoverRPOrEndExpr(void);
void ResyncRParen(void);
void ExpectRParen(byte arg1b);
void ChkIdentifier(void);
void ChkStructureMember(void);
void GetVariable(void);
void WrAtBuf(void const *buf, word cnt);
void WrAtByte(byte arg1b);
void WrAtWord(word arg1w);

/* plm1c.c */
void RestrictedExpression(void);
word InitialValueList(offset_t infoOffset);
void ResetStacks(void);
void PushParseWord(word arg1w);
void PopParseStack(void);
void PushParseByte(byte arg1b);
void ExprPop(void);
void ExprPush2(byte icode, word val);
void MoveExpr2Stmt(void);
void PushOperator(byte arg1b);
void PopOperatorStack(void);
void ExprMakeNode(byte icode, byte argCnt);
void AcceptOpAndArgs(void);
void FixupBased(offset_t arg1w);
void Sub_4D2C(void);
void ChkTypedProcedure(void);
byte GetCallArgCnt(void);
void Sub_4DCF(byte arg1b);
void MkIndexNode(void);
void ParsePortNum(byte arg1b);
void Sub_50D5(void);
byte Sub_512E(word arg1w);
void ConstantList(void);

/* plm1d.c */

void ExpressionStateMachine(void);

/* plm1e.c */
byte Sub_5945(void);
byte Sub_59D4(void);
void Expression(void);
word SerialiseParse(word arg1w);
void Initialization(void);
void ParseLexItems(void);

/* plm1f.c */
word GetElementSize();
int32_t RdAtByte(void);
int32_t RdAtWord(void);
void RdAtHdr(void);
void RdAtData(void);
void Sub_6EE0(void);

/* plm2a.c */
void WrFragData(void);
void PutTx1Byte(byte arg1b);
void PutTx1Word(word arg1w);
void EncodeFragData(byte arg1b);
void EmitTopItem(void);
void Tx2SyntaxError(byte arg1b);
byte Sub_5679(byte arg1b);
void Sub_56A0(byte arg1b, byte arg2b);
byte Sub_5748(byte arg1b);
word Sub_575E(offset_t arg1w);
void Sub_5795(word arg1w);
bool EnterBlk(void);
bool ExitBlk(void);
void Sub_58F5(byte arg1b);
void Sub_597E(void);
void Sub_5B96(byte arg1b, byte arg2b);
void Sub_5C1D(byte arg1b);
void Sub_5C97(byte arg1b);
void Sub_5D27(byte arg1b);
void Sub_5D6B(byte arg1b);
void Sub_5E66(byte arg1b);
void Sub_5EE8(void);
void Sub_5F4B(word val, word infoIdx, byte isWord, byte opFlag);
void GetVal(byte arg1b, wpointer arg2wP, wpointer arg3wP);
void Sub_611A(void);
void Sub_61A9(byte arg1b);
void Sub_61E0(byte arg1b);
void Sub_636A(byte arg1b);
void Sub_63AC(byte arg1b);
void Sub_6416(byte arg1b);
void GetTx2Item(void);
void Sub_652B(void);
void FillTx2Q(void);
void Sub_67A9(void);

/* plm2b.c */
void Sub_689E(void);

/* plm2c.c */
void Sub_7055(void);
void Sub_6BD6(void);

/* plm2d.c */
void Sub_717B(void);
void Sub_7550(void);

/* plm2e.c */
void Sub_7A85(void);
void Sub_7DA9(void);
void Sub_84ED(void);

/* plm2f.c */
void Sub_87CB(void);
void Sub_9457(void);

/* plm2g.c */
void FindParamInfo(byte arg1b);
void Sub_9560(void);
void Sub_9624(word arg1w);
void Sub_9646(word arg1w);
void Inxh(void);
void OpB(byte arg1b);
void OpD(byte arg1b);
void Sub_9706(void);
void MovDem(void);
void Sub_975F(void);
void Sub_978E(void);
void Sub_981C(void);
void Sub_994D(void);

/* plm2h.c */
void Sub_9BB0(void);
void Sub_9D06(void);
void Sub_9DD7(void);
void Sub_9EF8(void);
void Sub_9F14(void);
void Sub_9F2F(void);
void Sub_9F9F(void);
void Sub_A072(byte arg1b);
void Sub_A0C4(void);
void Sub_A10A(void);
void Sub_A153(void);

/* plm3a.c */
void FlushRecGrp(void);
void RecAddName(pointer recP, byte offset, byte len, char const *str);
void ExtendChk(pointer arg1wP, word arg2w, byte arg3b);
word CalcMaxStack(void);

void p3Error(word errNum, info_t *tokInfo, word stmt);
void Sub_49F9(void);

/* plm3b.c */
void WriteRec(pointer rec, byte fixed);
void RecAddByte(pointer rec, byte offset, byte val);
void RecAddWord(pointer rec, byte offset, word val);

/* plm4a.c */
void Sub_54BA(void);

/* plm4b.c */
void FlushRecs(void);
void AddWrdDisp(pstr_t *pstr, word arg2w);
void EmitLinePrefix(void);
void EmitStatementNo(void);
void EmitLabel(char const *label);
char const *FindErrStr(void);
void EmitError(void);
void FatalError_ov46(byte arg1b);
void ListCodeBytes(void);
void GetSourceLine(void);

/* plm4c.c */
pstr_t *pstrcpy(pstr_t *dst, pstr_t const *src);
pstr_t *pstrcat(pstr_t *dst, pstr_t const *src);
void Sub_5FE7(word arg1w, byte arg2b);
void Sub_66F1(void);
void Sub_6720(void);
void Sub_668B(void);

/* plm6a.c */
void MiscControl(vfile_t *txFile);
void Sub_42E7(void);

/* plma.c */
void SignOnAndGetSourceName(void);

/* plmb.c */
void InitKeywordsAndBuiltins(void);
void SetDate(char *str, byte len);
void SetPageLen(word len);
void SetPageNo(word v);
void SetMarginAndTabW(byte startCol, byte width);
void SetTitle(char *str, byte len);
void SetPageWidth(word width);

/* plmfile.c */
void CloseF(file_t *fileP);
void OpenF(file_t *fileP, char const *sNam, char const *fNam, char *access);

/* prints.c */
void PrintStr(char const *str, byte len);

/* putlst.c */
void lstc(byte ch);

/* symtab.c */
pstr_t const *pstrdup(pstr_t const *ps);
bool pstrequ(pstr_t const *ps, pstr_t const *pt);
index_t newSymbol(pstr_t const *ps);
void newInfo(byte type);
index_t newDict(index_t idx);
index_t newCase(word val);
index_t newXref(index_t scope, word line);
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
