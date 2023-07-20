#include "vfile.h"
#include "os.h"
#include <string.h>
#define VFCHUNK 4096
static void vfExpand(vfile_t *vf, uint32_t minSize) {
    if (vf->capacity >= minSize)
        return;
    while (vf->capacity < minSize)
        vf->capacity += VFCHUNK;
    vf->content = xrealloc(vf->content, vf->capacity);
}


void vfReset(vfile_t *vf) {
    vf->size = 0;
    vf->pos = 0;
}



void vfWbuf(vfile_t *vf, uint8_t const *buf, uint32_t len) {
    if (vf->pos + len >= vf->capacity)
        vfExpand(vf, vf->pos + len);
    memcpy(vf->content + vf->pos, buf, len);
    vf->pos += len;
    if (vf->pos > vf->size)
        vf->size = vf->pos;
}

void vfWbyte(vfile_t *vf, uint8_t val) {
    if (vf->pos + 1 >= vf->capacity)
        vfExpand(vf, vf->pos + 1);
    vf->content[vf->pos++] = val;
    if (vf->pos > vf->size)
        vf->size = vf->pos;
}

void vfWword(vfile_t *vf, uint16_t val) {
    if (vf->pos + 2 >= vf->capacity)
        vfExpand(vf, vf->pos + 2);
    vf->content[vf->pos++] = val % 256;
    vf->content[vf->pos++] = val / 256;
    if (vf->pos > vf->size)
        vf->size = vf->pos;
}

void vfRewind(vfile_t *vf) {
    vf->pos = 0;
}

uint32_t vfRbuf(vfile_t *vf, uint8_t *buf, uint32_t len) {
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
    if (vf->pos + 1 >= vf->size)
        return -1;
    uint16_t val = vf->content[vf->pos++];
    return vf->content[vf->pos++] * 256 + val;
}

void dump(vfile_t *vf, char const *fname) {
    FILE *fp = fopen(fname, "wb");
    if (vf->size)
        fwrite(vf->content, 1, vf->size, fp);
    fclose(fp);

}