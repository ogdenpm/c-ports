TARGET = plm80c
APPOBJS =	data.o endcom.o fatal.o fi.o lookup.o\
	lstsup.o main.o main0.o main1.o main2.o main3.o main4.o main5.o\
	main6.o page.o pdata4.o plm0a.o plm0b.o plm0e.o plm0f.o plm0g.o\
	plm1a.o plm1b.o plm1c.o plm1d.o plm1e.o plm1f.o plm2a.o plm2b.o plm2c.o plm2d.o\
	plm2e.o plm2f.o plm2g.o plm2h.o plm3a.o plm3b.o plm4a.o plm4b.o plm4c.o\
	plma.o plmfile.o prints.o symtab.o vfile.o
OBJS = $(APPOBJS) cmdline.o os.o

include ../common.mk

$(APPOBJS) : errors.h int.h plm.h vfile.h
fatal.o main.o main0.o main1.o main3.o main4.o main5.o main6.o plm0b.o plm0g.o\
plm1f.o plm2a.o plm2e.o plm3b.o plm4b.o plm4c.o plma.o plmfile.o symtab.o vfile.o: os.h
main4.o main6.o plma.o: cmdline.h
# common dependencies
cmdline.o: cmdline.h os.h
os.o: cmdline.h os.h showVersion.h
_version.o: _appinfo.h _version.h
