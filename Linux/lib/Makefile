TARGET = lib
OBJS =	lib.o newlib.o cmdline.o omf.o os.o 
CFLAGS += -MMD
include ../common.mk

lib.o : lib.h cmdline.h omf.h os.h
newlib.o: lib.h
# common dependencies
cmdline.o: cmdline.h os.h
omf.o: omf.h os.h
os.o: cmdline.h os.h showVersion.h
_version.o: _appinfo.h _version.h
