TARGET = lib
OBJS =	lib.o newlib.o cmdline.o omf.o os.o 
CFLAGS += -MMD
include ../common.mk

lib.o : lib.h cmdline.h omf.h os.h
newlib.o: lib.h
# common dependencies
omf.o: omf.h os.h
cmdline.o os.o: cmdline.h os.h utility.h
os.o: utility.h
