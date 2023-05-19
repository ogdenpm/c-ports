/****************************************************************************
 *  data.h: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/
#include "plm80types.h"
// defined in asm2m.c
extern byte opFlags[];
// defined in asm5m.c
extern byte labelUse;
// defined in startm.c
extern byte b3782[2];

extern char moduleName[];


// defined in globlm.c

#define	IN_BUF_SIZE	512
#define	OUT_BUF_SIZE	512
#define MAXFILEPARAM	260

extern char macroLine[129];
extern char *macroP;
extern bool inQuotes;
extern bool excludeCommentInExpansion;
extern bool inAngleBrackets;
extern byte expandingMacro;
extern bool expandingMacroParameter;
extern bool inMacroBody;
extern byte mSpoolMode;
extern bool b9060;
extern bool nestMacro;
extern byte savedMtype;
extern byte macroDepth;
extern byte macroSpoolNestDepth;
extern byte paramCnt;
extern byte startNestCnt;
extern byte argNestCnt;
extern tokensym_t *pMacro;
extern pointer macroInPtr;
extern macro_t macro[10];
#define curMacro macro[0]
extern word curMacroBlk;
extern word nxtMacroBlk;
extern word maxMacroBlk;
extern word macroBlkCnt;
extern byte macroBuf[129];
extern pointer savedMacroBufP;
extern pointer pNextArg;
extern word localIdCnt;
extern pointer startMacroLine;
extern pointer startMacroToken;
extern byte irpcChr[3];

extern byte localVarName[];
extern pointer contentBytePtr;
extern byte fixupSeg;
extern word fixOffset;
extern byte curFixupHiLoSegId;
extern byte curFixupType;
extern byte fixIdxs[4];
#define	fix22Idx	fixIdxs[0]
#define	fix24Idx	fixIdxs[1]
#define	fix20Idx	fixIdxs[2]
#define	fix6Idx		fixIdxs[3]
extern byte extNamIdx;
extern bool initFixupReq[4];
extern bool firstContent;
extern byte rEof[];
extern byte rExtnames[];
extern byte moduleNameLen;
extern byte rContent[];
extern byte rPublics[];
#define rReloc	rPublics	// shared
extern byte rInterseg[];
extern byte rExtref[];
extern byte rModend[];
extern bool inComment;
extern bool noOpsYet;
extern byte nameLen;
extern byte startSeg;
extern byte activeSeg;
extern bool inPublic;
extern bool inExtrn;
extern bool segDeclared[2];
extern byte alignTypes[4];
extern word externId;
extern word itemOffset;
extern bool badExtrn;
extern byte startDefined;	/* 0 or 1 */
extern word startOffset;
extern byte tokenIdx;
extern byte lineBuf[128];
extern token_t token[];
#define tokPtr	token[0].start
#define topSymbol token[0].symbol
extern pointer endLineBuf;
extern byte ifDepth;
extern bool skipIf[9];
extern bool inElse[9];
extern byte macroCondSP;
extern byte macroCondStk[17];
extern byte opSP;
extern byte opStack[17];
extern word accum1, accum2;
#define Low(n)   ((n) & 0xff)
#define High(n)  (((n) >> 8) & 0xff)
#define MkWord(h, l) (((h) << 8) | (l))
extern byte acc1RelocFlags;
extern byte acc2RelocFlags;
extern bool hasVarRef;
extern byte acc1ValType;
extern byte acc2ValType;
extern word acc1RelocVal;
extern word acc2RelocVal;
extern byte curChar;
extern byte reget;
extern byte lookAhead;
extern tokensym_t *symTab[3];
extern tokensym_t *endSymTab[3];
extern pointer baseMacroTbl;
extern byte gotLabel;
extern char name[MAXSYMSIZE + 1];
extern char savName[MAXSYMSIZE + 1];
extern bool haveNonLabelSymbol;
extern bool haveUserSymbol;
extern bool xRefPending;
extern byte passCnt;
extern bool createdUsrSym;
extern bool usrLookupIsID;
extern bool needsAbsValue;
extern FILE *objFp;
extern FILE *inFp;
extern FILE *lstFp;
extern FILE *macroFp;
extern word statusIO;
extern word openStatus;  /* status of last open for Read */
extern byte asmErrCode;
extern bool spooledControl;
extern bool primaryValid;
extern byte tokI;
extern bool errorOnLine;
extern bool atStartLine;
extern byte curCol;
extern pointer endItem;
extern pointer startItem;
extern word pageLineCnt;
extern word effectiveAddr;
extern word pageCnt;
extern bool showAddr;
extern bool lineNumberEmitted;
extern bool b68AE;
extern char tokStr[7];
extern char inBuf[];
extern char *objFile;
extern char *lstFile;
extern word srcLineCnt;
extern word lineNo;
extern byte spIdx;
extern word lastErrorLine;
extern controls_t controls;
extern bool ctlListChanged;
extern byte titleLen;
extern bool controlSeen[12];
extern byte saveStack[8][3];
extern byte saveIdx;
extern char titleStr[64];
extern word tokBufLen;
extern byte tokType;
extern byte controlId;
extern char tokBuf[];
extern word tokBufIdx;
extern word tokNumVal;
extern bool isControlLine;
extern bool scanCmdLine;
extern bool inDB;
extern bool inDW;
extern bool inExpression;
extern bool has16bitOperand;
extern byte phase;
extern byte curOpFlags;
extern byte yyType;
extern byte curOp;
extern byte topOp;
extern bool b6B2C;
extern byte nextTokType;
extern bool finished;
extern bool inNestedParen;
extern bool expectingOperands;
extern bool expectingOpcode;
extern bool condAsmSeen;    /* true when IF, ELSE, ENDIF seen [also macro to check] */
extern bool b6B33;
extern bool isInstr;
extern bool expectOp;
extern bool b6B36;
extern word segLocation[5];
extern word maxSegSize[3];
extern char *cmdLineBuf;
extern word errCnt;
extern pointer w6BCE;
extern char *cmdchP;
extern char *controlsP;
extern bool skipRuntimeError;
extern bool nestedMacroSeen;
extern byte ii;
extern byte jj;
extern byte kk;
extern char *curFileNameP;

extern address aVar;

// defined in keym.h
extern tokensym_t extKeywords[151];
#define MAXSYMBOLS	8192		// way more than original ASM80 could handle
#define MAXMACROTEXT	512
#define MAXMACROPARAM	1024
extern tokensym_t symbols[MAXSYMBOLS];
extern byte macroText[MAXMACROTEXT];
extern byte macroParams[MAXMACROPARAM];


// defined in rdsrc.c

extern bool pendingInclude;
extern bool includeOnCmdLine;
extern byte fileIdx;
extern FILE *srcfp;
extern byte lineChCnt;
extern file_t files[6];


extern int _argc;
extern char **_argv;
extern bool useLC;
extern bool killObjFile;
extern int maxSymWidth;