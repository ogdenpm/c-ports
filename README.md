# C ports of PL/M 80 and Fortran applications

This repository contains my ports of several PL/M 80 applications to C and my port of the old Fortran based PL/M80 compiler, again to C.

There is also a historical port of the ISIS-II PL/M 80 compiler to C++, directly done from the disassembled code, i.e. before I reversed engineered the compiler into PL/M 80 source.

Originally the code was distributed as part of my [Intel80Tools](https://github.com/ogdenpm/intel80tools) repository, however I have refactored into its own repository as a cleaner option, as the number of ports increases. Prebuilt Windows 32bit binaries of these tools (except the historic C++ port) are still distributed as part of the [Intel80Tools](https://github.com/ogdenpm/intel80tools) repository. 

See the doc directory for information on each of the applications, including usage differences from the original applications.

Visual studio solution files are provided for all the tools,  but most should compile under linux/unix with little or no modification.

Note if you get a warning message when building  files, it may be because you are using an older or possibly newer version than I have set the project to. Visual Studio provides simple options to retarget to any version you have.

There are also a couple of Windows command files from my [versionTools](https://github.com/ogdenpm/versionTools) repository in the Scripts directory. Of these version.cmd is the main one that creates the version numbers and perl replacement can be found in the [versionTools](https://github.com/ogdenpm/versionTools) repository. The other should be replaced with your own installation scripts.

------

```
Updated by Mark Ogden 12-Oct-2020
```
