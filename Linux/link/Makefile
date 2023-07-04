TARGET = link
OBJS =	lst.o os.o link3a.o link3.o link.o linkov.o omf.o

include ../common.mk
link3a.o link3.o link.o linkov.o : link.h  os.h omf.h lst.h
lst.o: lst.h os.h
os.o omf.o: os.h omf.h 

