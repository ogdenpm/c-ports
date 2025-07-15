/****************************************************************************
 *  vfile.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "vfile.h"
#include "os.h"
#include <string.h>
#define VFCHUNK 4096
static void vfExpand(vfile_t *vf, uint32_t minSize) {   // grow vfile if needed
    if (vf->capacity >= minSize)
        return;
    while (vf->capacity < minSize)
        vf->capacity += VFCHUNK;
    vf->content = safeRealloc(vf->content, vf->capacity);
}

void vfReset(vfile_t *vf) { // effectively delete and open
    vf->size = 0;
    vf->pos  = 0;
}

void vfWbuf(vfile_t *vf, void const *buf, uint32_t len) {
    vfExpand(vf, vf->pos + len);
    memcpy(vf->content + vf->pos, buf, len);
    vf->pos += len;
    if (vf->pos > vf->size)
        vf->size = vf->pos;
}

void vfWbyte(vfile_t *vf, uint8_t val) {
    vfExpand(vf, vf->pos + 1);
    vf->content[vf->pos++] = val;
    if (vf->pos > vf->size)
        vf->size = vf->pos;
}

void vfWword(vfile_t *vf, uint16_t val) {
    vfExpand(vf, vf->pos + sizeof(val));
    *(uint16_t *)(&vf->content[vf->pos]) = val;
    vf->pos += sizeof(val);
    if (vf->pos > vf->size)
        vf->size = vf->pos;
}

void vfRewind(vfile_t *vf) {
    vf->pos = 0;
}

uint32_t vfRbuf(vfile_t *vf, void *buf, uint16_t len) {
    if (vf->pos + len >= vf->size)
        len = vf->size - vf->pos;
    memcpy(buf, vf->content + vf->pos, len);
    vf->pos += len;
    return len;
}

int32_t vfRbyte(vfile_t *vf) {
    if (vf->pos >= vf->size)
        return -1;
    return vf->content[vf->pos++];
}

int32_t vfRword(vfile_t *vf) {
    uint16_t *p = (uint16_t *)(&vf->content[vf->pos]);
    if ((vf->pos += sizeof(uint16_t)) <= vf->size)
        return *p;
    vf->pos = vf->size;
    return -1;
 }

// dump vfile to real file for diagnostics

void dump(vfile_t *vf, char const *fname) {
#ifdef _DEBUG
    FILE *fp = fopen(fname, "wb");
    if (vf->size)
        fwrite(vf->content, 1, vf->size, fp);
    fclose(fp);
#endif
}
