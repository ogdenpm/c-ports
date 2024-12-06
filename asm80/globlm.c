/****************************************************************************
 *  globlm.c: part of the C port of Intel's ISIS-II asm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"

char *macroLine;

int macroPIdx = 0;
bool inQuotes = false;
bool excludeCommentInExpansion;

byte expandingMacro; // 0,1 or 0xff
bool expandingMacroParameter;
bool inMacroBody = false;
byte mSpoolMode; // 0->normal, 1->spool (local ok), 2->capture locals, 0xfe->might not occur ,
                 // 0xff->capture body
bool b9060;
bool nestMacro;
byte savedMtype;
byte macroDepth;
byte macroSpoolNestDepth;

byte argNestCnt = 0;

int macroInIdx;
/*
    mtype has the following values
    1 -> IRP
    2 -> IRPC
    3 -> DoRept
    4 -> MACRO Invocation
    5 -> ???
*/

macro_t macro[10] = { { .blk = 0xffff } };

word curMacroBlk  = 0xFFFF;

word macroBlkCnt;
byte macroBuf[129];

word localIdCnt;
int startMacroLineIdx;
int startMacroTokenIdx;

byte moduleNameLen = 6;

bool inComment     = false;
bool noOpsYet      = false;
byte nameLen;
byte startSeg = 1;
byte activeSeg;
bool inPublic = false;
bool inExtrn  = 0;
bool segDeclared[2];
byte alignTypes[4] = { 3, 3, 3, 3 };
word externId;

bool badExtrn     = false;
byte startDefined = 0;
word startOffset  = 0;
byte tokenIdx     = 0;
byte *lineBuf;

token_t tokenStk[MAXTOKENS];
byte ifDepth = 0;
bool skipIf[9];
bool inElse[9];
byte macroCondSP = 0;
byte macroCondStk[17];
byte opSP;
byte opStack[17];
word accum1, accum2;
byte acc1RelocFlags;
byte acc2RelocFlags;
bool hasVarRef;
byte acc1ValType;
byte acc2ValType;
word acc1RelocVal;
word acc2RelocVal;
byte curChar = 0;
byte reget   = false;
byte lookAhead;
// static byte pad6861 = 0;
tokensym_t *symTab[3];
tokensym_t *endSymTab[3];
tokensym_t symbols[MAXSYMBOLS];
byte *macroText;
byte macroArgs[MAXMACROARGTEXT];

pointer topMacroArg;
byte haveLabel = 0;
char name[MAXSYMSIZE + 1]; // '\0' added to end
char savName[MAXSYMSIZE + 1];
bool haveNonLabelSymbol; // true if we have seen a user symbol and confirmed that there is no :
bool haveUserSymbol;     // true if we have seen a user symbol and  not yet seen a following :
bool xRefPending   = false;
byte passCnt       = 0;
bool createdUsrSym = false;
bool usrLookupIsID = false;
bool needsAbsValue = false;
FILE *objFp;
FILE *lstFp;
byte asmErrCode     = ' ';
bool spooledControl = false;
bool primaryValid   = true;
byte tokI;
bool errorOnLine;
bool atStartLine;

int endItem;
int startItem;
word pageLineCnt;
word effectiveAddr;
word pageCnt;
bool showAddr;
bool lineNumberEmitted = false;
byte subHeadIdx        = 0;

char *objFile;
char *lstFile;
word srcLineCnt = 1;

word lineNo;
byte spIdx;

controls_t controls  = { .all = {
                            false, false, false, true,  true, false, true, true, true, true,
                            120,   66,    0,     false, 0,    0,     0,    true, true, true, false, false } };
word pageWidth       = 120;
word pageLength      = 66;

bool ctlListChanged  = true;
bool controlSeen[NCONTROL];
byte saveStack[8][3];
byte saveIdx = 0;
char titleStr[65];
word tokBufLen;
byte tokType;
char tokBuf[MAXFILENAME + 1];
word tokBufIdx = 0;
word tokNumVal;
bool isControlLine = false;
bool scanCmdLine;
bool inDB;
bool inDW;
bool inExpression;
bool has16bitOperand;
byte phase;
byte curOpFlags;
byte yyType;
byte curOp;
byte topOp;
bool b6B2C;
byte nextTokType;
bool finished;
bool inNestedParen;
bool expectOperand;
bool expectOpcode;
bool condAsmSeen; /* true when IF, ELSE, ENDIF seen [also macro to check] */
bool b6B33;
bool isInstr        = true;
bool expectOp       = true;
bool b6B36          = false;
word segLocation[5] = { 0, 0, 0, 0, 0 }; /* ABS, CODE, DATA, STACK, MEMORY */
word maxSegSize[3]  = { 0, 0, 0 };       /* seg is only ABS, CODE or DATA */
char *cmdLineBuf;
word errCnt;
char *cmdchP;
char *controlsP;
bool skipRuntimeError = false;
bool nestedMacroSeen;
char *curFileNameP;

char *depFile;