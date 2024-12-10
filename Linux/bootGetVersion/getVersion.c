// popen.c
/* This program uses _popen and _pclose to receive a
 * stream of text from a system process.
 */
#include <ctype.h>

#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif
#include "support.h"

char const help[] = "Usage:  %s [options] [directory | file]*\n"
                    "Where options are:\n"
                    "  -w      write new version file if version has updated\n"
                    "  -f      force write new version file\n"
                    "  -c file set alternative configuration file instead of version.in\n"
                    "  -d      show debugging information\n"
                    "If no directory or file is specified '.' is assumed\n";

#ifndef _MAX_PATH
#ifdef PATH_MAX
#define _MAX_PATH PATH_MAX
#else
#define _MAX_PATH 260
#endif
#endif

char *dirName;
char *fileName;
char *currentWorkingDir;

char decorate[128]  = "--decorate-refs=tags/appName-r*"; // updated once appName known
char tagPrefix[128] = "tag: appName-r";                  // updated once appName known

char const *logCmd[]      = { "git", "log", "-1", decorate, "--format=g%h,%ct,%D", "--", NULL, NULL };
char const *diffIndexNameCmd[] = { "git", "diff-index", "--name-only", "--ignore-cr-at-eol","HEAD", "--", ".", NULL };
char const *diffIndexCmd[]     = { "git", "diff-index", "--quiet", "--ignore-cr-at-eol", "HEAD", "--", NULL,  NULL };
char const *ignoreCmd[]        = { "git", "check-ignore", NULL, NULL };

int logPathIndex;
int diffIndexPathIndex;
int ignorePathIndex;

string_t exeBuf;

bool quiet;
bool includeUntracked;

int vCol = 14;
char const *appName;
char version[256];
int versionLoc = 0;
char date[20];
char oldVersion[256];
char const *configFile = "version.in";
char *versionFile;

bool noGit; // set to true if git isn't accessible

enum { DIRECTORY, SINGLEFILE, OTHER }; // path types

bool writeFile = false;
bool force     = false;

static bool isPrefix(char const *str, char const *prefix) {
    while (*prefix && *str++ == *prefix++)
        ;
    return *prefix == '\0';
}

static char *scanNumber(char *s, int low, int high) {
    if (!isdigit(*s) || *s == '0')
        return NULL;
    int val = 0;
    while (isdigit(*s))
        val = val * 10 + *s++ - '0';
    if (*s++ == '.' && low <= val && val <= high)
        return s;
    return NULL;
}

static char *scanRevision(char *s) {
    char *t = s;
    if (*t == 'g') {
        while (isxdigit(*t))
            t++;
        return t != s + 1 ? t : NULL;
    }
    if (isdigit(*t) && *t != '0') { // valid for both old and new
        while (isdigit(*t))
            t++;
        if (*t == '-' && isalpha(*++t)) {
            while (isalpha(*t))
                t++;
            if (isdigit(*t) && *t != '0') {
                while (isdigit(*t))
                    t++;
                return t;
            }
        }
        t = s;
    }
    while (isxdigit(*t))
        t++;

    return t != s ? t : NULL;
}

static bool parseVersion(char *line) {

    for (char *start = strchr(line, '2'); start; start = strchr(start + 1, '2')) {
        // scan forward looking for 20\d\d\.1?\d\.[123]?\d\.revision
        char *end;
        if ((end = scanNumber(start, 2000, 2099)) && (end = scanNumber(end, 1, 12)) &&
            (end = scanNumber(end, 1, 31)) && (end = scanRevision(end))) {
            if (*end == '+')
                end++;
            if (*end == '?')
                end++;
            *end = '\0';
            // we have a valid start and end
            strcpy(oldVersion, start);
            return true;
        }
    }

    for (char *start = strchr(line, 'x'); start; start = strchr(start + 1, 'x'))
        if (isPrefix(start, "xxxx.xx.xx.x?")) {
            strcpy(oldVersion, "xxxx.xx.xx.x?");
            return true;
        }
    oldVersion[0] = '\0';
    return false;
}

static void getOldVersion() {
    *oldVersion = '\0';
    FILE *fp = fopen(versionFile, "rt");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            for (char *s = line; *s; s++) {
                char *t = "git_version";
                while (*t && tolower(*s++) == *t++)
                    ;
                if (!*t && parseVersion(line))
                    break;
            }
        }
        fclose(fp);
    }
    if (*oldVersion == '\0') {
        force = true;
        strcpy(oldVersion, "xxxx.xx.xx.x?");
    }
}

static int setupContext(char *path) {
    if (!currentWorkingDir && !(currentWorkingDir = getcwd(NULL, _MAX_PATH)))
        fatal("Cannot determine current directory");

    if (chdir(currentWorkingDir))
        fatal("Cannot chdir to current working directory");

    free(dirName);
    free(fileName);

    struct stat sb;

#ifdef _MSC_VER
    dirName = _fullpath(NULL, path, _MAX_PATH);
#elif defined(linux)
    dirName = realpath(path, NULL);
#endif
    if (dirName == NULL)
        warn("Can't resolve directory/file %s\n", path);
    else if (stat(path, &sb) != 0)
        warn("Cannot access %s", path);
    else if (sb.st_mode & S_IFDIR) {
        if (chdir(path) == 0) {
            fileName = NULL;
            return DIRECTORY;
        }
        warn("Cannot change to directory %s", path);
    } else if (sb.st_mode & S_IFREG) {
        char *fname = (char *)basename(dirName);
        fileName    = safeStrdup(fname);
        *--fname    = '\0'; // remove filename
        if (chdir(dirName) == 0)
            return SINGLEFILE;
        warn("Cannot change to directory containing %s", path);
    } else
        warn("%s is not a directory or file", path);
    return OTHER;
}


// note the % 10000 and % 100 are to stop GCC complaining about possible overrun
static void initAsciiDate() {
    time_t tval;
    time(&tval);
    struct tm *timeInfo = gmtime(&tval);
    sprintf(date, "%04u-%02u-%02u %02u:%02u:%02u", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1,
            timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
}

static char *ctimeVersion(char *timeStr) {
    static char verStr[20];
    time_t tval         = strtoull(timeStr, NULL, 10);

    struct tm *timeInfo = gmtime(&tval);
    sprintf(verStr, "%04u.%u.%u", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1,
            timeInfo->tm_mday);
    return verStr;
}

#if DEBUG
static void show(char **s) {
    while (*s)
        printf("'%s' ", *s++);
    putchar('\n');
}
#endif

static void generateVersion(bool isApp) {
    int lResult = execute(logCmd, &exeBuf, false);
    if (lResult == 0 && exeBuf.str[0]) {
        char *s = strchr(exeBuf.str, '\n'); // remove any trailing new line
        if (s)
            *s = '\0';
        // simple split on comma

        char *pSha1 = exeBuf.str;
        if ((s = strchr(pSha1, ',')))
            *s = '\0';
        char *pCtime = s ? s + 1 : "";
        char rev[128];
        if ((s = strchr(pCtime, ','))) {
            if (isPrefix(++s, tagPrefix)) { // skip comma "tag: " appname "-r"
                s += strlen(tagPrefix);
                if (isdigit(*s) &&
                    !strchr(s, '-')) { // avoid issue with apps named {name} and {name}-r
                    char *t = pSha1 = rev;
                    while (isdigit(*s)) // but '-' between release and qualifier if present
                        *t++ = *s++;
                    if (*s)
                        *t++ = '-';
                    strcpy(t, s);
                }
            }
        }
        sprintf(version, "%s.%s", ctimeVersion(pCtime), pSha1);

        if (isApp) { // for app check don't consider the version file
            if (execute(diffIndexNameCmd, &exeBuf, false) == 0 && exeBuf.str[0]) {
                char *s = strchr(exeBuf.str, '\n'); // get end of first line
                *s++    = '\0';                     // convert to string
                // if there are no more lines and this one has a file name versionFile, then treat
                // as unchanged
                if (*s || stricmp(basename(exeBuf.str), versionFile) != 0)
                    strcat(version, "+");
            }
        } else if (execute(diffIndexCmd, NULL, false) == 1)
            strcat(version, "+");
    } else
        strcpy(version, "Untracked");
}

static void getOneVersion(char *name) {
    int type = setupContext(name);
    if (type == OTHER || strcmp(name, ".git") == 0)
        return;

    appName = basename(dirName);
    if (!*appName)
        appName = "-ROOT-";
    if (logPathIndex == 0) {
        for (char const **p = logCmd; *p; p++)
            logPathIndex++;
        for (char const **p = diffIndexCmd; *p; p++)
            diffIndexPathIndex++;
        for (char const **p = ignoreCmd; *p; p++)
            ignorePathIndex++;
    }
    ignoreCmd[ignorePathIndex] = diffIndexCmd[diffIndexPathIndex] = logCmd[logPathIndex] =
        fileName ? fileName : ".";
    int ignored = -1;
    if (!noGit) {
        ignored = execute(ignoreCmd, &exeBuf, false);
        if (ignored < 0) {
            warn("Git not found");
            noGit = true;
        }
    }
    if ((ignored == 0 || ignored > 1) && !includeUntracked)
        return;

    sprintf(decorate, "--decorate-refs=tags/%s-r*", appName);
    sprintf(tagPrefix, "tag: %s-r", appName);


    if (ignored == 0 || ignored == 128)
        strcpy(version, "Untracked");
    else if (type == DIRECTORY) {
        if (noGit || ignored == 1) {
            free(versionFile);
            versionFile = parseConfig(configFile);
            getOldVersion();
            *version = '\0'; // assume no version
            if (!noGit)
                generateVersion(true);
            if (!*version) {
                strcpy(version, oldVersion);
                if (!strchr(version, '?'))
                    strcat(version, "?");
            }
            if (writeFile && (force || strcmp(version, oldVersion) != 0))
                writeNewVersion(versionFile, version, date);
        }
    } else if (ignored == 1)
        generateVersion(false);
    else if (noGit)
        strcpy(version, "Unknown");

    if (isdigit(*version) || includeUntracked)
        printf("%-*s %c %s\n", vCol, type == DIRECTORY ? appName : name,
               type == DIRECTORY ? '-' : '+', version);
}

int main(int argc, char **argv) {
    while (getopt(argc, argv, "dfwuc:") != EOF) {
        switch (optopt) {
        case 'f':
            force = true;
            // FALLTHROUGH
        case 'w':
            writeFile = true;
            break;
        case 'd':
            debug = true;
            break;
        case 'c':
            configFile = optarg;
            break;
        case 'u':
            includeUntracked = true;
            break;
        }
    }

    initAsciiDate();
    if (optind + 1 >= argc)
        vCol = 1;
    else {
        for (int i = optind; i < argc; i++)
            if (strlen(argv[i]) > vCol)
                vCol = (int)strlen(argv[i]);
    }
    vCol = optind < argc - 1 ? 24 : 1;
    if (optind == argc)
        getOneVersion(".");
    else
        while (optind < argc)
            getOneVersion(argv[optind++]);

    if (currentWorkingDir && chdir(currentWorkingDir))
        ;

}
