TARGET = link
OBJS =	link3a.o link3.o link.o linkov.o cmdline.o lst.o omf.o os.o

include ../common.mk
link3a.o link3.o link.o linkov.o : link.h  os.h omf.h lst.h
lst.o: lst.h os.h
os.o omf.o: os.h omf.h 
# common dependencies
lst.o: cmdline.h lst.h os.h
omf.o: omf.h os.h
cmdline.o os.o link3a.o: cmdline.h os.h utility.h
link.o link3.o link3a.o linkov.o: link.h lst.h omf.h os.h
