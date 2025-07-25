/****************************************************************************
 *  wild.c: part of the C port of Intel's ISIS-II ixref                     *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <dirent.h>
#include <libgen.h>
#endif

#ifdef _MSC_VER
char *expandPath(char const *token) {
    static char const *lastToken;
    static intptr_t hfile = -1;
    struct _finddata_t file;
    while (1) {
        if (token != lastToken) {
            if (hfile != -1)
                _findclose(hfile);
            lastToken = token;
            hfile     = _findfirst(token, &file);
        } else if (hfile != -1) {
            if (_findnext(hfile, &file) != 0) {
                _findclose(hfile);
                hfile = -1;
            }
        }
        if (hfile == -1)
            return NULL;
        if (strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0)
            return safeStrdup(file.name);
    }
}

#else

bool match(char *regexp, char *text);
bool matchstar(char *regexp, char *text);

char *expandPath(char const *token) {
    static char const *lastToken;
    static DIR *dir;
    static char *path;
    static char *pattern;
    if (token != lastToken) {
        if (dir) {
            closedir(dir);
            dir = NULL;
        }
        if (path) {
            free(path);
            free(pattern);
        }
        lastToken = token;
        path      = safeStrdup(token);
        char *s   = (char *)basename(path);
        if (!strpbrk(s, "?*")) {
            pattern = NULL;
            return path;
        } else if (s == path) {
            pattern = path;
            path    = strdup(".");
        } else {
            pattern = strdup(s);
            *s      = '\0';
        }
        dir = opendir(path);
    }

    struct dirent *entry;
    while (dir) {
        if ((entry = readdir(dir))) {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") &&
                match(pattern, entry->d_name))
                return safeStrdup(entry->d_name);
        } else {
            closedir(dir);
            dir = NULL;
        }
    }
    if (path) {
        free(pattern);
        free(path);
        path = NULL;
    }
    return NULL;
}

/* match: search for glob match */
bool match(char *regexp, char *text) {
    if (*regexp == '\0')
        return *text == '\0';
    if (*regexp == '*')
        return matchstar(regexp + 1, text);

    if ((*text != '\0' && *regexp == *text) || *regexp == '?')
        return match(regexp + 1, text + 1);
    return false;
}

/* matchstar: */
bool matchstar(char *regexp, char *text) {
    do { /* a * matches zero or more instances */
        if (match(regexp, text))
            return true;
    } while (*text++ != '\0');
    return false;
}
#endif