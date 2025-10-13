#include "plm81.h"

#define MAXMACDEF 2048 /* max number of macro definitions */
#define MAXMACUSE 32   /* max nested macro depth */

#ifdef __GNUC__
#include <limits.h>
#define _MAX_PATH PATH_MAX
#endif

char path[_MAX_PATH];

FILE *srcFp;
FILE *polFp;
FILE *symFp;
FILE *lstFp;

int inSP = 0;
FILE *instk[7 + 1];
char *src;

void closefiles(void) {
    while (inSP > 0) {
        fclose(srcFp);
        srcFp = instk[inSP--];
    }
    fclose(srcFp);
    if (polFp) {
        fclose(polFp);
        if (errorCnt)
            unlink(makeFilename(src, ".pol", true));
    }
    if (symFp) {
        fclose(symFp);
        if (errorCnt)
            unlink(makeFilename(src, ".sym", true));
    }
    if (lstFp)
        fclose(lstFp);
}

void openfiles(char *srcFile) {
    src = srcFile;
    char *path;
    if (!(srcFp = fopen(path = makeFilename(srcFile, ".plm", false), "rt"))) {
        fprintf(stderr, "can't open source file %s\n", path);
        exit(1);
    }
    atexit(closefiles);
    if (!(polFp = fopen(path = makeFilename(srcFile, ".pol", true), "wb"))) {
        fprintf(stderr, "can't create pol file %s\n", path);
        exit(1);
    }
    if (!(symFp = fopen(path = makeFilename(srcFile, ".sym", true), "wb"))) {
        fprintf(stderr, "can't create symbol file %s\n", path);
        exit(1);
    }
    if (!(lstFp = fopen(path = makeFilename(srcFile, ".lst", true), "wt"))) {
        fprintf(stderr, "can't create listing file %s\n", path);
        exit(1);
    }
}





#define INMAX 256
static char ibuff[INMAX + 1];
char *ibptr = "";

char *readline(FILE *srcFp) {
    /* read a line from the input stream
       return a pointer to the line or NULL if no line is available
       expands TABs as determined by the $T option
       \r is dropped and other control characters are converted to a space
       the line is terminated with a \n if not truncated otherwise with \r
    */
    int col = 0;
    char *s = ibuff;
    int c;

    while ((c = getc(srcFp)) != EOF && c != '\n') {
        if (c == '\t')
            do {
                if (col++ < INMAX - 1)
                    *s++ = ' '; // convert tab to space
            } while (col % C_TAB);
        else if (c && c != '\r' && col++ < INMAX - 1)
            *s++ = c < ' ' ? ' ' : c;
    }
    if (col == 0 && c != '\n')
        return NULL; // no line available
    *s++ = col < INMAX - 1 ? '\n' : '\r';
    *s   = '\0';
    return ibuff;
}

// return the next character from the input stream or macro
// return EOF if no character is available
int gnc() {
    int ch;

    if ((ch = macGetc())) // get next character from macro if available
        return ch;

    while (!*ibptr) {
        for (;;) {
            if ((ibptr = readline(srcFp)))
                break;      // got a line
            if (inSP < 1)   // check if we are in an include file
                return EOF; // no more input
            fclose(srcFp);
            srcFp = instk[inSP--]; // pop include stack
        }
        emit( LIN,++C_LINECNT);
        char *s;
        if ((s = strchr(ibuff, '\r'))) { // line was truncated
            *s = '\n';
            fprintf(lstFp, "WARNING: line %d truncated\n", C_LINECNT);
        }

        if (C_PRINT != 0) { // print the line to listing file, wrapped if required
            int column = fprintf(lstFp, "%05d%3d %c  ", (uint16_t)C_LINECNT, curblk - 1,
                                 inSP > 0 ? '=' : ' ');
            for (s = ibuff; *s != '\n'; s++, column++) {
                if (column == C_WIDTH - 1 && s[1] != '\n')
                    column = fprintf(lstFp, "\\\n%12s", "");
                putc(*s, lstFp);
            }
            putc('\n', lstFp);
        }
        if (*ibptr == '$') {
            parseOptions(ibuff + 1); // parse the options
            ibptr = "";
        }
    }

    return *(uint8_t *)ibptr++; // make sure 0x80-0xff are returned as unsigned
}

void parseOptions(char *s) {

    while (*s && *s != '\n') {     // process all of line
        while (*s && !isalpha(*s)) // skip to option
            s++;
        if (!*s)
            return;
        int option = toupper(*s);
        while (isalpha(*s)) // skip over option long name
            s++;
        while (isspace(*s))
            s++;
        int val = contrl[option - 'A'];
        if (*s == '=') {
            while (isspace(*++s))
                ;
            if (option == 'I') {
                char *incName = s;
                while (*s && !isspace(*s))
                    s++;
                *s = 0;
                if (strlen(incName) == 0)
                    error("36: include file name missing");
                else
                    stackc(incName); // push include file on stack
                return;
            }
            val = 0;
            if (!isdigit(*s))
                error("51: missing value after = on $ control line");
            else
                while (isdigit(*s)) // get value
                    val = val * 10 + *s++ - '0';
        } else if (option == 'I') {
            error("36: include file name missing");
            return;
        } else if (val == 0 || val == 1)
            val = !val; // toggle the value
        else
            error("34: attempt to toggle non binary $ control");
        contrl[option - 'A'] = val;
        if (C_TAB < 1 || C_TAB > 8) {
            error("52: invalid tab size %d, must be between 1 and 8", C_TAB);
            C_TAB = 1;
        }
    }
}

void stackc(char *fname) {
    if (inSP >= 7)
        fatal("35: too many include files");
    else {
        FILE *incFp = fopen(makeFilename(fname, ".plm", false), "rt");
        if (!incFp)
            fprintf(stderr, "can't open include file %s. Skipping\n", fname);
        else {
            instk[++inSP] = srcFp;
            srcFp         = incFp;
        }
    }
}

/* macro support features */

static struct {
    int16_t idLen;
    uint16_t idStr;
} macdef[MAXMACDEF];

static struct {
    uint16_t macId;
    uint16_t macStr;
} macuse[MAXMACUSE];

uint16_t macdefSP        = 0;
static uint16_t macuseSP = 0;
uint16_t maxMacDef       = 0;

bool defMacro(int idLen, char *id, int valLen, char *val) {
    if (macdefSP + 1 >= ASIZE(macdef))
        return false; // Not enough space for new macro definition

    int idStr;
    // string is stored as identifier, text, '\0'
    if (!((idStr = newString(idLen, id)) && newString(valLen, val) && newString(1,"")))
        return false;                 // Failed to create string
    macdef[++macdefSP].idLen = idLen; // create the macro definition
    macdef[macdefSP].idStr   = idStr;

    if (macdefSP > maxMacDef)
        maxMacDef = macdefSP; // Update maxMacDef if current is larger

    return true;
}

// see if str invokes a macro
// return true if macro setup for expansion
bool useMacro(int len, char *str) {
    for (int i = macdefSP; i > 0; i--) {
        if (macdef[i].idLen == len && strequ(macdef[i].idStr, str, len)) {
            if (macuseSP + 1 >= ASIZE(macuse)) {
                fatal("6: too many nested macro expansions. Limit %d", ASIZE(macuse) - 1);
                return false; // Not enough space for macro use
            }
            macuse[++macuseSP].macId = i; // Store the index of the macro definition
            // text of macro follows after the identifier
            macuse[macuseSP].macStr = macdef[i].idStr + macdef[i].idLen;
            macdef[i].idLen         = -macdef[i].idLen; // negate to indicate in use
            return true;
        }
    }
    return false; // No matching macro found
}

// drop macros defined in the scope just ended
// reports error if a macro was mid expansion
void dropMacro(int newTop) {
    if (macdefSP > newTop) {
        macdefSP = newTop;
        for (int i = macuseSP; i > 0; i--) {
            if (macuse[i].macId >= newTop) {
                error("49: macro '%.*s' terminated its scope mid expansion", -macdef[i].idLen,
                      idToStr(macdef[i].idStr));
                macuse[i].macId = 0; // mark out of scope
                while (*(idToStr(macuse[i].macStr)))
                    macuse[i].macStr++;
            }
        }
    }
}

// get next macro char, handle nested macros, returns 0 if no chars left
int macGetc() {
    int ch;
    while (macuseSP) {
        if (macuse[macuseSP].macId) { // ignore if macro now out of scope
            if ((ch = *(uint8_t *)(idToStr(macuse[macuseSP].macStr++))))
                return ch;
            if (macuse[macuseSP].macId)
                macdef[macuse[macuseSP].macId].idLen = -macdef[macuse[macuseSP].macId].idLen;
        }
        macuseSP--;
    }
    return 0;
}

void decibp() {
    if (macuseSP)
        macuse[macuseSP].macStr--;
    else
        ibptr--;
}
