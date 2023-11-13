/****************************************************************************
 *  vfile.h: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct vfile {
    uint32_t capacity;
    uint32_t size;
    uint32_t pos;
    uint8_t *content;
} vfile_t;

void vfReset(vfile_t *vf);
void vfWbuf(vfile_t *vf, void const *buf, uint32_t len);
void vfWbyte(vfile_t *vf, uint8_t val);
void vfWword(vfile_t *vf, uint16_t val);
void vfRewind(vfile_t *vf);
uint32_t vfRbuf(vfile_t *vf, void *buf, uint16_t len);
int32_t vfRbyte(vfile_t *vf);
int32_t vfRword(vfile_t *vf);
void dump(vfile_t *vf, char const *fname);
