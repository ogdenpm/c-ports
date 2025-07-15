#include <stdlib.h>

// if user hasn't supplied an exit handler
// use the default
_Noreturn void Exit(int exitCode) {
    exit(exitCode);
}