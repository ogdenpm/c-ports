/****************************************************************************
 *  rawio.c: part of the C port of tex                             *
 *  The original CP/M application is Digital Research                       *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>


bool usedRawMode = false;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static char const *device[] = { "CON:",  "PRN:",  "AUX:",  "NUL:",  "COM1:", "COM2:",
                                "COM3:", "COM4:", "COM5:", "COM6:", "COM7:", "COM8:",
                                "COM9:", "LPT1:", "LPT2:", "LPT3:", "LPT4:", "LPT5:",
                                "LPT6:", "LPT7:", "LPT8:", "LPT9:", NULL };

bool isdevice(char const *fn) {
    char *s = strrchr(fn, ':');
    if (!s || s[1] != 0)
        return false;
    for (char const **dp = device; *dp; dp++) {
        if (_stricmp(fn, *dp) == 0)
            return true;
    }
    return false;
}

DWORD initOut;
DWORD initIn;

void disable_raw_mode(void) {
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), initIn);
}

void enable_raw_mode(void) {
    HANDLE inHandle  = GetStdHandle(STD_INPUT_HANDLE);

    if (!usedRawMode) {
        GetConsoleMode(inHandle, &initIn);
        usedRawMode = true;
        atexit(disable_raw_mode);
    }

    SetConsoleMode(inHandle, ENABLE_PROCESSED_INPUT);
}

#else
/*
 * on linux assume a file reference of /dev/xxx is a device
 */
bool isdevice(char const *fn) {
    return strncmp(fn, "/dev/", 5) == 0;
}

/*
 *   mini implementation of getch and kbhit
 */
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    if (!usedRawMode) {
        tcgetattr(0, &orig_termios);
        atexit(disable_raw_mode);
        usedRawMode = true;
    }
    struct termios term = orig_termios;
    term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
    tcsetattr(0, TCSAFLUSH, &term);
}

#endif

/* on modern processors the tex is very fast
 * as the OS supports control C to abort
 * the code here always returns no key hit
 */
bool kbhit() {
    return false;
}

int getch() { // this implementation keeps line editing available when not pausing to continue
    enable_raw_mode();
    int c = getchar();
    disable_raw_mode();
    return c;
}