#include "utility.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
/// <summary>
/// Equivalent to POSIX basename for visual C
/// </summary>
/// <param name="path">full path to file</param>
/// <returns>pointer filename only</returns>

char const *basename(char const *path) {
    char *s;
#ifdef _WIN32
    if (path[0] && path[1] == ':') // skip leading device
        path += 2;
#ifdef UNC
    else if (path[0] == '\\' && path[1] == '\\') // skip \\computer\share\ part
        if ((s = strchr(path + 2, '\\')) && (s = strchr(s, '\\')))
            path = s + 1;
        else
            path = strchr(path, '\0');
#endif
#endif
    if ((s = strrchr(path, '/')))
        path = s + 1;
#if _WIN32
    if ((s = strrchr(path, '\\')))
        path = s + 1;
#endif
    return path;
}

#endif

/// <summary>
/// generated a new file name by replacing/adding the ext
/// unless force is false and an ext exists already
/// </summary>
/// <param name="path">base filename including path</param>
/// <param name="ext">new extent including '.' to append</param>
/// <param name="force">if true will always replace/add ext, else only add if no existing
/// ext</param> <returns> created file, note will be freed at next call</returns>
static char *newFile;

char *makeFilename(char const *path, char const *ext, bool force) {
    char *s = strrchr(basename(path), '.');
    if (s && !force) // keep existing ext
        ext = s;
    int prefixLen = (s ? (int)(s - path) : (int)strlen(path));
    free(newFile);
    newFile = safeMalloc(prefixLen + strlen(ext) + 1);
    strncpy(newFile, path, prefixLen);
    strcpy(newFile + prefixLen, ext);
    return newFile;
}