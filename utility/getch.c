/*
    POSIX implementation of Windows _getch function
*/
#ifndef _MSC_VER
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int _getch(void) {

    unsigned char buf        = 0;
    struct termios old = { 0 };
    fflush(stdout);
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~(ICANON | ECHO);
    old.c_cc[VMIN]  = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON | ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
}
#endif