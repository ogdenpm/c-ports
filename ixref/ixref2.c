/****************************************************************************
 *  ixref2.c: part of the C port of Intel's ISIS-II ixref                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "ixref.h"
#include "os.h"
#include "cmdline.h"
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#ifdef _MSC_VER
#include <io.h>
#endif
char *path; // current file
sym_t *symlist;

#ifndef min
#define min(a, b) ((a) <= (b) ? (a) : (b))
#endif

int pstrcmp(pstr_t *ps1, pstr_t *ps2) {
    int cmp = strncmp(ps1->str, ps2->str, min(ps1->len, ps2->len));
    return cmp ? cmp : ps1->len - ps2->len;
}

int CmpRecords(record_t *rec1, record_t *rec2) {
    if (rec1->type == ENDMARKER)
        return 1;
    if (rec2->type == ENDMARKER)
        return -1;
    int cmp = pstrcmp((pstr_t *)rec1->data, (pstr_t *)rec2->data);
    return cmp ? cmp : rec1->type - rec2->type;
}

void GetRecord(FILE *fp, record_t *rec) {
    /* start of record */
    int ch    = getc(fp);
    rec->type = ch != EOF ? ch : ENDMARKER;

    if (ch != R_MOD && ch != R_PUBLIC && ch != R_EXTERNAL)
        return;
    ch = getc(fp);
    if (ch > 70)
        ch = 70;
    rec->len = ch;
    for (int i = 0; i < ch - 2; i++)
        rec->data[i] = getc(fp);
    if (rec->data[0] > maxIdLen)
        maxIdLen = rec->data[0];
    if (maxIdLen > 31)
        maxIdLen = 31;
}

void newXref(sym_t *ps, char const *module, record_t *rec) {
    xref_t *px;
    int cmp;
    for (px = (xref_t *)&ps->xrefList; px->next && (cmp = strcmp(module, px->next->module));
         px = px->next)
        ;
    xref_t *newXref   = safeMalloc(sizeof(xref_t));
    newXref->next     = px->next;
    px->next          = newXref;
    newXref->module   = module;
    newXref->isDef    = rec->type == R_PUBLIC;
    newXref->type     = rec->data[rec->data[0] + 1];
    newXref->dimRtype = rec->data[rec->data[0] + 2];
    if (newXref->type == T_PROCEDURE)
        newXref->dimRtype += rec->data[rec->data[0] + 3] * 256;
}

static void AddIxi() {
    char const *module;

    GetRecord(inFp, &inRec);
    if (inRec.type != R_MOD || feof(inFp))
        printf(" %s, BAD RECORD TYPE\n", path);
    else {
        module   = newmod(&inRec);
        sym_t *p = (sym_t *)&symlist;
        for (;;) {
            GetRecord(inFp, &inRec);
            if (inRec.type == R_LONGNAME) {
                updateFname(inFp);
                inRec.type = ENDMARKER;
            }
            if (inRec.type != ENDMARKER && inRec.type != R_PUBLIC && inRec.type != R_EXTERNAL) {
                printf(" %s, BAD RECORD TYPE\n", path);
                inRec.type = ENDMARKER;
            }
            if (inRec.type == ENDMARKER)
                return;

            char symname[33];
            int len = min(inRec.data[0], 32);
            memcpy(symname, inRec.data + 1, len);
            symname[len] = '\0'; // now a C string
            if (len > maxIdLen)
                maxIdLen = len;
            int cmp;
            sym_t *newSym;
            while (p->next && (cmp = strcmp(symname, p->next->name)) > 0)
                p = p->next;
            if (!p->next || cmp != 0) { // new entry
                newSym           = safeMalloc(sizeof(sym_t));
                newSym->next     = p->next;
                p->next          = newSym;
                newSym->name     = safeStrdup(symname);
                newSym->xrefList = NULL;
            }
            newXref(p->next, module, &inRec);
        }
    }
}

void collectRecords() {
    cmdP = startFiles; // reset to scan fils

    while (cmdP < endFiles) {
        char *token    = GetToken();
        bool foundFile = false;
        while ((path = expandPath(token))) {
            foundFile = true;
            /* if no explicit print file use the first file replacing ext by 'ixo' */
            if (!havePrint) {
                char const *fname = basename(path);
                char const *s     = strrchr(fname, '.');
                if (!s || s == fname)
                    s = strchr(fname, '\0');
                outFileName = safeMalloc(s - path + 4);
                memcpy(outFileName, fname, s - path);
                strcpy(outFileName + (s - path), ".ixo");
                InitPrint();
                havePrint = true;
            }

            if (!(inFp = Fopen(path, "rb")))
                IoError(path, "cannot open");
            AddIxi();
            fclose(inFp);
            free(path);
        }

        if (!foundFile)
            fprintf(stderr, "Warning: %s, does not match any files\n", token);

        if (*cmdP == ',')
            cmdP++;
    }
}