The notes below capture the key changes made as part of the port of the PLM81 code from Fortran 66 to modern C.

Whilst more changes may be made, the current changes represent a stable release

# External Impacting changes

The following are the key externally impacting changes, hopefully most people will not have any negative impact

- The file to compile is now specified as a command line option and intermediate files are generated from its name. Additionally the command line can specify key control options, rather than relying on $ control lines

  ```
  Usage : plm81 [-a] [-b] [-l nn] [-g] [-m] [-p] [-s nn] [-t n] [-u] [-w nn] srcfile
  Where
  -a       debug: show production analysis
  -b       debug: show stack dump
  -l nn    set line count - default 0 i.e. first line is 1
  -g       debug: show generated intermediate code
  -m       turn off dump symbol table
  -p       turn off source code listing
  -s n     debug: show symbol information 0 (default) none, 1 symbols, 2 symbols & data
  -t n     set tab expansion 1 <= n <= 8. Expand tabs with spaces so column is a multiple of n
  -u       force upper case in strings
  -w nn    set output line width (min 72) - default 132
  srcfile  is the source file, of the form prefix.ext e.g. m80.plm
           intermediate files prefix.lst, prefix.pol, prefix.sym are created
           if the srcfile does not have an extension, .plm is added
  
  Also supports single arguments -v, -V for version info and -h for help
  ```

- Support for the $ control lines has been modified to align with the options above.
  - Intermediate file options J, K, U, V, W are no longer needed for their original purpose as the intermediate file format has changed.
  - Support for Left Margin and Right Margin has been removed, so L and R are no longer used for their original purpose. To use very old PLM decks, external tools will need to apply the left/right margin clipping.
    Note the line length limit is now 256 characters. Strings can continue across lines and the '\n' is ignored, otherwise the '\n' terminates a token. Identifiers are truncated to 32 significant characters.
  - The input file is now replaced by the source file command line option and when the control $I is used it now takes a file name, without spaces and opens this as an include file, e.g. I=myincludefile.plm.
  - Using stdin as an input is no longer supported implicitly. The corresponding T option is no longer used for its original purpose.
  - The cryptic C and D options now use the freed L and W options.
  - The control options to list current control values are no longer supported.
  - The input file now supports ascii characters 0x00-0xff, however
    - '\0' and '\r' are ignored (useful when compiling under unix/linux with \r\n source files)
    - '\n' is used as a line terminator
    - '\t' is expanded with spaces. The -t/$T option value is used add spaces to the next column which is a multiple of the $T value. By default $T=1 which mimics the original behaviour of a single space.
      Warning it is not recommended to use control characters or characters with the top bit set inside a PL/M string.
    - As lower case letters are now supported, to retain compatibility with older code, the -u/$U option can be used to map lower case to upper case characters
- Error messages are now descriptive text.
- A fatal error terminates without trying to process any further. The original recover hacks were not reliable and could generate garbage.
  Note A 100 error threshold will also trigger a fatal error.

# Intermediate File Changes

The pol file is now a binary file.

The sym file is still ascii for now, but contains no line breaks.

On error the intermediate .pol and .sym files are deleted as they are not usable.

# Major Internal Changes

There are two major areas of change in the PL/M code

- File I/O

  - The Fortran unit numbering for files is replaced by file names.
  - The internal character set mapping is no longer used. It now assumes ascii chars. Related packed string arrays have been converted to C strings.
  - The handling of line input and $ control line processing has been simplified.
  - In most cases the W option is handled explicitly with long input lines being wrapped as needed. The only area this may break is for very long error messages which are not wrapped.
    Note. The space used for Fortran line printer control is no longer emitted.
  - Scanning of tokens has been simplified.

- Symbol Table
  This was a major structural change as the symbol table was used in multiple ways. Additionally the indexing in to the table was inconsistent making it hard to understand. The replacement code uses a single structure for each symbol, with an adjunct table for the symbol names to avoid a variable size structure. 

  Items that built down from the top of the original symbol array have been moved, specifically

  - Literally definitions, also uses the symbol table string array
  - Initial Data
  - Assignment lists
  - Identifier lists

  Some of the debug information has been modified to reflect the new symbol table.



```
Updated: 14-Jul-2025
```

