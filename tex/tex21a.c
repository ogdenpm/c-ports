#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <conio.h>
#include <ctype.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void showVersion(FILE *fp, bool full);

typedef uint8_t byte;
typedef uint16_t word;
typedef uint16_t address;

#define CR     '\r'
#define FF     '\f'
#define DC1    0x11
#define US     0x1f
#define ESC    0x1b
#define BELL   '\a'
#define CPMEOF 0x1a

enum { SRCTEX, SRCNESTED, SRCUSERINPUT = 3, SRCCMDLINE };
enum { NOSPC, JUSTIFYSPC, PUNCTSPC, ESCSPACE = 5, ESCHYPHEN };

#ifdef _WIN32
#define isDirSep(c) (c == '/' || c == '\\')
#else
#define isDirSep(c) (c == '/')
#endif

char copyright[] = " Copyright(c) 1980 Digital Research ";
#define VERSION   "TEX 2.1"
#define buildDate "02/13/81"

/*
declare aFileNotFound(*) byte data('File ! found$');
declare aUndefined(*) byte data('Undefined    Command$');
declare	(cmdChr1, cmdChr2) byte at(.aUndefined + 10);

declare aAbortYN(*) byte data(CR, 'Abort (Y/N) ?$');
*/
const byte breakChar[] = "\n\t ?*=:!=|_[]";

byte ii;
byte jj;
byte tabstops[]         = { 9,  17, 25,  33,  41,  49,  57,  65,  73,  81,
                    89, 97, 105, 113, 121, 129, 137, 145, 153, 255 };

char undefinedCommand[] = "Undefined    Command";
#define cmdChr1 undefinedCommand[10]
#define cmdChr2 undefinedCommand[11]

typedef struct {
    FILE *fp;
    char name[_MAX_PATH];
    char context[256];
    char *path;
    uint8_t cidx;
} FCB;

FCB defFCB;
FCB prnFCB;
FCB ixFCB;
FCB texFCB;

byte getCmdLine();
byte getcCon();
void putPrn(char c);
void putCon(char c);
void nullSub(char c);
byte getTex();
byte getRsp();
byte getDef();

byte tline, wline, eline, nline, sline;
byte lineOutBuf[256];
byte cmdLineIdx       = 0;

address prnBufIdx     = 0;
address texBufBase    = 0;
address fileBuf       = 0;
address defBufBase    = 0;
byte srcFileOpt       = SRCCMDLINE;
byte prevSrcFileOpt   = SRCTEX;
byte (*cgetc)()       = getCmdLine;
byte inputChar        = 0;
byte altSrcOpt        = 'X';
byte (*altGetc)()     = getcCon;
byte prevAltInputChar = ' ';
byte stdinIdx         = 0;
byte prnToDev         = 0;
void (*cputc)(char c) = putPrn;
void (*dstPutc)(char c);
byte leftMargin         = 8;
byte width              = 0;
byte indentThisLine     = 0;
bool marginsChanged     = false;
byte widthThisLine      = 70;
byte centreWidth        = 0;
byte lineLen            = 70;
byte pageOffset         = 8;
byte ppHangingIndent    = 6;
byte minParaLinesOnPage = 2;
byte curIndent          = 0;
byte tmpIndent          = 0;
byte leftHangingIndent  = 0;
byte escIdx             = 0;
byte nEsc               = 1;
byte escCodes[128];
byte escColumn[128];
bool slackFromRight       = true;
byte pageLen              = 66;
byte curLine              = 0;
byte topMargin            = 6;
byte headerMargin         = 3;
byte bottomMargin         = 5;
byte footerMargin         = 1;
byte lineSpacing          = 1;
address pageNum           = 1;
address firstPage         = 0;
address lastPage          = 0xFFFF;
byte formatOpt            = PUNCTSPC;
bool atBreakLine          = true;
bool autoJustify          = true;
bool ignoreRestOfLine     = true;
bool invalidParam         = false;
bool isJustified          = true;
bool printerIsJustifying  = false;
bool charSpacingOverriden = false;
byte charSpacing          = 5;
byte proportionalSpace    = 0;
byte ulCnt                = 0;
byte underscoreCnt        = 0;
byte boCnt                = 0;
byte boldCnt              = 0;
bool printerHMISet        = false;
bool ixInitialised        = false;
bool doIX                 = true;
byte centreLinesCnt       = 0;
byte curCol               = 1;
bool wsSpecialSeen        = false;
bool continueLine         = false;
byte pageNumMarker        = '%';
byte escapeCharacter      = '\\';
byte cmdLeadIn            = '.';
byte litLeadIn            = '`';
bool OOpt                 = false;
bool errPrint             = false;
bool FOpt                 = false;
bool manualFeed           = false;
bool isPaging             = false;
bool DOpt                 = false;
bool COpt                 = false;
bool doPageNum            = true;
bool noUserPageNum        = true;
byte signPrefix           = 0;
byte footerLine[133];
byte headerLine[133];
byte inBuf[128];

// cmd line variables
int _argc;
char **_argv;
int arg = 1; // current arg being processed
int argIdx;  // index into current arg string

char *cmdLine;

void waitKey() {
    fputs("Hit any key to continue:", stdout);
    getch();
    putchar('\n');
}

void wrapup() {
    if (prnToDev == 0) {
        fclose(prnFCB.fp);
        unlink(prnFCB.name);
    }
    if (ixInitialised) {
        fclose(ixFCB.fp);
        unlink(ixFCB.name);
    }
    fputs("\nTex Aborted\n", stderr);
    exit(1);
}

void error(char *msg) {
    FCB *fcb = NULL;
    uint8_t i;
    fprintf(stderr, "\nERROR: %s\n\n", msg);
    if (srcFileOpt == SRCUSERINPUT)
        return;

    fputs("CONTEXT:\n", stderr);
    if (srcFileOpt == SRCCMDLINE) {
        for (int i = 1; i < arg; i++)
            fprintf(stderr, "%s ", _argv[arg]);
        for (int i = 0; i < argIdx; i++)
            fputc(_argv[arg][i], stderr);
        fputc('?', stderr);
    } else if (srcFileOpt == SRCTEX)
        fcb = &texFCB;
    else if (srcFileOpt == SRCNESTED)
        fcb = &defFCB;

    if (fcb) {
        if (fcb->context[fcb->cidx] == 0) // check if ! filled cyclic buffer
            i = 0xff;
        else
            for (i = fcb->cidx; fcb->context[i] != '\n' && ++i != fcb->cidx;)
                ;
        while (++i != fcb->cidx)
            fputc(fcb->context[i], stderr);
    }
    fputc('\n', stderr);
}

void fatalError(char *msg) {
    error(msg);
    wrapup();
}

void putDstEscCode(char c) {
    dstPutc(ESC);
    dstPutc(c);
}

void enablePS() {
    putDstEscCode('S');
    putDstEscCode('P');
}

void disablePS(char n) {
    putDstEscCode('Q');
    dstPutc(US);
    dstPutc(n);
}

void absoluteHT(byte n) {
    if (proportionalSpace)
        disablePS(CR);
    putDstEscCode(9); /* initialise absolute HT to print position n */
    dstPutc(n + 1);
    if (proportionalSpace)
        enablePS();
}

void setMargins() {
    byte rightMargin;

    if (marginsChanged && DOpt) {
        rightMargin = leftMargin + width;
        if (!proportionalSpace)
            rightMargin--;
        marginsChanged = false;
        absoluteHT(leftMargin);
        putDstEscCode('9'); /* set left margin */
        absoluteHT(rightMargin);
        putDstEscCode('0'); /* set right margin */
        dstPutc(CR);
    }
}

void createFile(FCB *fcb, bool isIx) {
    if ((fcb->fp = fopen(fcb->name, isIx ? "wb" : "wt")) == NULL)
        fatalError(isIx ? "Can't create ix file" : "Can't create prn file");
}

byte getCmdLine() {
    if (arg < _argc && _argv[arg][argIdx]) {
        inputChar = _argv[arg][argIdx++];
        if (argIdx == 1 && inputChar == '-') // map leading - option to $ for common code
            inputChar = '$';
    } else if (arg == _argc || ++arg == _argc)
        inputChar = '\n';
    else {
        argIdx    = 0;
        inputChar = ' '; // simulate space between args
    }
    return inputChar;
}

char *getCmdParameter() {
    if (arg < _argc) {
        argIdx = strlen(_argv[arg]);
        return _argv[arg] + (argIdx == 0 ? 0 : 2);
    } else
        return NULL;
}

bool pageInRange() {
    return pageNum >= firstPage && pageNum <= lastPage;
}

void selectDst() {
    if (prnToDev == 0)
        cputc = putPrn;
    else if (prnToDev == 'X')
        cputc = putCon;
    //   else if (prnToDev == 'Y')
    //       cputc = wrlst;
    else
        cputc = nullSub;

    if (OOpt)
        if (!pageInRange())
            cputc = nullSub;
        else if (DOpt) {
            marginsChanged = true;
            if (proportionalSpace)
                enablePS();
        }
}

void selectSrc(byte n) {
    prevSrcFileOpt = srcFileOpt;
    if ((srcFileOpt = n) == SRCTEX)
        cgetc = getTex;
    else if (srcFileOpt == SRCNESTED)
        cgetc = getDef;
    else if (srcFileOpt == SRCCMDLINE)
        cgetc = getCmdLine;
    else
        cgetc = getRsp;
}

void selectSrcAndGetc(byte n) {
    selectSrc(n);
    cgetc();
}

byte getUCChar() {
    return inputChar = toupper(cgetc());
}

char *fname(char *name) {
    char *s;
    if (s = strrchr(name, '/'))
        name = s + 1;
#ifdef _WIN32
    if (s = strrchr(name, '\\'))
        name = s + 1;
#endif
    return name;
}

char *cpypath(char *name) {
    char *s    = fname(name);
    size_t len = s - name;
    char *path = malloc(len + 1);
    memcpy(path, name, len);
    path[len] = 0;
    return path;
}

bool initFCB(FCB *fcb) {
    int nameIdx  = 0;
    bool hasExt  = false;
    bool hasName = false;

    fcb->name[0] = '\0';
    if (fcb->path) {
        strcpy(fcb->name, fcb->path);
        nameIdx = strlen(fcb->name);
        if (nameIdx && !isDirSep(fcb->name[nameIdx - 1])) {
            fcb->name[nameIdx++] = '/';
            fcb->name[nameIdx]   = 0;
        }
    }
    cgetc(); // get char
    if (inputChar != '\n') {
        while (inputChar == ' ')
            cgetc();
        if (inputChar != '$') {
            while (!strchr(breakChar, inputChar) && nameIdx < _MAX_PATH - 1) {
                fcb->name[nameIdx++] = inputChar;
                if (inputChar == '.' && hasName)
                    hasExt = true;
                else if (!isDirSep(inputChar))
                    hasName = true;
                else
                    hasExt = hasName = false;
                cgetc();
            }
            fcb->name[nameIdx] = 0;
            if (!hasName) {
                error("Invalid File");
                return false;
            }
            if (!hasExt && strlen(fcb->name) < _MAX_PATH - 4)
                strcat(fcb->name, ".tex");
        }
    }
    return nameIdx != 0;
}

void putCon(char c) {
    putchar(c);
}

void putPrn(char c) {
    putc(c, prnFCB.fp);
}

void nullSub(char c) {
}

void putTextCh(char c) {
    ;
    byte n, tabCol;

    if (c == ESCSPACE)
        c = ' ';
    else if (c == ESCHYPHEN)
        c = '-';
    if (c >= ' ') {
        dstPutc(c);
        curCol++;
    } else if (c == '\t') {
        n = 0xff;
        while (curCol >= tabstops[++n])
            ;
        tabCol = tabstops[n];
        if (DOpt) {
            absoluteHT(tabCol + leftMargin - 1); /* patch 2 */
            curCol = tabCol;
        } else
            while (tabCol > curCol) {
                dstPutc(' ');
                curCol++;
            }
    } else
        dstPutc(c);
}

void leadingSpaceN(int n) {
    if (proportionalSpace) {
        curCol = n;
        absoluteHT(curCol + leftMargin);
    } else
        while (n-- > 0)
            cputc(' ');
}

void newLineN(int n) {
    while (n-- > 0) {
        cputc('\n');
        curLine++;
        if (isPaging && curLine % 22 == 0)
            getch();
        if (kbhit()) {
            getch();
            fputs("\nAbort (Y/N) ?", stdout);
            if (toupper(getch()) == 'Y')
                wrapup();
            putchar('\n');
        }
    }
}

void screenNewLineN(int cnt) {
    int n;

    while (--cnt >= 0) {
        n = widthThisLine;
        while (--n >= 0)
            cputc(' ');
        newLineN(1);
    }
}

bool roomOnPage(byte n) {
    return curLine + n <= pageLen - bottomMargin;
}

void putcLineOutChar() {
    cputc(lineOutBuf[sline++]);
}

void emitCharItem() {
    byte ch;

    while (escColumn[nEsc] == sline && nEsc <= escIdx) {
        dstPutc(ch = escCodes[nEsc]);
        if (ch == 'E') {
            underscoreCnt = ulCnt;
            ulCnt         = 0;
        } else if (ch == 'O') {
            boldCnt = boCnt;
            boCnt   = 0;
        } else if (ch == 'R')
            underscoreCnt = 0;
        else if (ch == '&')
            boldCnt = 0;
        nEsc++;
    }
    putcLineOutChar();
}

void spacingText(byte n) {
    void (*putLineChHandler)();

    if (DOpt) {
        putLineChHandler = putcLineOutChar;
        if (escIdx != 0)
            putLineChHandler = emitCharItem;
        if (boldCnt > 0)
            putDstEscCode('O'); /* set bold print */
        if (leftHangingIndent != 0) {
            if (underscoreCnt > 0)
                putDstEscCode('E'); /* auto underscore */
            while (leftHangingIndent-- != 0) {
                if (sline != n)
                    putLineChHandler();
            }
            if (underscoreCnt > 0)
                putDstEscCode('R'); /* auto underscore off */
        }
        if (printerIsJustifying)
            putDstEscCode('M'); /* auto justify */
        if (underscoreCnt > 0)
            putDstEscCode('E'); /* auto underscore */
        while (sline != n) {
            putLineChHandler();
        }
        if (underscoreCnt > 0) {
            underscoreCnt--;
            putDstEscCode('R'); /* auto underscore off */
        }

        if (boldCnt > 0)
            boldCnt--;
        if (printerHMISet) {
            if (eline == nline)
                printerHMISet = false;
            putDstEscCode('S'); /* return HMI to spacing switch */
        }
        if (nEsc > escIdx) {
            nEsc   = 1;
            escIdx = 0;
        }
    } else {
        nEsc   = 1; /* fix to clear escapes that are ! used */
        escIdx = 0;
        while (sline != n) {
            cputc(lineOutBuf[sline++]);
        }
    }
    tmpIndent         = 0;
    leftHangingIndent = 0;
}

void centre(byte n) {
    int extra = (n - (byte)(nline - sline)) / 2;
    if (DOpt)
        putDstEscCode('='); /* auto centre */
    else 
        leadingSpaceN(leftMargin + (extra >= 0 ? extra : 0));
    spacingText(nline);
}

void emitPageNum(bool centreNum) {

    uint16_t num = pageNum;
    tline        = nline;
    nline        = sline;

    while (num != 0) {
        lineOutBuf[--sline] = num % 10 + '0';
        num                 = num / 10;
    }
    if (centreNum) {
        leftMargin = pageOffset;
        centre(centreWidth);
    } else
        spacingText(nline);
    nline         = tline;
    noUserPageNum = false;
}

void emitHeadingOrFooter(char *buf, byte n) {
    int c;

    if (*buf) {
        newLineN(n);
        if (!DOpt)
            leadingSpaceN(leftMargin);
        else if (proportionalSpace)
            putDstEscCode('M'); /* auto justify */
        while ((c = *buf++)) {
            if (c == pageNumMarker)
                emitPageNum(false);
            else
                cputc(c);
        }
    }
}

void emitEndOfPage() {
    byte n;

    setMargins();
    if (curLine != 0) {
        if (pageLen - bottomMargin > curLine) {
            n = pageLen - bottomMargin - curLine;
            if (prnToDev == 'X')
                screenNewLineN(n);
            else
                newLineN(n);
        }
        if (bottomMargin != 0) {
            emitHeadingOrFooter(footerLine, footerMargin);
            if (doPageNum)
                if (noUserPageNum) /* if footer hasn't put page # */
                    if ((n = pageLen - curLine) > 1) {
                        newLineN(n / 2);
                        emitPageNum(true); /* put centred page # */
                    }
            if (FOpt)
                cputc(FF);
            else
                newLineN(pageLen - curLine);
        }
        pageNum       = pageNum + 1;
        noUserPageNum = true;
        curLine       = 0;
        if (proportionalSpace)
            putDstEscCode('X'); /* cancel all WP modes */
        if (OOpt)
            selectDst();
        if (manualFeed) {
            fputs("Insert next page, ", stdout);
            waitKey();
        }
    }
}

void emitHeadingIfAtTop() {
    setMargins();
    if (curLine == 0) {
        if (centreWidth == 0)
            centreWidth = width;
        if (topMargin != 0) {
            emitHeadingOrFooter(headerLine, topMargin - headerMargin - 1);
            newLineN(topMargin - curLine);
            if (proportionalSpace)
                putDstEscCode('X'); /* cancel WP modes */
        }
    }
}

void spacing(byte n) {

    emitHeadingIfAtTop();
    n *= lineSpacing;
    if (roomOnPage(n + 1))
        newLineN(n);
    else
        emitEndOfPage();
}

byte slack; // promoted to global
bool notFirstPass;

byte allocateSlack(byte srcIdx, byte srcEndIdx, byte dstIdx, byte step) {
    char c, prevC;

    prevC = ' '; /* don't add extra space at either end of line */
    while (srcIdx != srcEndIdx) {
        c = lineOutBuf[dstIdx += step] = lineOutBuf[srcIdx += step];
        if (c == ' ' && slack != 0)           /* seen a space & have slack*/
            if (prevC != '.' || notFirstPass) /* don't add after a . on first pass */
                if (prevC != ' ') {           /* don't add more than 1 space per run */
                    lineOutBuf[dstIdx += step] = ' ';
                    slack--;
                }
        prevC = lineOutBuf[srcIdx];
    }
    return dstIdx;
}

void moveUpText(byte oldEnd, byte oldStart, byte newEnd) {
    while (oldEnd != oldStart) {
        lineOutBuf[--newEnd] = lineOutBuf[--oldEnd];
    }
}

void justify() {
    byte slackAtStart;

    slack        = widthThisLine - (eline - sline);
    sline        = (tline = sline) + leftHangingIndent;
    notFirstPass = false;
    while (slack != 0) {
        slackAtStart = slack;
        if (slackFromRight) {
            moveUpText(nline, wline, nline + slack);
            wline += slack;
            nline += slack;
            eline += slack;
            sline = allocateSlack(eline - slack, sline, eline, -1);
        } else {
            sline = (tline = sline) - slack;
            eline = allocateSlack(tline - 1, eline - 1, sline - 1, 1) + 1;
        }
        notFirstPass = true;
        if (slackAtStart == slack) { /* couldn't allocate any slack */
            error("Word too long");
            break;
        }
    }

    if (leftHangingIndent > 0) {
        moveUpText(tline + leftHangingIndent, tline, sline);
        sline -= leftHangingIndent;
    }
    slackFromRight = !slackFromRight;
}

void outputLine() {
    dstPutc = cputc;
    emitHeadingIfAtTop();
    if (wsSpecialSeen)
        cputc = putTextCh;
    if (centreLinesCnt > 0) {
        /* patch */
        if (!DOpt)
            leadingSpaceN(indentThisLine);

        centre(widthThisLine);
        centreLinesCnt--;
    } else if (nline != sline) {
        if (!DOpt)
            leadingSpaceN(leftMargin);
        curCol = 1;
        leadingSpaceN(indentThisLine);
        if (formatOpt > JUSTIFYSPC) {
            if (eline != nline) {
                if (eline == sline) {
                    eline = wline = nline;
                    error("Word too long");
                }
                if (isJustified)
                    if (DOpt)
                        printerIsJustifying = true;
                    else
                        justify();
            }
            spacingText(eline);
            if (escIdx != 0)
                while (sline != wline) {
                    lineOutBuf[sline] = 0;
                    emitCharItem();
                }
            else
                sline = wline;
        } else
            spacingText(nline);
    }
    eline = sline;
    spacing(1);
    if (printerIsJustifying) {
        printerIsJustifying = false;
        putDstEscCode('X'); /* cancel WP modes */
    }
    if (wsSpecialSeen) {
        selectDst();
        if (sline == nline)
            wsSpecialSeen = false;
    }
    if (!roomOnPage(1))
        emitEndOfPage();
}

void markEscCode(char c) {
    if (escIdx < 127) {
        escCodes[++escIdx] = c;
        escColumn[escIdx]  = nline;
    }
}

void putEscCode(char c) {
    markEscCode(ESC);
    markEscCode(c);
}

void trimTrailingSpace() {
    while (lineOutBuf[nline - 1] == ' ' && nline != sline)
        nline--;
}

void txtBreak() {
    if (sline != nline && autoJustify) {
        trimTrailingSpace();
        eline = wline  = nline;
        slackFromRight = true;
        outputLine();
    }
    dstPutc = cputc;
}

void finishPage() {
    txtBreak();
    emitEndOfPage();
}

bool openSrc(FCB *fcb) {

    txtBreak();
    if (initFCB(fcb))
        if ((fcb->fp = fopen(fcb->name, "rt")))
            return true;
        else
            error("File not found");
    return false;
}

void nextSrcOrDone() {
    selectSrc(SRCCMDLINE);
    inputChar = ' ';

    while (inputChar != '\n' && inputChar != '$')
        if (openSrc(&texFCB)) {
            selectSrc(SRCTEX);
            return;
        }

    manualFeed = 0;
    emitEndOfPage();
    if (prnToDev == 0)
        fclose(prnFCB.fp);
    if (ixInitialised)
        fclose(ixFCB.fp);
    if (prnToDev != 'X') /* ! to console */
        fputs("\nTex Finished\n", stdout);
    exit(0);
}

char getNext(FCB *fcb) {
    int c = getc(fcb->fp);
    if (c == EOF)
        c = CPMEOF;
    fcb->context[fcb->cidx] = c; /* maintain cyclic context */
    fcb->cidx               = (defFCB.cidx + 1) % 256;
    return c & 0x7f;
}

byte getTex() {

    if ((inputChar = getNext(&texFCB)) == CPMEOF) {
        nextSrcOrDone();
        cgetc();
    }
    return inputChar;
}

byte getDef() {
    if ((inputChar = getNext(&defFCB)) == CPMEOF) {
        /* switch back to console on EOF seen */
        altGetc   = getcCon;
        altSrcOpt = 'X';
        selectSrcAndGetc(SRCTEX);
    }
    return inputChar;
}

byte getcCon() {
    return inputChar = getchar();
}

byte getRsp() {
    /* get character from alternate stream */
    altGetc();
    /* if (we have seen blank line) revert to original stream */
    if (inputChar == '\n' && prevAltInputChar == '\n')
        selectSrcAndGetc(prevSrcFileOpt);
    return prevAltInputChar = inputChar;
}

void skipToEOL() {
    while (inputChar != '\n')
        cgetc();
    ignoreRestOfLine = false;
}

void getcProcessEscape() {
    byte ucInputChar;

    while (1) {
        cgetc();
        while (1) {
            if (inputChar < ' ') {
                if (inputChar == '\n') {
                    atBreakLine = true;
                    skipToEOL();
                    inputChar = ' ';
                    return;
                }
                if (inputChar != '\t') {
                    markEscCode(inputChar);
                    break;
                }
                wsSpecialSeen = true;
            }
            if (inputChar == escapeCharacter) {
                cgetc();
                ucInputChar = toupper(inputChar);
                if (ucInputChar == ' ') {
                    inputChar     = ESCSPACE;
                    wsSpecialSeen = true;
                    return;
                } else if (ucInputChar == '-') {
                    inputChar     = ESCHYPHEN;
                    wsSpecialSeen = true;
                    return;
                } else if (inputChar == '\n')
                    skipToEOL();
                else if (ucInputChar == '"') {
                    inputChar = '\n';
                    continue;
                } else if (ucInputChar == 'C')
                    continueLine = true;
                else if (ucInputChar == 'B') {
                    markEscCode(8);
                    cgetc();
                    markEscCode(inputChar);
                } else if (ucInputChar == 'U')
                    putEscCode('D'); /* negative 1/2 line feed */
                else if (ucInputChar == 'D')
                    putEscCode('U'); /* 1/2 line feed */
                else
                    return;
                break;
            }
            return;
        }
    }
}

void setupThisLine() {
    byte oldVal;

    oldVal = leftMargin;
    if ((leftMargin = pageOffset) != oldVal)
        marginsChanged = true;
    if (formatOpt > NOSPC)
        indentThisLine = curIndent + tmpIndent;
    else
        indentThisLine = 0;
    if (tmpIndent > 132) {
        leftHangingIndent = -tmpIndent; /* when tmpIndent < curIndent */
        slackFromRight    = true;
    }
    oldVal = width;
    if ((width = lineLen) != oldVal)
        marginsChanged = true;
    widthThisLine = width - indentThisLine;
    if (charSpacingOverriden)
        widthThisLine += (widthThisLine * charSpacing / 100);
}

byte collectLine() {
    byte ch;

    while (1) {
        atBreakLine = false;
        getcProcessEscape();
        if (inputChar == cmdLeadIn) {
            atBreakLine = true;
            autoJustify = true;
            return false;
        } else if (inputChar == litLeadIn) {
            atBreakLine = true;
            autoJustify = false;
            return false;
        } else if (inputChar == ' ') {
            autoJustify = true;
            txtBreak();
            setupThisLine();
            if (atBreakLine) {
                spacing(1);
                setupThisLine();
                continue;
            }
        }
        continueLine = false;
        while (!atBreakLine) {
            lineOutBuf[nline++] = inputChar;
            getcProcessEscape();
        }
        if (!continueLine)
            break;
    };

    if (centreLinesCnt == 0) {
        trimTrailingSpace();
        if (formatOpt == PUNCTSPC) /* add 2 spaces after . ? or ! */
        {
            if ((ch = lineOutBuf[nline - 1]) == '.' || ch == '?' || ch == '!')
                lineOutBuf[nline++] = ' ';
            lineOutBuf[nline++] = ' ';
        }
    }
    return true;
}

byte collectTextBlock() {
    if (eline == sline)
        setupThisLine();
    /* if ! formated or is centred just collect one line */
    if (formatOpt < PUNCTSPC || centreLinesCnt > 0)
        return collectLine();
    /* otherwise collect as many lines as fit in a single output line */
    while (widthThisLine >= (byte)(nline - sline)) {
        if (!collectLine())
            return false;
    }
    /* back off text if too much collected */
    eline = sline + widthThisLine;
    /* look for a space or hyphen */
    while (lineOutBuf[eline] != ' ') {
        if (lineOutBuf[--eline] == '-') {
            wline = ++eline;
            return true;
        }
    }
    wline = eline + 1;
    if (lineOutBuf[eline - 1] == ' ')
        eline--;
    if (lineOutBuf[wline] == ' ' && wline != nline)
        wline++;
    return true;
}

void undefinedCommandError() {
    error(undefinedCommand);
}

int16_t getNumber() {
    int16_t val;

    signPrefix = 0;
    val        = 0;
    while (getUCChar() == ' ')
        ;
    if (isdigit(inputChar))
        signPrefix = 1;
    else if (inputChar == '+') {
        signPrefix = 2;
        getUCChar();
    } else if (inputChar == '-') {
        signPrefix = 3;
        getUCChar();
    }
    while (isdigit(inputChar)) {
        val = val * 10 + (inputChar - '0');
        getUCChar();
    }
    return val;
}

uint16_t get16NewValue(uint16_t n) {
    uint16_t val;

    val = getNumber();
    if (signPrefix == 1)
        n = val;
    if (signPrefix == 2)
        n += val;
    if (signPrefix == 3)
        n -= val;
    return (n & 0x8000) ? 0 : n;
}

byte get8NewValue(byte n) {
    uint16_t val = get16NewValue(n);
    if ((val & 0xff00) != 0)
        undefinedCommandError();
    return (byte)val;
}

byte get8AbsNumber() {
    byte val = get8NewValue(0);
    if (signPrefix > 1)
        undefinedCommandError();
    return val;
}

bool openDefSrc() {              /* true if ok */
    if (srcFileOpt == SRCNESTED) /* cannot nest */
        invalidParam = true;
    else if (openSrc(&defFCB))
        return true;
    return false;
}

void setAltGetcToDefSrc() {
    if (openDefSrc()) { /* file opened so set up to read */
        altSrcOpt = 0;
        altGetc   = getDef;
    }
}

void setupPS() {
    if (DOpt) {
        txtBreak();
        enablePS();
        proportionalSpace = true;
        charSpacing       = 22;
    }
}

void getPath(FCB *fcb) {
    // path options are only valid on command line
    if (srcFileOpt != SRCCMDLINE || !(fcb->path = getCmdParameter()))
        fatalError("Invalid Parameter");
}

void getOptions() {
    byte fileOptChar;
    bool dollarSeen, SOpt, QOpt;

    SOpt              = false;
    QOpt              = false;
    proportionalSpace = false;
    OOpt              = false;
    errPrint          = false;
    FOpt              = false;
    manualFeed        = false;
    isPaging          = false;
    DOpt              = false;
    dollarSeen        = false;
    fileOptChar       = 0;

    while (getUCChar() != '\n') {
        if (dollarSeen) {
            if (fileOptChar == 0) {
                if (inputChar == 'S') /* $S - paging or manual feed for non file */
                    SOpt = true;
                else if (inputChar == 'F') /* $F - use form feed */
                    FOpt = true;
                else if (inputChar == 'D')
                    DOpt = charSpacingOverriden = true;
                else if (inputChar == 'O') { /* $Onn [mm] - range to print pages nn-mm */
                    OOpt      = true;
                    firstPage = get16NewValue(0);
                    if (inputChar != '\n')
                        lastPage = get16NewValue(0xffff);
                } else if (inputChar == 'N') /* $Nnn - initialise page number to nn */
                    pageNum = get16NewValue(pageNum);
                else if (inputChar == 'R') /* $R - alt cgetc uses default file */
                    setAltGetcToDefSrc();
                else if (inputChar == 'Q')
                    QOpt = true;
                else if (inputChar == 'C') /* prompt for continue at .NX */
                    COpt = true;
                else if (inputChar != '$' && inputChar != ' ' && inputChar != ',')
                    fileOptChar = inputChar;
            } else {
                if (fileOptChar == 'T') /* $Tpath where path is tex file directory path */
                    getPath(&texFCB);
                else if (fileOptChar == 'P') { /* $Ppath or $P- for stdout */
                    getPath(&prnFCB);
                    if (strcmp(prnFCB.path, "-") == 0)
                        prnToDev = 'X';
                } else if (fileOptChar == 'E') { /* $EY - enable error printing */
                    if (inputChar == 'Y')
                        errPrint = true;
                    else if (inputChar != 'X') /* $EX - no error printing (default) */
                        fatalError("InvalidParameter");
                } else if (fileOptChar == 'X') /* $XZ - no ix file */
                    if (inputChar == 'Z')
                        doIX = false;
                    else /* else $Xpath where path is ix file path */
                        getPath(&ixFCB);
                else
                    fatalError("InvalidParameter");
                fileOptChar = 0;
            }
        } else if (inputChar == '$')
            dollarSeen = true;
        if (inputChar == '\n')
            break;
    }
    if (fileOptChar != 0)
        fatalError("InvalidParameter");
    if (prnToDev == 'X') /* print to console - no page offset */
        pageOffset = 0;
    if (SOpt)
        if (prnToDev == 'X') /* if $S and $PX) paging */
            isPaging = true;
        else
            manualFeed = true; /* else if ($S) manual feed */
    selectDst();

    if (QOpt && DOpt)
        setupPS();
    if (FOpt)
        cputc(FF);
    if (DOpt) {
        putDstEscCode('X'); /* cancel all WP modes except PS & CR settling time */
        putDstEscCode('S'); /* return HMI control to spacing switch */
    }
}

void getFirstArgChar() { /* 1st arg can be proceeded by a space */
    if (cgetc() == ' ')
        cgetc();
}

byte getHeaderOrFooter(char *userBuf, byte width) {
    char buf[140]; // allow for user buffer & possible overrun with ESC sequences
    int ii              = 0;
    bool spacingChanged = false;
    memset(buf, ' ', sizeof(buf));
    getFirstArgChar();
    width--; // reserve space for '\0'
    while (ii < width && inputChar != '\n') {
        if (inputChar == escapeCharacter && toupper(cgetc()) == 'S') {
            jj = get8NewValue(12);
            if (DOpt) {
                buf[ii++] = ESC;
                buf[ii++] = 'W'; /* shadow print on */
                if (proportionalSpace) {
                    if (jj > 12) {
                        buf[ii++] = ESC; /* set offset to 3/120" */
                        buf[ii++] = DC1;
                        buf[ii++] = 3;
                    }
                } else if (jj != 12) {
                    buf[ii++]      = ESC;
                    buf[ii++]      = US;
                    buf[ii++]      = jj + 1;
                    spacingChanged = true;
                }
            }
        } else {
            buf[ii++] = inputChar;
            cgetc();
        }
    }

    if (spacingChanged) {
        buf[ii++] = ESC;
        buf[ii++] = 'S'; /* return HMI to spacing switch */
    }
    if (ii >= width) {
        invalidParam = true;
        ii           = width;
    }
    buf[ii] = 0;
    memcpy(userBuf, buf, width + 1); // copy string including '\0' and trailing spaces
    return ii;
}

void writeIXInfo() {
    char buf[128];

    if (!doIX)
        return;
    if (srcFileOpt == SRCUSERINPUT) /* input from console is invalid */
        if (altSrcOpt == 'X') {
            invalidParam = true;
            return;
        }
    if (!ixInitialised) {
        ixInitialised = true;
        if (ixFCB.path == NULL)
            ixFCB.path = texFCB.path;
        createFile(&ixFCB, true);
    }
    buf[0] = cmdChr2;
    buf[1] = pageNum % 256;
    buf[2] = pageNum / 256;
    buf[3] = getHeaderOrFooter(buf + 4, 124);
    fwrite(buf, 1, 128, ixFCB.fp);
}

byte getMax8BitNumber(byte oldVal, byte upper) {
    int i = get8NewValue(oldVal);
    if (i < upper)
        return i;
    invalidParam = true;
    return oldVal;
}

void writePrompt() {
    getFirstArgChar();
    putchar(BELL);

    while (inputChar != '\n') {
        putchar(inputChar);
        cgetc();
    }
    putchar('\n');
}

byte getLeadInCH(byte c) {
    ii = get8AbsNumber();
    if (inputChar != '\n')
        return inputChar;
    return c;
}

void condNewPage(byte n) {
    if (!roomOnPage(n + 1))
        finishPage();
}

void procDotCmd() {
    bool rescan;
    do {
        rescan = false;
        getUCChar();
        cmdChr1 = inputChar;
        getUCChar();
        cmdChr2          = inputChar;
        ignoreRestOfLine = true;
        invalidParam     = false;

        switch (cmdChr1) {
        default:
            undefinedCommandError();
            break;
        case 'A':
            if (cmdChr2 == 'D') { // .AD - adjust margins
                txtBreak();
                isJustified = true;
            } else
                undefinedCommandError();
            break;
        case 'B':
            if (cmdChr2 == 'O') { /* .BO n - bold for next n ?? */
                boCnt = get8NewValue(1);
                putEscCode('O');
            } else if (cmdChr2 == 'P') { /* .BP +-n - begin page */
                finishPage();
                pageNum = get16NewValue(pageNum);
                if (signPrefix > 1)
                    pageNum--;
                else if (inputChar == 'O' && !(pageNum & 1))
                    pageNum++;
            } else if (cmdChr2 == 'R') /* .BR - txtBreak */
                txtBreak();
            else
                undefinedCommandError();
            break;
        case 'C':
            if (cmdChr2 == '2') /* .C2 nc - set literal leadin char to c - n ! used */
                litLeadIn = getLeadInCH('`');
            else if (cmdChr2 == 'C') /* .CC nc - set command leadin char to c - n ! used */
                cmdLeadIn = getLeadInCH('.');
            else if (cmdChr2 == 'E') { /* .CE n - centre lines */
                txtBreak();
                centreLinesCnt = get8NewValue(1);
            } else if (cmdChr2 == 'L') /* .CL opts - process non file related options opts */
                getOptions();
            else if (cmdChr2 == 'P') /* .CP n - conditional page */
                condNewPage(get8AbsNumber());
            else if (cmdChr2 == 'S') { /* .CS n - set character spacing? */
                charSpacingOverriden = true;
                charSpacing          = getMax8BitNumber(charSpacing, 101);
            } else
                undefinedCommandError();
            break;
        case 'D':
            if (cmdChr2 == 'S') /* .DS - double space */
                lineSpacing = 2;
            else
                undefinedCommandError();
            break;

        case 'E':
            if (cmdChr2 == 'C') /* .EC nc - set escape char to c - n ! used */
                escapeCharacter = getLeadInCH('\\');
            else
                undefinedCommandError();
            break;
        case 'F':
            if (cmdChr2 == 'I') { /* .FI - FILL set formating to add extra space after . ? ! */
                txtBreak();
                formatOpt = PUNCTSPC;
            } else if (cmdChr2 == 'M') /* .FM +-n - set footer margin */
                footerMargin = getMax8BitNumber(footerMargin, bottomMargin);
            else if (cmdChr2 == 'T') /* .FT text - set footer text */
                getHeaderOrFooter(footerLine, 133);
            else
                undefinedCommandError();
            break;
        case 'H':
            if (cmdChr2 == 'E') /* .HE text - set heading */
                getHeaderOrFooter(headerLine, 133);
            else if (cmdChr2 == 'M') /* .HM +-n - set header margin */
                headerMargin = getMax8BitNumber(headerMargin, topMargin);
            else
                undefinedCommandError();
            break;
        case 'I':
            if (cmdChr2 == 'G') { /* .IG - ignore */
                while (inputChar != '.') {
                    skipToEOL();
                    cgetc();
                }
                rescan = true;
            } else if (cmdChr2 == 'N') { /* .IN +-n - indent */
                txtBreak();
                curIndent = getMax8BitNumber(curIndent, lineLen);
            } else if (cmdChr2 == 'X') /* .IX text - index - write index line type X */
                writeIXInfo();
            else
                undefinedCommandError();
            break;
        case 'L':
            if (cmdChr2 == 'I') { /* .LI - literal */
                txtBreak();
                skipToEOL();
                if (proportionalSpace) /* disable PS and set to no format */
                    disablePS(17);
                ii        = formatOpt;
                formatOpt = NOSPC;
                while (collectTextBlock()) { /* process until next . line */
                    outputLine();
                }
                formatOpt = ii; /* restore original format & PS */
                if (proportionalSpace)
                    enablePS();
                rescan = true;
            } else if (cmdChr2 == 'L') { /* .LL +-n - line length */
                txtBreak();
                lineLen = getMax8BitNumber(lineLen, 132);
            } else if (cmdChr2 == 'S') /* .LS n - line spacing */
                lineSpacing = get8NewValue(lineSpacing);
            else
                undefinedCommandError();
            break;
        case 'M':
            if (cmdChr2 == 'B') { /* .MB +-n - margin, bottom */
                bottomMargin = getMax8BitNumber(bottomMargin, pageLen - topMargin);
                footerMargin = min(footerMargin, bottomMargin - 1);
            } else if (cmdChr2 == 'T') { /* .MT +-n - margin, top */
                topMargin    = getMax8BitNumber(topMargin, pageLen - bottomMargin);
                headerMargin = min(headerMargin, topMargin - 1);
            } else
                undefinedCommandError();
            break;
        case 'N':
            if (cmdChr2 == 'A') { /* .NA - no adjust */
                txtBreak();
                isJustified = false;
            } else if (cmdChr2 == 'E') /* .NE n - Need n lines else new page */
                condNewPage(get8AbsNumber());
            else if (cmdChr2 == 'F') { /* .NF - NOFILL - simple space adjust no extra for . ? ! */
                txtBreak();
                formatOpt = JUSTIFYSPC;
            } else if (cmdChr2 == 'X') { /* .NX file - next file */
                if (COpt) {
                    fputs("Continue(Y/N) ?", stdout);
                    if (toupper(getch()) != 'Y')
                        nextSrcOrDone(); /* check for file on command line */
                    /* possible bug falls through to opening src below */
                    putchar('\n');
                }
                if (openSrc(&texFCB)) /* check for file after .NX */
                    ;
                else if (srcFileOpt != SRCTEX) /* if no file, switch back to tex file if needed */
                    selectSrc(SRCTEX);
                else
                    nextSrcOrDone(); /* else get next file from command line */
                ignoreRestOfLine = false;
            } else
                undefinedCommandError();
            break;
        case 'O':
            if (cmdChr2 == 'P') /* .OP - omit page numbers */
                doPageNum = 0;
            else
                undefinedCommandError();
            break;
        case 'P':
            if (cmdChr2 == 'A') { /* .PA n - page advance */
                txtBreak();
                jj = get8NewValue(1);
                if (curLine == 0 && jj != 0) /* if at top of page and include this in page count */
                    jj--;
                for (ii = 1; ii <= jj; ii++)
                    spacing(pageLen);
            } else if (cmdChr2 == 'C') /* .PC nc - set page number marker */
                pageNumMarker = getLeadInCH('%');
            else if (cmdChr2 == 'L') { /* .PL +-n - set page length */
                if (bottomMargin + topMargin < (ii = get8NewValue(pageLen)))
                    pageLen = min(ii, 255);
                else
                    invalidParam = true;
            } else if (cmdChr2 == 'N') { /* .PN +-n - set page number */
                pageNum   = get16NewValue(pageNum);
                doPageNum = 0xff;
            } else if (cmdChr2 == 'O') { /* .PO +-n - set page offset */
                txtBreak();
                pageOffset = get8NewValue(pageOffset);
            } else if (cmdChr2 == 'P') { /* .PP n m - paragraph - n indent, m min line count */
                txtBreak();
                spacing(1);
                condNewPage(minParaLinesOnPage);
                tmpIndent = ppHangingIndent = getMax8BitNumber(ppHangingIndent, lineLen);
                if (inputChar != '\n')
                    minParaLinesOnPage = getMax8BitNumber(minParaLinesOnPage, pageLen);
            } else if (cmdChr2 == 'S') /* .PS - enable proportional space */
                setupPS();
            else
                undefinedCommandError();
            break;
        case 'Q':
            if (cmdChr2 == 'B' || cmdChr2 == 'S') { /* .QB or .QS - quit bold / shadow */
                putEscCode('&');                    /* bold/shadow off */
                if (!proportionalSpace)
                    putEscCode('S');     /* return HMI to spacing switch */
            } else if (cmdChr2 == 'I') { /* .QI - quit indent */
                txtBreak();
                curIndent = 0;
            } else if (cmdChr2 == 'P') { /* .QP - quit proportional space */
                txtBreak();
                putEscCode('Q'); /* disable PS */
                proportionalSpace = 0;
                charSpacing       = 5;
            } else if (cmdChr2 == 'U') { /* .QU - quit underline */
                ii = nline;
                if (escColumn[escIdx] != nline)
                    trimTrailingSpace();
                putEscCode('R'); /* underscore off */
                nline = ii;
            } else
                undefinedCommandError();
            break;
        case 'R':
            if (cmdChr2 ==
                'D') { /* .RD prompt - get used input from response file / command line */
                /* bad if reading from nested file */
                if (srcFileOpt != SRCNESTED || altSrcOpt != 0) {
                    writePrompt();
                    skipToEOL();
                    selectSrc(SRCUSERINPUT);
                } else
                    invalidParam = true;
            } else if (cmdChr2 == 'F') /* .RF file - set response file - ! valid in nested file */
                                       /* continues with existing alt file if file ! given */
                                       /* but will then txtBreak if read from file is needed !!!! */
                setAltGetcToDefSrc();  /* sets alt cgetc to use default file */
            else
                undefinedCommandError();
            break;
        case 'S':
            if (cmdChr2 == 'H') { /* .SH n - set shadow printing */
                putEscCode('W');  /* shadow print on */
                ii = get8NewValue(12);
                if (proportionalSpace) {
                    if (ii > 12) { /* add 3/120" to character spacing */
                        putEscCode(DC1);
                        markEscCode(3);
                    }
                } else if (ii != 12) { /* set HMI to n-1 */
                    putEscCode(US);
                    markEscCode(ii);
                    printerHMISet = true;
                }
            } else if (cmdChr2 == 'O') { /* .SO file - source from file */
                txtBreak();
                if (openDefSrc()) {
                    skipToEOL();
                    selectSrc(SRCNESTED);
                }
            } else if (cmdChr2 == 'P') { /* .SP n - space lines */
                txtBreak();
                spacing(get8NewValue(1));
            } else if (cmdChr2 == 'S') /* .SS - single space */
                lineSpacing = 1;
            else if (cmdChr2 == 'T') {
                writePrompt();
                txtBreak();
                waitKey();
            } else
                undefinedCommandError();
            break;
        case 'T':
            if (cmdChr2 == 'A') { /* .TA n... - set tab stops */
                ii = 0xff;
                while (inputChar != '\n') {
                    ii++;
                    tabstops[ii] = get8NewValue(tabstops[ii]);
                }
            } else if (cmdChr2 == 'C') /* .TC text - TOC write index line type T */
                writeIXInfo();
            else if (cmdChr2 == 'I') { /* .TI +-n - temporary indent */
                txtBreak();
                jj        = getMax8BitNumber(curIndent, lineLen);
                tmpIndent = jj - curIndent;
            } else if (cmdChr2 == 'M') /* .TM msg - type message */
                writePrompt();
            else
                undefinedCommandError();
            break;
        case 'U':
            if (cmdChr2 == 'L') { /* .UL n - underline */
                ulCnt = get8NewValue(1);
                putEscCode('E'); /* underscore on */
            } else
                undefinedCommandError();
            break;
        } /* case */
    } while (rescan);

    if (invalidParam)
        error(&cmdChr1);
    if (ignoreRestOfLine)
        skipToEOL();
}

void mkName(FCB *fcb, char *ext) {
    char *s;
    if (fcb->path) {
        strcpy(fcb->name, fcb->path);
        strcat(fcb->name, fname(texFCB.name));
    } else
        strcpy(fcb->name, texFCB.name);
    if (s = strrchr(fname(fcb->name), '.'))
        strcpy(s, ext);
    else
        strcat(fcb->name, ext);
}

int main(int argc, char **argv) {
    
    if (argc == 2 && stricmp(argv[1], "-v") == 0) {
        showVersion(stdout, argv[1][1] == 'V');
        exit(0);
    }
    _argc = argc;
    _argv = argv;

    getOptions();        /* pick up options from command line */
    if (prnToDev != 'X') /* unless output is to console show version */
        fputs("\n" VERSION "\n", stdout);
    arg    = 1;
    argIdx = 0;
    if (!openSrc(&texFCB)) /* open tex file or exit if nothing */
        wrapup();
    mkName(&ixFCB, ".ix");
    if (!prnToDev) {
        mkName(&prnFCB, ".prn");
        createFile(&prnFCB, false); /* create the file */
    }
    selectSrc(SRCTEX); /* select from tex file */
    while (1) {
        if (collectTextBlock())
            outputLine();
        else
            procDotCmd();
    }
}
