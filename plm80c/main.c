/****************************************************************************
 *  main.c: part of the C port of Intel's ISIS-II plm80                     *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"
// static byte copyRight[] = "[C] 1976, 1977, 1982 INTEL CORP";
char const help[] =
    "Usage: %s inputFile [pl/m-80 option]*)\n"
    "Where the case insensitive pl/m-80 options are: (note * indicates default)\n"
    "CODE             | NOCODE*     Turn on/off listing of generated code\n"
    "COND*            | NOCOND      Turn on/off listing of conditional lines in listing.\n"
    "DATE [(dateString)]            Set date to show in listing header. Default string is today in "
    "yyyy-mm-nn format\n"
    "                               and also the current time in hh:mm:ss format if there is room\n"
    "DEBUG            | NODEBUG*    Include local symbols and line information in object file.\n"
    "EJECT                          Eject listing to next page\n"
    "INCLUDE (file)                 Set an initial include file to read\n"
    "INTVECTOR(interval, location)\n"
    "                 | NOINTVECTOR Interrupt vector for Intel 8259. Default INTVECTOR(8,0)\n"
    "IXREF [(file)]   | NOIXREF*    Controls generation of ixref file. File defaults to "
    "{src}.ixi,\n"
    "LEFTMARGIN (column)            Changes initial column on a line from 1.\n"
    "LIST*            | NOLIST      Turn on/off listing. Default LIST\n"
    "MAKEDEPEND [(file)]            Generate makefile dependencies. File defaults to "
    ".deps/{src}.d\n"
    "OBJECT* [(file)] | NOOBJECT    Object file. Default OBJECT({src}.obj)\n"
    "PAGELENGTH (length)            Change length of page for listing from 60\n"
    "PAGEWIDTH (width)              Change width of page listing from 120\n"
    "PAGING           | NOPAGING*   Listing is paginated.Default NOPAGING\n"
    "PRINT* [(file)]  | NOPRINT     Listing file. File defaults to {src}.lst\n"
    "RESET (switch list)            Set each var in the comma separated list to 0\n"
    "SET (switch list)              Each var [= number] in the list is set to the specified value, "
    "default 0FFH\n"
    "SYMBOLS          | NOSYMBOLS*  Symbol table in listing file.\n"
    "TITLE (titleString)            Optional title to show in listing header.\n"
    "WORKFILES(device, device)      Ignored as no longer needed\n"
    "XREF             | NOXREF*     Cross ref in listing file.\n"
    "See Intel PL/M-80 documentation for more details, other than MAKEDEPEND extension\n"
    "Notes:\n"
    "* {src} is source file name minus any extent\n"
    "* SET and RESET switches are used in conditional compilation expressions\n"
    "* File names are of the format [:Fx:]path, where x is a digit and path\n"
    "  The :Fx: maps to a directory prefix from the same named environment variable\n"
    "* Long lines are supported, as is the Intel '&' line continuation option\n"
    "* Response file input for compiling is supported by using \"%s <file\"\n"
    "* Any object file created is deleted on error, which helps with make builds\n";


void Start() {
    SignOnAndGetSourceName();
    InitKeywordsAndBuiltins();

    Start0(); // lex pass
    if (Start1() == 2) {
        Start2();
        if (Start3() == 4) // only returns if listing/object or XREF needed
            Start4();      // listing/object generation only returns if XREF needed
    } else
        Start6(); // Error module, only returns if XREF needed
    Start5();     // XREF module
}

