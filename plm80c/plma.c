/****************************************************************************
 *  plma.c: part of the C port of Intel's ISIS-II plm80                     *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "cmdline.h"
#include "os.h"
#include "plm.h"
#include <ctype.h>
#include "utility.h"

char *cmdTextP;

byte verNo[] = VERSION;

static struct {
    pstr_t *name;
    byte id;
    byte paramCnt;
    byte returnType;
} builtins[] = {
    // clang-format off
    {(pstr_t *)("\x5""CARRY"),  0,  0, BYTE_T},
    {(pstr_t *)("\x3""DEC"),    1,  1, BYTE_T},
    {(pstr_t *)("\x6""DOUBLE"), 2,  1, ADDRESS_T},
    {(pstr_t *)("\x4HIGH"),     3,  1, BYTE_T},
    {(pstr_t *)("\x5INPUT"),    4,  1, BYTE_T},
    {(pstr_t *)("\x4LAST"),     5,  1, ADDRESS_T},
    {(pstr_t *)("\x6LENGTH"),   6,  1, ADDRESS_T},
    {(pstr_t *)("\x3LOW"),      7,  1, BYTE_T},
    {(pstr_t *)("\x4MOVE"),     8,  3, 0},
    {(pstr_t *)("\x6OUTPUT"),   9,  1, 0},
    {(pstr_t *)("\x6PARITY"),   10, 0, BYTE_T},
    {(pstr_t *)("\x3ROL"),      11, 2, BYTE_T},
    {(pstr_t *)("\x3ROR"),      12, 2, BYTE_T},
    {(pstr_t *)("\x3SCL"),      13, 2, BYTE_T},
    {(pstr_t *)("\x3SCR"),      14, 2, BYTE_T},
    {(pstr_t *)("\x3SHL"),      15, 2, BYTE_T},
    {(pstr_t *)("\x3SHR"),      16, 2, BYTE_T},
    {(pstr_t *)("\x4SIGN"),     17, 0, BYTE_T},
    {(pstr_t *)("\x4SIZE"),     18, 1, BYTE_T},
    {(pstr_t *)("\x8STACKPTR"), 19, 0, ADDRESS_T},
    {(pstr_t *)("\x4TIME"),     20, 1, 0},
    {(pstr_t *)("\x4ZERO"),     21, 0, BYTE_T}
};

// the plm reserved keywords - format
// name (pstr), keywordId (byte) see intermediate tokens in plm.h
// in the symbols area the infoP value is set to 0xff00 + the keywordId
static struct {
    pstr_t *name;
    byte id;
} keywords[] = {
    {(pstr_t *)("\x7""ADDRESS"),  L_ADDRESS},
    {(pstr_t *)("\x3""AND"),      L_AND},
    {(pstr_t *)("\x2""AT"),       L_AT},
    {(pstr_t *)("\x5""BASED"),    L_BASED},
    {(pstr_t *)("\x2""BY"),       L_BY},
    {(pstr_t *)("\x4""BYTE"),     L_BYTE},
    {(pstr_t *)("\x4""CALL"),     L_CALL},
    {(pstr_t *)("\x4""CASE"),     L_CASE},
    {(pstr_t *)("\x4""DATA"),     L_DATA},
    {(pstr_t *)("\x7""DECLARE"),  L_DECLARE},
    {(pstr_t *)("\x7""DISABLE"),  L_DISABLE},
    {(pstr_t *)("\x2""DO"),       L_DO},
    {(pstr_t *)("\x4""ELSE"),     L_ELSE},
    {(pstr_t *)("\x6""ENABLE"),   L_ENABLE},
    {(pstr_t *)("\x3""END"),      L_END},
    {(pstr_t *)("\x3""EOF"),      L_EOF},
    {(pstr_t *)("\x8""EXTERNAL"), L_EXTERNAL},
    {(pstr_t *)("\x2GO"),         L_GO},
    {(pstr_t *)("\x4GOTO"),       L_GOTO},
    {(pstr_t *)("\x4HALT"),       L_HALT},
    {(pstr_t *)("\x2IF"),         L_IF},
    {(pstr_t *)("\x7INITIAL"),    L_INITIAL},
    {(pstr_t *)("\x9INTERRUPT"),  L_INTERRUPT},
    {(pstr_t *)("\x5LABEL"),      L_LABEL},
    {(pstr_t *)("\x9LITERALLY"),  L_LITERALLY},
    {(pstr_t *)("\x5MINUS"),      L_MINUS},
    {(pstr_t *)("\x3MOD"),        L_MOD},
    {(pstr_t *)("\x3NOT"),        L_NOT},
    {(pstr_t *)("\x2OR"),         L_OR},
    {(pstr_t *)("\x4PLUS"),       L_PLUS},
    {(pstr_t *)("\x9PROCEDURE"),  L_PROCEDURE},
    {(pstr_t *)("\x6PUBLIC"),     L_PUBLIC},
    {(pstr_t *)("\x9REENTRANT"),  L_REENTRANT},
    {(pstr_t *)("\x6RETURN"),     L_RETURN},
    {(pstr_t *)("\x9STRUCTURE"),  L_STRUCTURE},
    {(pstr_t *)("\x4THEN"),       L_THEN},
    {(pstr_t *)("\x2TO"),         L_TO},
    {(pstr_t *)("\x5WHILE"),      L_WHILE},
    {(pstr_t *)("\x3XOR"),        L_XOR}};

// clang-format on
static void InitFilesAndDefaults() {
    LEFTMARGIN      = 1;
    char const *src = srcFileTable[0].fNam;
    char *s         = (char *)basename(src);
    char *t         = strrchr(s, '.');
    if (!t || t == s)
        t = strchr(s, '\0');
    int prefixLen = (int)(t - s);
    bool uc       = false;
    // if uppercase seen and no lowercase then add upper case ext, else lower case
    for (t = s; *t && !islower(*t); t++)
        if (isupper(*t))
            uc = true; // at least one uc seen
    if (*t)
        uc = false;
    ixiFileName = safeMalloc(prefixLen + 5); // allow for .xxx\0
    memcpy(ixiFileName, s, prefixLen);
    strcpy(ixiFileName + prefixLen, uc ? ".IXI" : ".ixi");

    lstFileName = safeMalloc(prefixLen + 5); // allow for .xxx\0
    memcpy(lstFileName, s, prefixLen);
    strcpy(lstFileName + prefixLen, uc ? ".LST" : ".lst");

    objFileName = safeMalloc(prefixLen + 5); // allow for .xxx\0
    memcpy(objFileName, s, prefixLen);
    strcpy(objFileName + prefixLen, uc ? ".OBJ" : ".obj");

    depFileName = safeMalloc(prefixLen + 9); // .deps/ {prefix} .d\0;
    strcpy(depFileName, ".deps/");
    memcpy(depFileName + 6, s, prefixLen);
    strcpy(depFileName + 6 + prefixLen, ".d");

    IXREF    = false;

    PRINT    = true;

    XREF     = false;
    SYMBOLS  = false;
    DEBUG    = false;
    PAGING   = true;
    OBJECT   = true;
    DEPEND   = false;

    OPTIMIZE = true;

    SetMarkerInfo(20, 21);
    SetPageNo(0);
} /* InitFilesAndDefaults() */

void SignOnAndGetSourceName() {
    puts("\nPL/M-80 COMPILER " VERSION);
    GetNonSpaceToken();                        // skip invoke name
    srcFileTable[0].fNam = GetNonSpaceToken(); // get src file
    if (!*srcFileTable[0].fNam)
        FatalCmdLineErr("No file to compile");
    cmdTextP    = cmdP;
    moreCmdLine = *cmdTextP != '\n'; // continuation lines end in \r, end of all is \n
    InitFilesAndDefaults();
} /* SignOnAndGetSourceName() */

static void InstallBuiltins() {
    for (uint32_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        Lookup(builtins[i].name);
        CreateInfo(0, BUILTIN_T, curSym);
        info->builtinId  = builtins[i].id;
        info->paramCnt   = builtins[i].paramCnt;   // modified to be consistent with procedures
        info->returnType = builtins[i].returnType; // modified to be consistent with procedures
    }
    Lookup((pstr_t *)"\x6MEMORY");
    CreateInfo(0, BYTE_T, curSym);
    info->flag |= F_LABEL | F_MEMORY | F_ARRAY;
} /* InstallBuiltins() */

static void InstallKeywords() {
    for (uint32_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        Lookup(keywords[i].name);
        curSym->infoChain = infotab + MAXINFO + keywords[i].id;
    }
} /* InstallKeywords() */

static void InitInfoAndSym() {
    SetPageNo(1);
    localLabelCnt = 0;
    cmdLineCaptured++;
    scopeChains[0] = scopeChains[1] = scopeSP = 0;
} /* InitInfoAndSym() */

void InitKeywordsAndBuiltins() {
    InitInfoAndSym();
    InstallKeywords();
    InstallBuiltins();
} /* InitKeywordsAndBuiltins() */

void SetPageNo(word v) {
    pageNo = v - 1;
}