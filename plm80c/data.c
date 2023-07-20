/****************************************************************************
 *  data.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"


index_t curSym;
index_t infoIdx;
bool moreCmdLine = true;
word LEFTMARGIN;
word localLabelCnt;
word srcFileIdx;
index_t hashTab[64]; // offset is to pointer to array of offsets
word blockDepth;
offset_t *localLabels;
byte *procIds;
offset_t helpers[117];
word caseLabels[];
word linesRead;
word programErrCnt;
word procCnt;
word cmdLineCaptured = 0;
word dsegSize = 0;
word csegSize = 0;
word objBlk;
word objByte;
file_t srcFileTable[6];
file_t srcFil;
file_t lstFile;
file_t objFile;
file_t conFile;
vfile_t utf1;
vfile_t utf2;
vfile_t atf;
vfile_t xrff;
file_t ixiFile;
word procChains[35];
word procInfo[255];
word blk1Used = 400;
word blk2Used = 400;
offset_t ov1Boundary = 0x9F00;    // last address of ov1 rounded up to page boundary + 0x100
word blkSize1 = 0xC400;     // last address of ov2 rounded up to page boundary
word blkSize2 = 0xA400;     // last address of ov4 rounded up to page boundary
byte srcStemLen;
bool standAlone = true;

bool afterEOF = false;
bool haveModuleLevelUnit = false;
byte fatalErrorCode = 0;
//byte pad3C43 = 1;
offset_t ov0Boundary = 0xA000;        // last address of ov0 rounded up to page boundary
byte controls[8];
//byte pad_3C4E[2];
byte srcStemName[10];
bool debugSwitches[26];
offset_t cmdLineP;
offset_t startCmdLineP;
//byte overlay[7][FILE_NAME_LEN] = { ":F0:PLM80 .OV0 ", ":F0:PLM80 .OV1 ", ":F0:PLM80 .OV2 ", ":F0:PLM80 .OV3 ",
//								   ":F0:PLM80 .OV4 ", ":F0:PLM80 .OV5 ", ":F0:PLM80 .OV6 "};
char *ixiFileName;
char *lstFileName;
char *objFileName;
bool isList = false;
word pageNo      = 0;
byte b3CF2;
word lChCnt = 0;
word lBufSz = 0;
byte linLft = 0;
byte wrapMarkerCol, wrapMarker, wrapTextCol;
byte col = 0;
byte skipCnt = 0;
byte tWidth = 0;
byte TITLELEN = 1;
byte PAGELEN = 60;
byte PWIDTH = 120;
byte margin = 0xFF;
char DATE[10];
char TITLE[60] = " ";
//word ISIS = 0x40;
word REBOOTVECTOR = 0;



byte intVecNum = 8;
word intVecLoc = 0;
bool hasErrors = false;
//byte overlay6[]  = ":F0:PLM80 ";
//byte ov6[] = ".OV6 ";
char version[] = "V4.0";
//byte pad3DA1;
//byte invokeName[] = ":F0:PLM80 ";
//byte ov0[] =  ".OV0 ";


