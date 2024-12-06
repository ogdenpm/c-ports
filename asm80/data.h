/****************************************************************************
 *  data.h: part of the C port of Intel's ISIS-II asm80                     *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm80types.h"
#include "../shared/os.h"
// defined in asm2m.c
extern byte opFlags[];
// defined in asm5m.c
extern byte labelUse;
// defined in startm.c
extern byte b3782[2];

extern char moduleName[];

// defined in globlm.c

#define MAXFILENAME _MAX_PATH
#define MAXTOKENS   20
#define MAXSYMSIZE  31

extern char *macroLine;
extern int macroPIdx;
extern bool inQuotes;
extern bool excludeCommentInExpansion;
extern byte expandingMacro;
extern bool expandingMacroParameter;
extern bool inMacroBody;
extern byte mSpoolMode;
extern bool b9060;
extern bool nestMacro;
extern byte savedMtype;
extern byte macroDepth;
extern byte macroSpoolNestDepth;

extern byte argNestCnt;
extern int macroInIdx;
extern macro_t macro[10];
#define curMacro macro[0]
extern word curMacroBlk;

extern word macroBlkCnt;
extern byte macroBuf[129];

extern word localIdCnt;
extern int startMacroLineIdx;
extern int startMacroTokenIdx;

extern byte rExtnames[];
extern byte moduleNameLen;

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

extern bool badExtrn;
extern byte startDefined; /* 0 or 1 */
extern word startOffset;
extern byte tokenIdx;
extern byte *lineBuf;

extern token_t tokenStk[];
#define token      tokenStk[0]
#define tokenStart (lineBuf + token.start)
extern byte ifDepth;
extern bool skipIf[9];
extern bool inElse[9];
extern byte macroCondSP;
extern byte macroCondStk[17];
extern byte opSP;
extern byte opStack[17];
extern word accum1, accum2;
#define Low(n)       ((n)&0xff)
#define High(n)      (((n) >> 8) & 0xff)
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
extern pointer topMacroArg;
extern byte haveLabel;
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
extern byte asmErrCode;
extern bool spooledControl;
extern bool primaryValid;
extern byte tokI;
extern bool errorOnLine;
extern bool atStartLine;
extern int endItem;
extern int startItem;
extern word pageLineCnt;
extern word effectiveAddr;
extern word pageCnt;
extern bool showAddr;
extern bool lineNumberEmitted;
extern byte subHeadIdx;
extern char *inBuf;
extern char *inPtr;
extern char *objFile;
extern char *lstFile;
extern word srcLineCnt;
extern word lineNo;
extern byte spIdx;
extern controls_t controls;
extern word pageWidth;  // allows for > 255
extern word pageLength; // allows for > 255
extern bool ctlListChanged;
extern bool controlSeen[NCONTROL];
extern byte saveStack[8][3];
extern byte saveIdx;
extern char titleStr[];
extern word tokBufLen;
extern byte tokType;
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
extern bool expectOperand;
extern bool expectOpcode;
extern bool condAsmSeen; /* true when IF, ELSE, ENDIF seen [also macro to check] */
extern bool b6B33;
extern bool isInstr;
extern bool expectOp;
extern bool b6B36;
extern word segLocation[5];
extern word maxSegSize[3];
extern char *cmdLineBuf;
extern word errCnt;
extern char *cmdchP;
extern char *controlsP;
extern bool skipRuntimeError;
extern bool nestedMacroSeen;
extern char *curFileNameP;

/*
    The symbol table and macro arg handling is left as per the original
    Changing the symbol table handling to use say hashed lookup would
    change the order of symbols in the object file
    Changing the handling of macro arg text requires considerable changes
    and just allowing for a large expansion area is easier.
    It you get an out of space error, just increase the value of MAXMACROARGTEXT
    and recompile
    Other tables now grow automatically
 */
#define MAXSYMBOLS      8192 // way more than original ASM80 could handle
#define MAXMACROARGTEXT 4096 // allows for lots of macro arg text
extern tokensym_t symbols[MAXSYMBOLS];
extern byte *macroText;
extern byte macroArgs[MAXMACROARGTEXT];

// defined in rdsrc.c

extern bool pendingInclude;
extern bool includeOnCmdLine;
extern byte fileIdx;
extern FILE *srcfp;
extern file_t files[6];

extern int _argc;
extern char **_argv;
extern bool killObjFile;
extern int maxSymWidth;
extern char dateStr[22]; // [yyyy-mm-dd hh:mm]
extern char *depFile;

extern int maxSymbolSize;