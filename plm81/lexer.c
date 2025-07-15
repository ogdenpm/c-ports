#include "plm81.h"



// vindx contains the start index of the first token of a given length
const uint8_t vindx[] = { 0, 1, 14, 20, 26, 35, 39, 41, 45, 47, 50 };
// clang-format off
const char *tokens[]  = {
    /* 0 */ "null",
    /* 1 */ ";", ")", "(", ",", ":", "=", "<", ">", "+", "-", "*", "/", ".",
    /*14 */ "IF", "DO", "GO", "TO", "OR", "BY",
    /*20 */ "EOF", "END", "XOR", "AND", "NOT", "MOD",
    /*26 */ "HALT", "THEN", "ELSE", "CASE", "CALL", "GOTO", "DATA", "BYTE", "PLUS",
    /*35 */ "LABEL", "BASED", "MINUS", "WHILE",
    /*39 */ "ENABLE", "RETURN", "DISABLE", "DECLARE", "ADDRESS", "INITIAL",
    /*45 */ "<NUMBER>", "<STRING>",
    /*47 */ "INTERRUPT", "PROCEDURE", "LITERALLY",
    /*50 */ "<IDENTIFIER>",
    /*51 */ "<TO>", "<BY>",
    /*53 */ "<TYPE>", "<TERM>",
    /*55 */ "<GROUP>", "<WHILE>", "<GO TO>",
    /*58 */ "<ENDING>", "<PROGRAM>",
    /*60 */ "<REPLACE>", "<PRIMARY>",
    /*62 */ "<VARIABLE>", "<CONSTANT>", "<RELATION>",
    /*65 */ "<STATEMENT>", "<IF CLAUSE>", "<TRUE PART>", "<DATA LIST>", "<DATA HEAD>", "<LEFT PART>",
    /*71 */ "<ASSIGNMENT>", "<EXPRESSION>", "<GROUP HEAD>", "<BOUND HEAD>",
    /*75 */ "<IF STATEMENT>", "<WHILE CLAUSE>", "<INITIAL LIST>", "<INITIAL HEAD>",
    /*79 */ "<CASE SELECTOR>", "<VARIABLE NAME>", "<CONSTANT HEAD>",
    /*82 */ "<STATEMENT LIST>", "<CALL STATEMENT>", "<PROCEDURE HEAD>",
    /*85 */ "<PROCEDURE NAME>", "<PARAMETER LIST>", "<PARAMETER HEAD>",
    /*88 */ "<BASED VARIABLE>", "<LOGICAL FACTOR>", "<SUBSCRIPT HEAD>",
    /*91 */ "<BASIC STATEMENT>", "<GO TO STATEMENT>", "<STEP DEFINITION>",
    /*94 */ "<IDENTIFIER LIST>", "<LOGICAL PRIMARY>",
    /*96 */ "<RETURN STATEMENT>", "<LABEL DEFINITION>", "<TYPE DECLARATION>",
    /*99 */ "<ITERATION CONTROL>", "<LOGICAL SECONDARY>",
    /*101*/ "<LOGICAL EXPRESSION>",
    /*102*/ "<DECLARATION ELEMENT>",
    /*103*/ "<PROCEDURE DEFINITION>",
    /*104*/ "<DECLARATION STATEMENT>", "<ARITHMETIC EXPRESSION>",
    /*106*/ "<IDENTIFIER SPECIFICATION>"
};
/* token ids */

int acclen;
#define MAXSTR 4096
uint8_t accum[MAXSTR + 1];
int accumIVal = 0;
int token = 0;

uint16_t topStr = 1;
static uint8_t string[0x10000];

int newString(uint16_t len, const uint8_t *str) {
    if (topStr + len >= sizeof(string)) {
        fatal("2: pass-1 string table overflow");
        return 0; // Not enough space
    }

    uint16_t base = topStr; // the base address of the last string
    memcpy(&string[topStr], str, len);
    topStr += len; // Move topStr to the next available position
    return base;
}

bool strequ(int strId, uint8_t const *str, int len) {
    if (strId < 0 || strId >= topStr || len < 0)
        return false;
    return memcmp(&string[strId], str, len) == 0;
}

uint8_t *idToStr(int loc) {
    return string + loc;
}



void scan() {

    /*      global tables */
    /*     scan finds the next entity in the input stream */
    /*     the resulting item is placed into accum (of length */
    /*     acclen).  token is set to  the item found */

    for (;;) {
        failsf = true;
        int ch;
        while (isspace(ch = gnc()))
            ;
        acclen = 0;
        if (ch == EOF) {
            token = EOFILE;
            return;
        }
        if (isdigit(ch)) { /*     number */
            token = NUMBV;
            while (isxdigit(ch)) {
                accum[acclen++] = ch;                // cannot exceed line length so safe
                while ((ch = toupper(gnc())) == '$') // gobble $ in number
                    ;
            }
            /*     check radix */
            uint8_t radix;
            if (ch == 'H') // H
                radix = 16;
            else if (ch == 'Q' || ch == 'O') // Q or O
                radix = 8;
            else {
#pragma warning(suppress: 6385)
                if (accum[acclen - 1] == 'B') { // B
                    radix = 2;
                    acclen--;
                } else {
                    radix = 10;
                    if (accum[acclen - 1] == 'D') // D
                        acclen--;
                }
                decibp();
            }
            accumIVal = 0;
            for (int i = 0; i < acclen; i++) {
                uint8_t digit = accum[i];
                digit         = isdigit(digit) ? digit - '0' : digit - 'A' + 10;
                if (digit >= radix || (accumIVal = accumIVal * radix + digit) >= 0x10000) {
                    error("6: number conversion error");
                    accumIVal = 0;
                    break;
                }
            }
            return;
        } else if (isalpha(ch)) { // alpha character
            token = IDENTV;       /*     identifier */
            while (isalnum(ch)) {
                if (acclen < 32)
                    accum[acclen++] = toupper(ch);
                while ((ch = gnc()) == '$') // gobble up $ in name
                    ;
            }
            decibp();
        } else if (!isalnum(ch)) { // non alpha numeric
            if (ch == '\'') {      // quote
                bool warned = false;
                token       = STRV;
                for (;;) {
                    while ((ch = gnc()) == '\n') // ignore nl
                        ;
                    if (ch == '\'') {
                        while ((ch = gnc()) == '\n') // allow '' to straddle line break
                            ;
                        if (ch != '\'') // nope wasn't '' so we have all of the string
                            break;
                    }
                    if (ch < ' ')
                        ch = ' ';
                    if (acclen < MAXSTR)
                        // stuff char (double quote reduced to single)
                        accum[acclen++] = C_UPPER ? toupper(ch) : ch;
                    else if (!warned) {
                        error("xx: string too long, possibly missing closing quote");
                        warned = true;
                    }
                }
                decibp(); // backup one
                return;
            } else {
                acclen   = 1;
                accum[0] = ch;
                token    = 0;
                if (ch == '/') { // look for comment
                    if ((ch = gnc()) != '*')
                        decibp();
                    else { // got a comment
                        while (ch != '/') {
                            /*     comment found */
                            while (ch != EOF && (ch = gnc()) != '*')
                                ;
                            while (ch == '*')
                                ch = gnc();
                            if (ch == EOF) {
                                token = EOFILE;
                                return;
                            }
                        }
                        continue; // comment skipped try again
                    }
                }
            }
        }
        if (acclen < sizeof(vindx) - 1) {
            /*     search for token in vocabulary */
            for (int i = vindx[acclen]; i < vindx[acclen + 1]; i++) {
                if (memcmp(accum, tokens[i], acclen) == 0) {
                    token = i;
                    return;
                }
            }
        }
        // if we get here it is not a keyword/keysymbol
        // if its an identifier and not used as a macro return it
        if (token == IDENTV && !useMacro(acclen, accum))
            return;
        // if its a single character then its spurious
        if (token == 0) {
            error("8: illegal symbol");
        }
    }
}
