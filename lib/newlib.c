/****************************************************************************
 *  newlib.c: part of the C port of Intel's ISIS-II lib             *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "lib.h"
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#define stricmp strcasecmp
#endif

/*
    The code below is a major rewrite of the original ISIS-II lib utility
    The original had multiple optimisations to minimise disk activity including
    using the names and locations data to read only certain modules.
    Given the general size of OMF85 libraries and modern hardware, the new code scans each library
    requested, copying or skipping record by record as required.
    In the process new names, location and dictionary information is built up using
    module header and public definition information.
    As a by product the new approach detects crc errors in the input files.

*/

extern jmp_buf reset;

typedef struct _module {
    struct _module *next;
    pstr_t name;
} module_t;

module_t *moduleHead;

typedef struct _publist {
    struct _publist *next;
    pstr_t name;
} publist_t;

module_t *curModule;
int moduleCnt;

void libReset(int retCode);
char libTmp[20];

typedef struct {
    char const *label;
    uint32_t len;
    uint32_t size;
    uint8_t *content;
} bigbuf_t;
#define BBCHUNK 2048
bigbuf_t libnam = { "names" };
bigbuf_t libloc = { "locations" };
bigbuf_t libdic = { "dictionary" };

void appendBigBuf(bigbuf_t *p, uint8_t *content, uint32_t len) {
    if (p->len + len >= 0xffff)
        FatalError("Library %s record too large adding %s(%s)", omfInName, curModule->name.str);
    if (p->len + len >= p->size) {
        do {
            p->size += BBCHUNK;
        } while (p->len + len > p->size);
        p->content = xrealloc(p->content, p->size);
    }
    memcpy(p->content + p->len, content, len);
    p->len += len;
}

module_t *newModule(pstr_t const *name) {
    module_t *p;
    for (p = (module_t *)&moduleHead; p->next; p = p->next)
        if (pstrequ(&p->next->name, name))
            FatalError("Duplicate module %.*s", name->len, name->str);
    p->next = xmalloc(sizeof(module_t) + name->len);
    p       = p->next;
    p->next = NULL;
    memcpy(&p->name, name, name->len + 1);
    p->name.str[p->name.len] = '\0'; // make str a C string
    ++moduleCnt;
    appendBigBuf(&libnam, (uint8_t *)name, name->len + 1); // append to names record
    return p;
}

char *pubList;
uint16_t pubListLen;
uint16_t pubListSize;
#define PCHUNK 2048

publist_t *hashTable[128];

void resetLibMem() {
    for (int i = 0; i < 128; i++) {
        publist_t *q;
        for (publist_t *p = hashTable[i]; p; p = q) {
            q = p->next;
            free(p);
        }
        hashTable[i] = NULL;
    }
    for (module_t *q, *p = moduleHead; p; p = q) {
        q = p->next;
        free(p);
    }
    moduleHead = NULL;
}

uint8_t Hash(pstr_t const *pstr) {
    uint8_t i, hash;

    hash = pstr->len;
    for (i = 0; i < pstr->len; i++) {
        hash = ((hash >> 1) | (hash << 7)) ^ pstr->str[i];
    }
    return hash % 128;
} /* Hash */

void addPublic(pstr_t const *name) {
    // check for name in hash table
    uint8_t hash = Hash(name);
    for (publist_t *p = hashTable[hash]; p; p = p->next) {
        if (pstrequ(&p->name, name)) {
            fprintf(stderr, "%s: duplicate symbol %s\n", curModule->name.str, p->name.str);
            libReset(2);
        }
    }
    publist_t *p = xmalloc(sizeof(publist_t) + name->len);
    p->next      = hashTable[hash];
    memcpy(&p->name, name, name->len + 1);
    p->name.str[p->name.len] = '\0'; // make str a C string as well
    hashTable[hash]          = p;
    appendBigBuf(&libdic, (uint8_t *)name, name->len + 1);
}

void CopyModule() {
    uint32_t loc = ftell(omfOutFp);
    uint8_t locbytes[4];
    putWord(locbytes, loc / 128);
    putWord(locbytes + 2, loc % 128);
    appendBigBuf(&libloc, locbytes, 4);
    while (inType != R_MODEND) {
        CopyRecord();
        if (inType == R_PUBDEF) {
            ReadByte(); // segId
            while (inP < inEnd) {
                ReadWord(); // not interested in offset
                pstr_t const *name = ReadName();
                ReadByte(); // 0
                addPublic(name);
            }
        }
        GetRecord();
    }
    CopyRecord(); // include MODEND
    uint8_t zero = 0;
    appendBigBuf(&libdic, &zero, 1);
}

/* process the input file,
    if adding
        Append simple module
        Append selected modules from library or all of library if no modules specified
    if Deleting
        Append all but selected modules from library, the calling routine ensures modules are
   specified

   The function updates the names, location and dictionary information for modules appended.

   Note the code processes all of the modules in a library, even though use of names and
   dictionary could potentially be slightly more optimal. In practice with modern
   hardware and caching the performance difference will be very small and the approach simplifies
   the code.
*/
void ProcessFile(char const *fileName, bool adding, namelist_t *mlist) {
    bool isLib = false;
    openOMFIn(fileName);
    GetRecord();
    if (inType == R_LIBHDR) {
        isLib = true;
        GetRecord();
    }
    if (mlist && !isLib)
        IoError(fileName, "Not a library file");
    if (!isLib || inType != R_LIBNAM) {
        do {
            if (inType != R_MODHDR)
                IoError(fileName, "Expected module header");
            pstr_t const *modName = ReadName();
            uint8_t save          = *inP; // make modName a temporary C string
            *inP                  = '\0';
            bool inc              = !mlist ? true : !adding;
            for (namelist_t *p = mlist; p; p = p->next) {
                if (!p->seen && stricmp(modName->str, p->name->str) == 0) {
                    p->seen = true;
                    inc     = adding;
                    break;
                }
            }
            *inP = save; // revert in case we are copying modhdr
            if (inc) {
                curModule = newModule(modName);
                CopyModule();
            } else {
                while (inType != R_MODEND)
                    GetRecord();
            }
            GetRecord();
        } while (isLib && inType != R_LIBNAM);
    }
    closeOMFIn();
    if (!isLib && inType != R_MODEOF)
        IoError(fileName, "Missing EOF record");
    // check if all requested modules seen
    for (namelist_t *p = mlist; p; p = p->next) {
        if (!p->seen)
            fprintf(stderr, "%s: module %s not found\n", fileName, p->name->str);
    }
}

void libReset(int retCode) {
    // avoid recursive errors
    SetITrap(0);
    resetLibMem();
    longjmp(reset, retCode ? retCode : -1);
}

void InitLib() {
    if (!*libTmp)
        sprintf(libTmp, "_lib_%d", getpid());
    SetITrap(libReset);
    omfOutName = libTmp;
    openOMFOut();
    InitRecord(R_LIBHDR);
    WriteWord(0);
    WriteLocation(10);
    EndRecord();
    libnam.len = 0;
    libloc.len = 0;
    libdic.len = 0;
    moduleCnt  = 0;
}

void FinaliseLib(char const *libName) {
    uint32_t namesLoc = ftell(omfOutFp); // for libhdr fixup
    InitRecord(R_LIBNAM);
    EndUserRecord(libnam.content, libnam.len);
    InitRecord(R_LIBLOC);
    EndUserRecord(libloc.content, libloc.len);
    InitRecord(R_LIBDIC);
    EndUserRecord(libdic.content, libdic.len);
    InitRecord(R_MODEOF);
    EndRecord();
    rewind(omfOutFp);
    InitRecord(R_LIBHDR);
    WriteWord(moduleCnt);
    WriteLocation(namesLoc);
    EndRecord();
    closeOMFOut();
    if (Access(libName, 0) == 0 && Delete(libName))
        IoError(libName, "Delete failed");
    if (Rename(libTmp, libName))
        IoError(libName, "Renaming library tmp file failed");
    resetLibMem();
    SetITrap(0);
}