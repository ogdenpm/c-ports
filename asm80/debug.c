/****************************************************************************
 *  debug.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"

static char *opNames[] = {
    "BEGIN", "\\r", "(", ")", "*", "+", ",", "-", "unary +", "/", "unary -",
    "EQ", "LT", "LE", "GT", "EE", "NE", "NOT", "AND", "OR", "XOR",
    "MOD", "SHL", "SHR", "HIGH", "LOW", "DB", "DW", "DS", "EQU", "SET",
    "ORG", "END", "IF", "ELSE", "ENDIF", "LXI", "REG16", "LDSTAX", "ARITH", "IMM8",
    "MVI", "INRDCR", "MOV", "IMM16", "SINGLE", "RST", "ASEG", "CSEG", "DSEG",
    "PUBLIC", "EXTRN", "NAME", "STKLN", "MACRO", "MACROBODY", "ENDM", "EXITM",
    "MACRONAME", "IRP", "IRPC", "ITERPARAM", "REPT", "LOCAL", "OPTVAL", "NUL"
};


void ShowYYType(void) {
    if (yyType < sizeof(opNames) / sizeof(*opNames))
        printf(" %i:%s\n", yyType, opNames[yyType]);
    else
        printf(" %i:%02X\n", yyType, yyType);
}
void DumpSymbols(byte tableId)
{
    char token[7];

    tokensym_t *s, *e;
    s = symTab[tableId];
    e = endSymTab[tableId];
    if (s == e)
        printf("no data for symtab[%d]\n", tableId);
    else {
        printf("symtab[%d]\n", tableId);
        while (s < e) {
            UnpackToken(s->tok, (byte *)token);
            token[6] = 0;
            printf("tok = \"%s\", line/val = %04X, type = %02X, flags = %02X\n", token, s->addr, s->type, s->flags);

            s++;
        }
    }
}

void DumpOpStack(void) {
    printf("OpStack:");
    for (int i = 0; i <= opSP; i++) {
        if (opStack[i] < sizeof(opNames) / sizeof(*opNames))
            printf(" %i:%s", i, opNames[opStack[i]]);
        else
            printf(" %i:%02X", i, opStack[i]);
    }
    printf("\n");
}

void DumpTokenStackItem(int i, bool pop)
{
    char tok[7];
    tokensym_t *s;

    if (i == 0 && pop)
        printf("^");
    else
        printf("%1d", pop ? i - 1 : i);

    if (token[i].size == 4) {
        UnpackToken((wpointer)token[i].start, (byte *)tok);
        printf(" %6.6s", tok);
    }
    else if (token[i].size == 1)
        printf(" %6d", *token[i].start);
    else if (token[i].size == 2)
        printf(" %6d", *(wpointer)token[i].start);
    else
        printf(" %6.*s", token[i].size, token[i].start);
    printf("  %02X   %02X  %3d  %3d", token[i].type, token[i].attr, token[i].size, token[i].symId);
    s = token[i].symbol;
    if (s /*&& (symTab[TID_SYMBOL] <= s && s <= endSymTab[TID_SYMBOL] || symTab[TID_MACRO] <= s && s <= endSymTab[TID_MACRO]) */) {
        UnpackToken(s->tok, (byte *)tok);
        printf("   %6.6s %04X   %02X   %02X\n", tok, s->addr, s->type, s->flags);
    }
    else
        printf("\n");
}
void DumpTokenStack(bool pop)
{
//    char token[7];
//    token[6] = 0;

    printf("TokenStack:\n");
    printf("  Token  Type Attr Size  Id | Sym    Addr  Type Flags\n");
    DumpTokenStackItem(0, pop);
    for (int i = tokenIdx; i > 0; i--)
        DumpTokenStackItem(i, pop);

}