/****************************************************************************
 *  lib.h: part of the C port of Intel's ISIS-II lib             *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "../shared/omf.h"
#include "../shared/os.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct _namelist {
    struct _namelist *next;
    uint8_t seen;
    pstr_t const *name;
} namelist_t;

typedef struct _file {
    struct _file *next;
    char const *name;
    namelist_t *modules;
} file_t;

void ProcessFile(char const *fileName, bool adding, namelist_t *mlist);
void InitLib();
void FinaliseLib(char const *libName);