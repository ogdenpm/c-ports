#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "utility.h"


static FILE *logFp;

/// <summary>
/// Opens the log file
/// </summary>
/// <param name="name">The name of the log file to create</param>
/// <returns>true if created</returns>
bool openLog(char const *name) {
    if (!(logFp = fopen(name, "wt")))
        warn("Cannot create log file %s", name);
    return logFp != NULL;
}

/// <summary>
/// Closes the log file
/// </summary>
void closeLog() {
    if (logFp)
        fclose(logFp);
    logFp = NULL;
}


/// <summary>
/// shared code to pring out error/warning issue
/// </summary>
/// <param name="fp">The stream to write to</param>
/// <param name="prefix">The prefix string, or NULL</param>
/// <param name="fmt">printf compatible format string</param>
/// <param name="args">The arguments for the format string</param>
static void _msg(char *prefix, char const *fmt, va_list args) {
    if (logFp) {
        if (prefix)
            fputs(prefix, logFp);
        vfprintf(logFp, fmt, args);
        putc('\n', logFp);
    }
    if (prefix)
        fputs(prefix, stderr);
    vfprintf(stderr, fmt, args);
    putc('\n', stderr);
}

    /// <summary>
/// emit Fatal error message with '\n' appended to stderr and exit
/// </summary>
/// <param name="fmt">printf compatible format string</param>
/// <param name="">optional parameters</param>
/// <returns>exits with 1</returns>
_Noreturn void fatal(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _msg("Fatal Error: ", fmt, args);
    va_end(args);
    Exit(1);
}

/// <summary>
/// emit warning message with '\n' appended to stdout
/// </summary>
/// <param name="fmt">printf compatible format string</param>
/// <param name="">optional parameters</param>
void warn(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _msg("Warning: ", fmt, args);
    va_end(args);
}

void logMsg(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (logFp)
        vfprintf(logFp, fmt, args);
    vprintf(fmt, args);
    va_end(args);
}
