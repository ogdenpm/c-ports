/****************************************************************************
 *  data.c: part of the C port of Intel's ISIS-II plm80                     *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

sym_t *curSym;
info_t *info;
bool moreCmdLine = true;
uint16_t LEFTMARGIN;
uint16_t localLabelCnt;
uint16_t srcFileIdx;
sym_t *hashTab[64]; // offset is to pointer to array of offsets
uint16_t scopeSP;
uint16_t *localLabels;
uint8_t *procIds;
uint16_t helperAddr[117];
uint16_t linesRead;
uint16_t programErrCnt;
uint16_t procCnt;
uint16_t cmdLineCaptured = 0;
uint16_t dsegSize        = 0;
uint16_t csegSize        = 0;
file_t srcFileTable[6];
file_t srcFil;
file_t lstFile;
file_t objFile;
vfile_t utf1;
vfile_t utf2;
vfile_t atf;
file_t ixiFile;
uint16_t scopeChains[35];
info_t *procInfo[255];
uint8_t srcStemLen;
bool standAlone          = true;

bool afterEOF            = false;
bool haveModuleLevelUnit = false;
uint16_t fatalCode;
uint8_t controls[9];
uint8_t srcStemName[10];
bool debugSwitches[26];
offset_t cmdLineP;
offset_t startCmdLineP;
char *depFileName;
char *ixiFileName;
char *lstFileName;
char *objFileName;
bool isList = false;
uint16_t pageNo = 0;
uint8_t linLft = 0;
uint8_t wrapMarkerCol, wrapTextCol;
uint8_t col      = 0;
uint8_t skipCnt  = 0;
uint8_t tWidth   = 4;

uint8_t PAGELEN  = 57;
uint8_t PWIDTH   = 120;
uint8_t margin   = 0xFF;
char DATE[20];  // max yyyy-mm-dd hh:mm:ss\0
uint16_t DATELEN;
uint16_t TITLELEN;
char TITLE[61]; // allow to be '\0' terminated

uint8_t intVecNum = 8;
uint16_t intVecLoc = 0;
bool hasErrors = false;
char version[] = VERSION;

