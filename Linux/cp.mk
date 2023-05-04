.PHONY: all mkversion clean distclean 

SRCDIR=$(subst /Linux,,$(realpath .))
ROOT:=$(realpath ../..)
INSTALLDIR = $(ROOT)/Linux/Install

#CFLAGS = -O3 -Wall -I$(SRCDIR) -I$(ROOT)/shared -D_DEBUG
CFLAGS = -O3 -Wall -I$(SRCDIR) -I$(ROOT)/shared
CXXFLAGS = -O3 -Wall -I$(SRCDIR) -I$(ROOT)/shared
VPATH = $(SRCDIR):$(ROOT)/shared


all: $(TARGET) | $(INSTALLDIR)
	cp -p $(TARGET) $(INSTALLDIR)

publish: distclean mkversion
	$(MAKE) 

# check version and force timestamp change so build
# information is updated
mkversion:
	(cd $(SRCDIR); perl $(ROOT)/Scripts/getVersion.pl -W)

_version.o: _version.h _appinfo.h appinfo.h

$(SRCDIR)/_version.h:
	(cd $(SRCDIR); perl $(ROOT)/Scripts/getVersion.pl -W)

$(TARGET): $(OBJS) _version.o 
ifndef CPLUSPLUS
	gcc -o $@ $^
else
	g++ -o $@ $^
endif

$(ROOT)/linux/Install:
	mkdir $@
clean:
	rm -f *.o 

distclean: clean
	rm -f $(TARGET)

rebuild: distclean
	$(MAKE)

