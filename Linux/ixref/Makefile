TARGET = ixref
OBJS =	ixref.o ixref2.o ixref3.o table.o wild.o cmdline.o os.o
include ../common.mk

ixref.o ixref2.o: cmdline.h
ixref.o ixref2.o ixref3.o table.o wild.o: os.h
ixref.o ixref2.o ixref3.o table.o: ixref.h 
# common dependencies
cmdline.o: cmdline.h os.h
os.o: cmdline.h os.h showVersion.h
_version.o: _appinfo.h _version.h
