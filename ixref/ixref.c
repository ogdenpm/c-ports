/****************************************************************************
 *  ixref.c: part of the C port of Intel's ISIS-II ixref                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "ixref.h"
#include "cmdline.h"
#include "os.h"
#include <stdarg.h>
#include <string.h>

#define VERSION "V1.3"

char const help[]        = "usage: %s ixifiles [,ixifile]* options*\n"
                           "Where options are:\n"
                           "TITLE(text)     title to show on listings. text can be quoted\n"
                           "PRINT(file)     save listing to file rather than show on the console\n"
                           "PUBLICS         include Public symbols only, cannot be used with EXTERNALS\n"
                           "EXTERNALS       include external symbols only, cannot be used with PUBLICS\n"
                           "PAGEWIDTH(val)  set page width to val (60-132) rather than 120\n"
                           "\n"
                           "Spaces can be used instead of commas to separate ixi file names\n"
                           "also * and  ? are supported for multiple file selection\n"
                           "File names are of the format [:Fx:]path, where x is a digit and path\n"
                           "The :Fx: maps to a directory prefix from the ISIS_Fx environment variable\n";

char *insufficientMemory = " ,  NOT ENOUGH MEMORY\n";

char *outFileName;
char *tmpModnm = "modnm.tmp ";
char *tmpIxout = "ixout.tmp";
char *tmpIxin  = "ixin.tmp";
FILE *tmpModnmFp;
record_t modRecord;
record_t tmpInRec;
record_t tmpOutRec;
record_t inRec;

uint16_t entry;
FILE *inFp;
FILE *tmpInFp;

FILE *outFp;
FILE *tmpOutFp;
bool showPublics   = true;
bool showExternals = true;
bool haveTitle;
bool havePageWidth;
uint8_t outLineLen = 120;
bool havePrint;
char *endFiles;
char *startFiles;
uint8_t outCol   = 1;
uint8_t maxIdLen = 11;
uint8_t outLinesLeft;
uint8_t outPageLen = 60;
uint8_t pageNumber;
char *title;
uint8_t titleLen;

void ExpectLP(void) {
    ExpectChar('(', "left parenthesis expected"); /* left parenthesis expected */
}

void ExpectRP(void) {
    ExpectChar(')', "right parenthesis expected"); /* right parenthesis expected */
}

static void OutNewPage() {
    fputc('\f', outFp);
    int width = fprintf(outFp, "PL/M IXREF         ");
    if (haveTitle)
        width += fprintf(outFp, "%s", title);
    fprintf(outFp, "%*s%4d\n\n\n", outLineLen - width - 7, "PAGE", ++pageNumber);
    outLinesLeft = outPageLen - 3;
    outCol       = 1;
}

void OutNewLine() {
    putc('\n', outFp);
    fflush(outFp);
    outCol = 1;
    if (--outLinesLeft == 0)
        OutNewPage();
}

void OutNNewLines(int n) {
    while (n-- != 0)
        OutNewLine();
}

void OutPstr(char const *str, int len) {
    if (outCol + len > outLineLen) {
        OutNewLine();
        outCol += fprintf(outFp, "%*s", maxIdLen + 7, "");
    }
    fwrite(str, 1, len, outFp);
    outCol += len;
}

void OutStr(char const *s) {
    char const *t;
    while ((t = strchr(s, '\n'))) {
        OutPstr(s, (int)(t - s));
        OutNewLine();
        s = t + 1;
    }
    if (*s)
        OutPstr(s, (int)strlen(s));
}

void OutPrintf(char const *fmt, ...) {
    va_list args;
    char line[1024];
    va_start(args, fmt);
    vsprintf(line, fmt, args);
    va_end(args);
    OutStr(line);
}

void InitPrint() {
    if (!(outFp = Fopen(outFileName, "wt")))
        IoError(outFileName, "Could not create");
    OutNewPage();
    fputs("PL/M IXREF, " VERSION "\nINVOKED BY:\n", outFp);
    outLinesLeft -= 2;
    outLinesLeft -= printCmdLine(outFp, outLineLen, 0);
    OutNewLine();
}

uint16_t ParseLPNumRP(void) {
    ExpectLP();
    uint16_t num = GetNumber();
    ExpectRP();
    return num;
}

static void ParsePageWidth() {
    ExpectLP();
    uint16_t pageWidth = GetNumber();
    ExpectRP();
    havePageWidth = true;
    if (pageWidth < 60 || pageWidth > 132) {
        fprintf(stderr, "Warning: Page width outside range 60-132, using 120\n");
        pageWidth = 120;
    }
    outLineLen = (uint8_t)pageWidth;
}

static void ParseTitle() {
    ExpectLP();
    title = GetText();
    if (!*title)
        FatalCmdLineErr("Missing text for TITLE control");
    ExpectRP();
    titleLen = (uint8_t)strlen(title);
    if (titleLen > 60)
        title[titleLen = 60] = '\0';
    haveTitle = true;
}

static void ParsePrint() {
    ExpectLP();
    outFileName = GetText();
    if (!*outFileName)
        FatalCmdLineErr("Missing listing file for PRINT control");
    ExpectRP();
    havePrint = true;
}

char *reserved[] = { "", "TITLE", "PRINT", "PUBLICS", "EXTERNALS", "PAGEWIDTH" };

bool morefiles() {
    if (*cmdP == ',') {
        cmdP++;
        return true;
    }
    char *ntoken = PeekToken();
    for (uint32_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++)
        if (stricmp(reserved[i], ntoken) == 0)
            return false;
    return true;
}

static void ParseCmdLine() {
    GetToken(); // skip invoke name
    startFiles = cmdP;
    do {
        GetToken();
        endFiles = cmdP;
    } while (morefiles());
    endFiles--;

    char const *token;
    while (*(token = GetToken())) {
        if (stricmp(token, "TITLE") == 0) {
            if (haveTitle)
                FatalCmdLineErr("TITLE specified more than once");
            else
                ParseTitle();
        } else if (stricmp(token, "PRINT") == 0) {
            if (havePrint)
                FatalCmdLineErr("PRINT specified more than once");
            else
                ParsePrint();
        } else if (stricmp(token, "PUBLICS") == 0) {
            if (!(showPublics && showExternals))
                FatalCmdLineErr("Invalid use of PUBLICS");
            else
                showExternals = 0;
        } else if (stricmp(token, "EXTERNALS") == 0) {
            if (!(showPublics && showExternals))
                FatalCmdLineErr("Invalid use of EXTERNALS");
            else
                showPublics = 0;
        } else if (stricmp(token, "PAGEWIDTH") == 0) {
            if (havePageWidth)
                FatalCmdLineErr("PAGEWIDTH specified more than once");
            else
                ParsePageWidth();
        } else
            FatalCmdLineErr("Unknown control");
    }
}

void Start() {
    puts("PL/M IXREF " VERSION);
    ParseCmdLine();
    if (havePrint)
        InitPrint();
    collectRecords();
    sub5317();
    Exit(0);
}

