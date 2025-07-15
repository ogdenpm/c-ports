#include <stdlib.h>
#include "utility.h"
/// <summary>
/// helper to malloc memory and check for out of memory
/// </summary>
/// <param name="size">Amount of memory to allocate in bytes.</param>
/// <returns>pointer to the allocated block</returns>
void *safeMalloc(size_t size) {
    void *p = malloc(size);
    if (!p)
        fatal("Out of memory");
    return p;
}


/// <summary>
/// helper to relloc memory and check for out of memory
/// </summary>
/// <param name="old">The old memory block</param>
/// <param name="size">The new size</param>
/// <returns></returns>
void *safeRealloc(void *old, size_t size) {
    void *p = realloc(old, size);
    if (!p)
        fatal("Out of memory");
    return p;
}