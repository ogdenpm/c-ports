TARGET = link
OBJS =	link3a.o link3.o link.o linkov.o cmdline.o lst.o omf.o os.o

include ../common.mk
link3a.o link3.o link.o linkov.o : link.h  os.h omf.h lst.h
lst.o: lst.h os.h
os.o omf.o: os.h omf.h 
# common dependencies
cmdline.o: cmdline.h os.h
lst.o: cmdline.h lst.h os.h
omf.o: omf.h os.h
os.o: cmdline.h os.h showVersion.h
_version.o: _appinfo.h _version.h

