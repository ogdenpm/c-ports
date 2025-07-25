TARGET = ixref
OBJS =	ixref.o ixref2.o ixref3.o table.o wild.o cmdline.o os.o
include ../common.mk

ixref.o: cmdline.h
ixref.o ixref3.o table.o wild.o: os.h
ixref.o ixref2.o ixref3.o table.o: ixref.h 
# common dependencies
ixref2.o cmdline.o os.o: cmdline.h os.h utility.h
