TARGET = locate
OBJS =	lst.o os.o loc1.o  loc2b.o loc3.o loc4.o\
	loc6.o loc7.o omf.c

include ../common.mk

loc1.o loc2b.o loc3.o loc4.o loc6.o loc7.o : loc.h os.h omf.h lst.h
lst.o os.o omf.o : lst.h os.h
os.o omf.o: omf.h


