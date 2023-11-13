TARGET = locate
OBJS =	loc1.o  loc2b.o loc3.o loc4.o\
	loc6.o loc7.o cmdline.o lst.o os.o omf.o

include ../common.mk

loc1.o loc2b.o loc3.o loc4.o loc6.o loc7.o : loc.h lst.h omf.h os.h
cmdline.o: cmdline.h os.h
lst.o: cmdline.h lst.h os.h
omf.o: omf.h os.h
os.o: cmdline.h os.h showVersion.h
_version.o: _appinfo.h _version.h


