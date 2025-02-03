/****************************************************************************
 *  debug.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"
#include <stdio.h>

#ifdef _DEBUG

int symMode        = 1;
int infoMode       = 0;

char *builtinStr[] = { "CARRY", "DEC",    "DOUBLE", "HIGH",     "INPUT", "LAST", "LENGTH", "LOW",
                       "MOVE",  "OUTPUT", "PARITY", "ROL",      "ROR",   "SCL",  "SCR",    "SHL",
                       "SHR",   "SIGN",   "SIZE",   "STACKPTR", "TIME",  "ZERO" };

#if 0
 void dumpAllInfo()
 {
     index_t p;
     for (p = botInfo + (infoMode == 0 ? 2 : procInfo[1]); p < topInfo; p += InfoP(p)->len) {
         showInfo(p, stdout);
         putchar('\n');
     }

}
#endif

char *flagsstr[] = { "F_PUBLIC",    "F_EXTERNAL",  "F_BASED",    "F_INITIAL",   "F_REENTRANT",
                     "F_DATA",      "F_INTERRUPT", "F_AT",       "F_ARRAY",     "F_STARDIM",
                     "F_PARAMETER", "F_MEMBER",    "F_LABEL",    "F_AUTOMATIC", "F_PACKED",
                     "F_ABSOLUTE",  "F_MEMORY",    "F_DECLARED", "F_DEFINED",   "F_MODGOTO" };

void showFlags(uint32_t flags, FILE *fp) {
    for (int i = 0; i < 20; i++) {
        if (flags & (1 << i))
            fprintf(fp, " %s", flagsstr[i]);
    }
}

void showInfo(index_t idx, FILE *fp) {
    info_t *info = FromIdx(idx);

    switch (info->type) {
    case 0:
        fprintf(fp, ": LIT");
        break;
    case 1:
        fprintf(fp, ": LABEL");
        break;
    case 2:
        fprintf(fp, ": BYTE");
        break;
    case 3:
        fprintf(fp, ": ADDRESS");
        break;
    case 4:
        fprintf(fp, ": STRUCT");
        break;
    case 5:
        fprintf(fp, ": PROC");
        break;
    case 6:
        fprintf(fp, ": BUILTIN");
        break;
    case 7:
        fprintf(fp, ": MACRO");
        break;
    case 8:
        fprintf(fp, ": UNKNOWN");
        break;
    case 9:
        fprintf(fp, ": COND");
        break;
    default:
        fprintf(fp, ": OOPS type = %d\n", info->type);
    }
    fprintf(fp, " '%s'", info->sym ? info->sym->name->str : "(Null)");
    fprintf(fp, ", scope = %04X", info->scope);

    switch (info->type) {
    case LIT_T:
        fprintf(fp, ", lit = '");
        for (char const *s = info->lit->str; s[1] && s[1] != '\n'; s++)
            putc(*s, fp);
        putc('\'', fp);

        break;
    case LABEL_T:
    case BYTE_T:
    case ADDRESS_T:
    case STRUCT_T:
    case PROC_T:
        fprintf(fp, ", flags =");
        showFlags(info->flag, fp);
        break;
    case BUILTIN_T:
        fprintf(fp, ", builtin = %s, paramCnt = %d, data type = %d", builtinStr[info->builtinId],
                info->paramCnt, info->returnType);
        break;
    case MACRO_T:
        fprintf(fp, ", flags =");
        showFlags(info->flag, fp);
        break;
    case CONDVAR_T:
        fprintf(fp, ", flag =  %02X", info->condFlag);
        break;
    case UNK_T:
        break;
    }
    if (LABEL_T <= info->type && info->type <= PROC_T) {
        if (info->flag & F_EXTERNAL)
            fprintf(fp, ", extId = %d", info->extId);
        if (info->type >= BYTE_T) {
            if (info->parent)
                fprintf(fp, ", parent = %s",
                        info->parent->sym ? info->parent->sym->name->str : "unknown");
        }
    }
    if (info->type == PROC_T) {
        fprintf(fp, ", intno = %d", info->intno);
        fprintf(fp, ", procId = %d", info->procId);
    }
}

#ifdef TRACE
void dumpBuf(vfile_t *fp) {
    for (uint32_t i = 0; i < fp->size; i++)
        printf("%02X%c", fp->content[i], i % 16 == 15 ? '\n' : ' ');
    putchar('\n');
}

char *tx2NameTable[] = {
    "T2_LT",         "T2_LE",          "T2_NE",         "T2_EQ",         "T2_GE",
    "T2_GT",         "T2_ROL",         "T2_ROR",        "T2_SCL",        "T2_SCR",
    "T2_SHL",        "T2_SHR",         "T2_JMPFALSE",   "T2_13",         "T2_14",
    "T2_15",         "T2_16",          "T2_17",         "T2_DOUBLE",     "T2_PLUSSIGN",
    "T2_MINUSSIGN",  "T2_STAR",        "T2_SLASH",      "T2_MOD",        "T2_AND",
    "T2_OR",         "T2_XOR",         "T2_BASED",      "T2_BYTEINDEX",  "T2_WORDINDEX",
    "T2_MEMBER",     "T2_UNARYMINUS",  "T2_NOT",        "T2_LOW",        "T2_HIGH",
    "T2_ADDRESSOF",  "T2_PLUS",        "T2_MINUS",      "T2_LT_VAL",     "T2_LE_VAL",
    "T2_NE_VAL",     "T2_EQ_VAL",      "T2_GE_VAL",     "T2_GT_VAL",     "T2_44",
    "T2_45",         "T2_46",          "T2_47",         "T2_48",         "T2_49",
    "T2_50",         "T2_51",          "T2_52",         "T2_53",         "T2_54",
    "T2_55",         "T2_56",          "T2_TIME",       "T2_STKBARG",    "T2_STKWARG",
    "T2_DEC",        "T2_COLONEQUALS", "T2_OUTPUT",     "T2_CASEBLOCK",  "T2_STKARG",
    "T2_65",         "T2_66",          "T2_67",         "T2_68",         "T2_MOVE",
    "T2_70",         "T2_RETURNBYTE",  "T2_RETURNWORD", "T2_RETURN",     "T2_74",
    "T2_75",         "T2_76",          "T2_77",         "T2_78",         "T2_79",
    "T2_80",         "T2_81",          "T2_82",         "T2_83",         "T2_84",
    "T2_85",         "T2_86",          "T2_87",         "T2_88",         "T2_89",
    "T2_90",         "T2_91",          "T2_92",         "T2_93",         "T2_94",
    "T2_95",         "T2_96",          "T2_97",         "T2_98",         "T2_99",
    "T2_100",        "T2_101",         "T2_102",        "T2_103",        "T2_104",
    "T2_105",        "T2_106",         "T2_107",        "T2_108",        "T2_109",
    "T2_110",        "T2_111",         "T2_112",        "T2_113",        "T2_114",
    "T2_115",        "T2_116",         "T2_117",        "T2_118",        "T2_119",
    "T2_120",        "T2_121",         "T2_122",        "T2_123",        "T2_124",
    "T2_125",        "T2_126",         "T2_127",        "T2_128",        "T2_129",
    "T2_ADDW",       "T2_BEGMOVE",     "T2_CALL",       "T2_CALLVAR",    "T2_SETSTMTNO",
    "T2_PROCEDURE",  "T2_LOCALLABEL",  "T2_CASELABEL",  "T2_LABELDEF",   "T2_INPUT",
    "T2_GOTO",       "T2_JMP",         "T2_JNC",        "T2_JNZ",        "T2_SIGN",
    "T2_ZERO",       "T2_PARITY",      "T2_CARRY",      "T2_DISABLE",    "T2_ENABLE",
    "T2_HALT",       "T2_STMTCNT",     "T2_LINEINFO",   "T2_MODULE",     "T2_SYNTAXERROR",
    "T2_TOKENERROR", "T2_EOF",         "T2_LIST",       "T2_NOLIST",     "T2_CODE",
    "T2_NOCODE",     "T2_EJECT",       "T2_INCLUDE",    "T2_ERROR",      "T2_SETADDR",
    "T2_165",        "T2_166",         "T2_167",        "T2_168",        "T2_169",
    "T2_170",        "T2_171",         "T2_IDENTIFIER", "T2_NUMBER",     "T2_BIGNUMBER",
    "T2_175",        "T2_176",         "T2_177",        "T2_178",        "T2_179",
    "T2_180",        "T2_STACKPTR",    "T2_SEMICOLON",  "T2_OPTBACKREF", "T2_CASE",
    "T2_ENDCASE",    "T2_ENDPROC",     "T2_LENGTH",     "T2_LAST",       "T2_SIZE",
    "T2_BEGCALL",    "T2_254"
};

char *tx2Name(byte op) {

    return tx2NameTable[op];
}

#endif
char *t1Items[] = {
    "T1_LINEINFO",  "T1_SYNTAXERROR", "T1_TOKENERROR", "T1_LIST",      "T1_NOLIST",
    "T1_CODE",      "T1_NOCODE",      "T1_EJECT",      "T1_INCLUDE",   "T1_STMTCNT",
    "T1_LABELDEF",  "T1_LOCALLABEL",  "T1_JMP",        "T1_JMPFALSE",  "T1_PROCEDURE",
    "T1_SCOPE",     "T1_END",         "T1_DO",         "T1_DOLOOP",    "T1_WHILE",
    "T1_CASE",      "T1_CASELABEL",   "T1_IF",         "T1_STATEMENT", "T1_CALL",
    "T1_RETURN",    "T1_GO",          "T1_GOTO",       "T1_SEMICOLON", "T1_ENABLE",
    "T1_DISABLE",   "T1_HALT",        "T1_EOF",        "T1_AT",        "T1_INITIAL",
    "T1_DATA",      "T1_IDENTIFIER",  "T1_NUMBER",     "T1_STRING",    "T1_PLUSSIGN",
    "T1_MINUSSIGN", "T1_PLUS",        "T1_MINUS",      "T1_STAR",      "T1_SLASH",
    "T1_MOD",       "T1_COLONEQUALS", "T1_AND",        "T1_OR",        "T1_XOR",
    "T1_NOT",       "T1_LT",          "T1_LE",         "T1_EQ",        "T1_NE",
    "T1_GE",        "T1_GT",          "T1_COMMA",      "T1_LPAREN",    "T1_RPAREN",
    "T1_PERIOD",    "T1_TO",          "T1_BY",         "T1_INVALID",   "T1_MODULE",
    "T1_XREFUSE",   "T1_XREFDEF",     "T1_EXTERNAL"
};

void DumpT1Stream() // to be used after Start1
{
    FILE *fpout;
    int w1, w2, w3;
    sym_t *sym;
    vfile_t *vf  = &utf1;
    uint32_t pos = vf->pos;

    if ((fpout = fopen("t1.dmp", "w")) == NULL) {
        fprintf(stderr, "can't create t1.dmp\n");
        return;
    }
    vfRewind(&utf1);
    while (vf->pos < vf->size) {
        int c = vfRbyte(&utf1);
        if (c > T1_EXTERNAL)
            fprintf(fpout, "Invalid lex item %d\n", c);
        else {
            fprintf(fpout, "%s", t1Items[c]);
            switch (c) {
            case T1_LINEINFO:
                w1 = vfRword(vf);
                w2 = vfRword(vf);
                w3 = vfRword(vf);
                fprintf(fpout, ": line %d, stmt %d, blk %d", w1, w2, w3);
                break;
            case T1_SYNTAXERROR:
                fprintf(fpout, ": %d", vfRword(vf));
                break;
            case T1_TOKENERROR:
                w1  = vfRword(vf);
                sym = &symtab[vfRword(vf)];
                if (sym == NULL)
                    fprintf(fpout, ": %d -NULL-", w1);
                else
                    fprintf(fpout, ": %d %.*s", w1, sym->name->len, sym->name->str);
                break;
            case T1_STRING:
                w1 = vfRword(vf);
                fprintf(fpout, " %d '", w1);
                while (w1--)
                    putc(vfRbyte(vf), fpout);
                putc('\'', fpout);
                break;
            case T1_NUMBER:
            case T1_STMTCNT:
            case T1_LABELDEF:
            case T1_LOCALLABEL:
            case T1_JMP:
            case T1_JMPFALSE:
            case T1_SCOPE:
            case T1_CASELABEL:
                w1 = vfRword(vf);
                if (c != T1_SCOPE)
                    fprintf(fpout, " %d", w1);
                if (c == T1_SCOPE || (c == T1_NUMBER && w1 > 9))
                    fprintf(fpout, " [%04X]", w1);
                break;
            case T1_IDENTIFIER:
                sym = &symtab[vfRword(vf)];
                if (sym == NULL)
                    fprintf(fpout, " -NULL-");
                else
                    fprintf(fpout, " %.*s", sym->name->len, sym->name->str);
                break;
            case T1_AT:
            case T1_INITIAL:
            case T1_DATA:
            case T1_PROCEDURE:
            case T1_XREFUSE:
            case T1_XREFDEF:
            case T1_EXTERNAL:
                w1 = vfRword(vf);
                if (w1 == 0)
                    fprintf(fpout, " -NULL-");
                else {
                    if (c == T1_XREFUSE && FromIdx(w1)->sym)
                        fprintf(fpout, ": %s", FromIdx(w1)->sym->name->str);
                    else
                        showInfo(w1, fpout);
                }
                break;
            case T1_INCLUDE:
                w1 = vfRword(vf);
                fprintf(fpout, "(%s)", includes[w1]);
                break;
            }
            putc('\n', fpout);
        }
    }
    fclose(fpout);
    vf->pos = pos;
}
#endif
