TARGET = asm80
APPOBJS =	asm1m.o asm2m.o asm3m.o asm4m.o asm5m.o asm6m.o asm8m.o\
		asxref.o cntrlm.o debug.o emitm.o globlm.o initm.o\
		nkeym.o listm.o memory.o rdsrc.o startm.o
OBJS = $(APPOBJS) cmdline.o os.o
include ../common.mk

$(APPOBJS): asm80.h data.h error.h literals.h plm80types.h procs.h
cntrlm.o emitm.o initm.o memory.o rdsrc.o startm.o: os.h
listm.o startm.o: cmdline.h

# common dependencies
cmdline.o: cmdline.h os.h
os.o: cmdline.h os.h showVersion.h
_version.o: _appinfo.h _version.h
