#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define ENDMARKER         0x81
#define R_MOD             0xFF
#define R_PUBLIC          0
#define R_EXTERNAL        1
#define T_LABEL           1
#define T_BYTE            2
#define T_ADDRESS         3
#define T_STRUCTURE       4
#define T_PROCEDURE       5
#define T_WORD            6
#define T_INTEGER         7
#define T_REAL            8
#define T_POINTER         9




typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t data[68];
} record_t;

typedef struct {
    uint8_t len;
    char str[0];
} pstr_t;

typedef struct {
    char const *name;
    char const *fname;
} mod_t;


typedef struct _xref {
    struct _xref *next;
    char *module;
    bool isDef;
    uint8_t type;
    uint16_t dimRtype;
} xref_t;

typedef struct _sym {
    struct _sym *next;
    char const *name;
    xref_t *xrefList;
} sym_t;



/* ixref.plm */
extern char *outFileName;
extern char *tmpModnm;
extern char *tmpIxin;
extern char *tmpIxout;
extern FILE *tmpModnmFp;
extern record_t modRecord;
extern FILE *inFp;
extern record_t inRec;

extern FILE *tmpInFp;
extern record_t tmpInRec;
extern FILE *outFp;
extern FILE *tmpOutFp;
extern bool havePrint;
extern bool showPublics;
extern bool showExternals;
extern char *endFiles;
extern char *startFiles;
extern uint8_t outCol;
extern uint8_t maxIdLen;



void OutNewPage();
void OutNewLine();
void OutNNewLines(int n);
void OutPstr(char const *str, int len);
void OutStr(char const *s);
void OutPrintf(char const *fmt, ...);
void InitPrint();
void ParseCmdLine();
void Signon();

/* ixref2.plm */
int CmpRecords(record_t *rec1, record_t *rec2);
void GetRecord(FILE *fp, record_t *rec);
void MergeSortRecords();
void collectRecords();

/* ixref3.plm */
void ListModules();
void sub5317();

void ExpectLP(void);
void ExpectRP(void);

char const *newmod(record_t const *rec);
extern int modCnt;
extern mod_t *modtab;
extern sym_t *symlist;