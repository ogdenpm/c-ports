#include "utility.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
/// <summary>
/// Simple implementation of basename for Windows
/// although UNC paths are allowed, paths with a volume only may not return sensible values
/// </summary>
/// <param name="path">full path to file</param>
/// <returns>pointer filename only, in the path</returns>

char *basename(char *path) {
    char *s;

    if ((s = strrchr(path, '/')))
        path = s + 1;
    if ((s = strrchr(path, '\\')))
        path = s + 1;
    if ((s = strrchr(path, ':')))
        path = s + 1;
    return path;
}

#endif

/// <summary>
/// generated a new file name by replacing/adding the ext
/// unless force is false and an ext exists already
/// the file is allocated via safeMalloc and must be freed by the caller
/// </summary>
/// <param name="path">base filename including path</param>
/// <param name="ext">new extent including '.' to append</param>
/// <param name="force">if true will always replace/add ext, else only add if no existing
/// ext</param> <returns>created file</returns>

char *makeFilename(char const *path, char const *ext, bool force) {
    // find the extension in the file name part of path itself: POSIX basename() may return
    // a pointer to its own storage (it does on macOS), which made prefixLen meaningless
    char const *name = path;
    for (char const *p = path; *p; p++)
        if (*p == '/' || *p == '\\' || *p == ':')
            name = p + 1;
    char *s = strrchr(name, '.');
    if (s && !force) // keep existing ext
        ext = s;
    int prefixLen = (s ? (int)(s - path) : (int)strlen(path));
    char *newFile = safeMalloc(prefixLen + strlen(ext) + 1);
    strncpy(newFile, path, prefixLen);
    strcpy(newFile + prefixLen, ext);
    return newFile;
}