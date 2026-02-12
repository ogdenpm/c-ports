/****************************************************************************
 *  main2.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"
#include <setjmp.h>

// clang-format off
uint16_t nodeTypeToAttribute[] = {
    0x11B, 0x14B, 0x12B, 0x12B, 0x11B, 0x14B, 0x60,  0x60,  0x62,  0x62,  0x5B,  0x62,  0xB,   0x1E4, 0x1E7, 0x1E8,
    0x1EE, 0x1F1, 0x8D,  0xCF,  0x10B, 0xE,   0x12,  0x14,  0xEB,  0xEB,  0xEB,  0xA9,  0x9A,  0x96,  0xA1,  0x69,
    0x68,  0x70,  0x76,  0x90,  0xEB,  0x10B, 0x15B, 0x18B, 0x1BB, 0x19B, 0x17B, 0x16B, 0x58,  0x19,  0x20,  0x27,
    0x19,  0x20,  0x27,  0x16,  0x17,  0x18,  0x16,  0x17,  0x18,  0xA,   0x38,  0x3E,  0x67,  0x2A,  6,     0xC,
    0x1DE, 0x80,  0x43,  0x45,  0x47,  0x1DB, 0x49,  0xC9,  0xCD,  0xCE,  0xB7,  0xBA,  0xBD,  0x41,  0x42,  0x44,
    0x46,  0x48,  0x4A,  0x4B,  0x4C,  0x41,  0x42,  0x44,  0x46,  0x48,  0x4A,  0x4B,  0x4C,  0x7C,  0x42,  0x44,
    0x46,  0x48,  0x48,  0x46,  0x44,  0x42,  0x7C,  0x42,  0x44,  0x46,  0x48,  0x48,  0x46,  0x44,  0x42,  0x4F,
    0x42,  0x44,  0x46,  0x48,  0x4A,  0x4D,  0x4E,  0x57,  0x50,  0x51,  0x52,  0x53,  0x54,  0x55,  0x56,  0xC0, 
    0xC3,  0xC6,  0xCF, 0x1DC,  0,     0
};

// clang-format on
blk_t blk[20];

uint16_t callStackDepth[10];
uint16_t callStackBase[10];
//typedef struct {
//    uint8_t type;
//    index_t left;
//    index_t right;
//    index_t extra;
//    uint8_t exprAttr;
//    uint8_t exprLoc;
//    uint16_t cnt;
//} tx2_t;
tx2_t tx2[255] = { { T2_SEMICOLON, 0, 0, 0, LIT_A, 0, 0 },
                   { T2_LOCALLABEL, 0, 0, 0, LABEL_A, 0, 1 },
                   { T2_SEMICOLON, 0, 0, 0, 0, 0, 0 },
                   { T2_SEMICOLON, 0, 0, 0, 0, 0, 0 }};


RegisterState registerState[9]; // Unified register tracking

bool registerInExpression[9]; // Used by active expression
bool registerNeedsSave[9]; // Must be preserved
bool registerWasSaved[9]; // Was marked for save
bool registerHasValue[9]; // Contains valid data
uint8_t savedRegisterCount; // Registers to save
uint8_t exprRegisterCount; // Active expr reg count
uint8_t exprAttr[2];
uint8_t exprLoc[2];
uint8_t curExprLoc[2];
uint8_t operandComplexity[2];   // bC0B9[] - Complexity cost
uint8_t operandRegisterCost[2]; // bC0BB[] - Register cost
uint8_t operandCategory[2];     // bC0BD[] - Operation category
uint8_t operandFragmentType[2]; // bC0BF[] - Fragment type
uint8_t iCodeArgsIndex[2];
uint8_t stackRegisterAttrs[125]; // Packed attribute+offset
uint8_t stackNodeContents[125];  // TX2 node at each stack level
//uint8_t bC1BD = 0;
uint8_t tx2qp;     // current write position (4-255)
uint8_t tx2qNxt       = 4; // next unprocessed note (set by setEndFirstStmt)
uint8_t tx2qEnd       = 4; // endof valid data (=tx2qp on exit)
uint16_t codeSize            = 0;
uint16_t currentStackDepth         = 0;  // wC1C3 - Stack depth
uint16_t stackUsage         = 0;
uint16_t localVariableSize         = 0;
uint8_t activeGrpCnt         = 0;
uint8_t blkOverCnt    = 0;
uint8_t procCallDepth = 0;
bool returnGenerated        = false;
bool nextReturnState;
bool eofSeen = false;   // Stop filling when T2_EOF encountered
uint8_t curNodeType;
uint8_t nodeControlFlags;
uint8_t curExtProcId = 1;
uint8_t blkId  = 0;
uint16_t currentFragmentCode;
bool conflictMode = false;   //b0C1D8
uint8_t selectedOperatorIdx;
uint8_t cfrag1;
uint8_t iCodeArgsIdx;
uint16_t iCodeArgs[5];
uint8_t fragLen;
uint8_t fragment[34];
uint8_t bC209[]     = { 4, 5, 3, 2, 0, 1 };
bool invertComparison      = false; // b0C20

uint8_t copyRight[] = "(C) 1976, 1977, 1982 INTEL CORP";

static void InitPass() {
    vfReset(&utf1);
    vfRewind(&utf2);
    blk[0].info =procInfo[1];
    programErrCnt = 0;
} /* Sub_3F27() */

static void CodeAndStackSize() {
    info = procInfo[1];
    info->codeSize   = codeSize;
    info->stackUsage = stackUsage;
} /* Sub_3F7D() */

uint16_t Start2() {
 //   dump(&utf2, "utf2_main2");  // diagnostic dump
    if (setjmp(exception) == 0) {
        InitPass();
        while (1) {
            FillTx2Queue();
            SetFirstStatementEnd();
            if (tx2[4].type == T2_EOF)
                break;
            DeRelStmt();
            OptimiseStmtNodes();
            GenerateStatementCode();
        }
    }
    /* longjmp comes here */
    CodeAndStackSize();
    return 3; // Chain(overlay[3]);
}
