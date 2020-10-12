/****************************************************************************
 *  plm80: C port of Intel's ISIS-II PLM80 v4.0                             *
 *  Copyright (C) 2020 Mark Ogden <mark.pm.ogden@btinternet.com>            *
 *                                                                          *
 *  This program is free software; you can redistribute it and/or           *
 *  modify it under the terms of the GNU General Public License             *
 *  as published by the Free Software Foundation; either version 2          *
 *  of the License, or (at your option) any later version.                  *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program; if not, write to the Free Software             *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,              *
 *  MA  02110-1301, USA.                                                    *
 *                                                                          *
 ****************************************************************************/


#include "plm.h"
#include <stdio.h>
#include <stdlib.h>
#define LOWMEM  0x613F      // plm lowest value of MEMORY
#define HIGHMEM 0xf7ff

static pointer memory;
offset_t MEMORY;

void initMem()
{
    if ((memory = (pointer)malloc(HIGHMEM - LOWMEM + 1)) == NULL) {
        fprintf(stderr, "can't allocate memory\n");
        exit(2);
    }
}

offset_t MemCk()
{
    return HIGHMEM;
}

offset_t ptr2Off(pointer p)
{
    if (p == 0)     // allow NULL
        return 0;
    if (p < memory || memory + HIGHMEM < p) {
        fprintf(stderr, "out of range memory ref %p\n", p);
        exit(3);
    }
    return (offset_t)(p - memory + LOWMEM);
}

pointer off2Ptr(offset_t off)
{
    if (off == 0)       // allow NULL
        return 0;
    if (off < LOWMEM || HIGHMEM < off) {
        fprintf(stderr, "out of range offset %04X\n", off);
        exit(4);
    }
    return memory + off - LOWMEM;
}