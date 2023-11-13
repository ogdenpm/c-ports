/****************************************************************************
 *  cmdline.c: part of the C port of Intel's ISIS-II 8085 toolchain         *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "../shared/cmdline.h"
#include "../shared/os.h"

#define CLCHUNK 256 // size increase as command line grows

#define STDIN   0
#define STDOUT  1

char *cmdP; // current location on command line

char *endToken; // used in error reporting

// command line routines
static char *commandLine; // users command line
static int cmdLineLen;    // current command line length
static int cmdLineSize;   // space allocate for the command line
static char *tokenLine;   // copy of the line, modified to create C string tokens
// returns true if more appends to the command line are needed

static bool appendCmdLine(char const *s) {
    int c;
    int len = (int)strlen(s);

    while (cmdLineLen + len + 3 > cmdLineSize) // allow for #\n\0 for error reporting
        commandLine = xrealloc(commandLine, cmdLineSize += CLCHUNK);
    while ((c = *s++)) {
        if (c != '\n')
            commandLine[cmdLineLen++] = c >= ' ' ? c : ' '; // control char -> space
        else {
            while (commandLine[cmdLineLen - 1] == ' ') // trim trailing spaces
                cmdLineLen--;

            if (commandLine[cmdLineLen - 1] == '&') {
                commandLine[cmdLineLen - 1] = '\r'; // replace continuation with \r
                return true;
            } else {
                while (commandLine[cmdLineLen - 1] == '\r') // trim any trailing continuations
                    cmdLineLen--;
                commandLine[cmdLineLen++] = '\n';
                return false; // all done
            }
        }
    }
    return true; // not reached end of line
}

// get any command line extension and a shadow copy for token construction
static char *getLine(char *line) {
    char buf[256];

    while (appendCmdLine(line)) {
        if (strchr(line, '\n') && ttyin && ttyout) // need prompt for new line ?
            fputs("**", stdout);
        if (!fgets(buf, 256, stdin)) {
            if (cmdLineLen > 1)        // more than just the prompt
                appendCmdLine("\n\n"); // finish off line even if previous had &
            break;
        }
        line = buf;
    }
    commandLine[cmdLineLen] = '\0';
    free(tokenLine);
    tokenLine = xstrdup(commandLine);
    return commandLine;
}

char *getInteractiveLine() {
    cmdLineLen = 0;
    if (ttyin && ttyout)
        putchar('*');
    return getLine("*");
}

char *getCmdLine(int argc, char **argv) {
    cmdLineLen = 0;
    appendCmdLine(argc == 1 ? "*" : "-");
    appendCmdLine(invokeName);
    if (argc == 1)
        appendCmdLine("&");
    else {
        for (int i = 1; i < argc; i++) {
            appendCmdLine(" ");
            appendCmdLine(argv[i]);
        }
    }
    return getLine("\n");
}

// public command line handling functions
void SkipWs() {
    while (*cmdP == ' ' || *cmdP == '\r')
        cmdP++;
}

void ExpectChar(uint8_t ch, char const *msg) {
    SkipWs();
    if (ch == *cmdP) {
        endToken = ++cmdP;
        SkipWs();
    } else
        FatalCmdLineErr(msg);
}

static char *ScanToken(char const *delims) {
    SkipWs();
    if (*cmdP == '\'') { // quoted token
        cmdP++;
        delims = "'\n\r";
    }
    char *token = tokenLine + (cmdP - commandLine);
    char *p     = strpbrk(cmdP, delims);
    if (!p)
        p = strchr(cmdP, '\0');
    if (*p != *delims) {
        if (*delims == '\'')
            FatalCmdLineErr("Missing terminating quote");
        else if (*delims == ')')
            FatalCmdLineErr("Missing terminating )");
    }
    if (*p == '\'')
        cmdP = p + 1;
    else if (*delims != '\'') { // trim unless started with '
        while (p > cmdP && p[-1] == ' ')
            p--;
        cmdP = p;
    }
    tokenLine[p - commandLine] = '\0';
    endToken                   = p;
    SkipWs();
    return token;
}

void SetEndToken(char *p) {
    endToken = p;
}

static char *peekedToken;
/* use a shadow copy of commandLine to create '\0' terminated tokens */
char *GetToken(void) {
    if (peekedToken) {
        char *tmp   = peekedToken;
        peekedToken = NULL;
        return tmp;
    }
    return ScanToken(" ,()\n\r");
}

char *PeekToken(void) {
    return peekedToken = GetToken();
}

char *GetText(void) {
    return ScanToken(")\n\r");
}

char *GetNonSpaceToken(void) {
    return ScanToken(" \n\r");
}

uint16_t GetNumber(void) {
    char const *pch;
    uint8_t radix, digit;
    uint32_t num = 0;
    char *token  = GetToken();

    for (pch = token; isxdigit(*pch); pch++)
        ;
    char suffix = toupper(*pch);
    if (suffix == 'H')
        radix = 16;
    else if (suffix == 'O' || suffix == 'Q')
        radix = 8;
    else if (*pch != '\0')
        num = 0x10000;
    else if (toupper(pch[-1]) == 'B' || toupper(pch[-1]) == 'D') {
        if (--pch == token)
            num = 0x10000;
        else
            radix = toupper(pch[-1]) == 'B' ? 2 : 10;
    } else
        radix = 10;

    for (; num < 0x10000 && token < pch; token++) {
        digit = isdigit(*token) ? *token - '0' : toupper(*token) - 'A' + 10;
        if (digit >= radix)
            num = 0x10000;
        else
            num = num * radix + digit;
    }
    if (num >= 0x10000)
        FatalCmdLineErr("Invalid number");
    return (uint16_t)num;
}

// print the command line, splitting long lines
int printCmdLine(FILE *fp, int width, int offset) {
    int col   = offset;
    char *s   = offset ? commandLine + 1 : commandLine;
    int nlCnt = 0;
    while (*s) {
        char *brk = strpbrk(s, ", '\n\r");
        if (*brk == '\'')
            brk = strpbrk(brk + 1, "'\n\r"); // for quoted token include it
        if (!brk)
            brk = strchr(s, '\0'); // if no break do whole string
        int len = (int)(brk - s);
        if (col + len >= width - 1) { // if string doesn't fit with \ or & at end
            col = 2;                  // indent on new line
            fputs("\\\n  ", fp);
            nlCnt++;
        }
        col += fprintf(fp, "%.*s", len, s);
        if (!*brk || *brk == '\n')
            break;
        if (*brk == '\r') {
            fprintf(fp, "&\n%*s**", offset, "");
            col = 2 + offset;
            nlCnt++;
        } else
            fputc(*brk, fp);
        s = brk + 1;
    }
    if (col) {
        fputc('\n', fp);
        nlCnt++;
    }
    return nlCnt;
}

// common error handlers
_Noreturn void FatalCmdLineErr(char const *errMsg) {
    strcpy(endToken, "#\n");
    fprintf(stderr, "Command line error near #: %s\n", errMsg);
    printCmdLine(stderr, 120, 0);
    Exit(1);
}