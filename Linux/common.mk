# common.mk: 
# common rules to support Linux make builds

define BOGUS
Summary:
  These rules assumes that Linux is a sub directory of the main repo
  and that its sub directories mirror the
  main repo ones e.g. repo/app/ and repo/Linux/app
  this allows a simple location of the source code

  built files are put in Linux/Install
  real install will be from this directory if required

Makefile:
  Typically the Makefile in the relevant Linux/app directory
  would have the following structure, stared lines are optional
  don't include the - annotation comments.


  TARGET = {target name}
  OBJS = {list of objects to build} 	- exclude the _version.o file
* INCLUDES={optional include dirs} 		- omit if no special include dirs
* LIBS={optional additional libraries} 	- omit if no special libraries
* LINKER=g++							- only include for C++ builds
  include ../common.mk
 
  # object file header dependencies e.g.
  $(OBJS): {include files}


  note to allow INCLUDES to reference files in the source tree
  a letter ^ is mapped to the ROOT directory

Targets:
  five standard targets are supported, with the default being all if not specified
  all		- makes the application and copies to the Install directory
  		  	  due to slow git file access Windows <-> WSL, all does not regenerate
  		  	  the version stamp, unless it is missing.
  		  	  during development this speeds up the build cycle
  clean		- removes .o files
  distclean - removes the target files in addition to the *.o file
  			  it does not delete files in the Install directory
  publish 	- like rebuild but also forces the version stamp to be updated
  rebuild 	- runs distclean and then all
endef

.PHONY: all mkversion clean distclean publish
SRCDIR=$(subst /Linux,,$(realpath .))
ROOT:=$(realpath ../..)
INSTALLDIR = $(ROOT)/Linux/Install

CFLAGS = -O3 -Wall -I$(SRCDIR) -I$(ROOT)/shared $(addprefix -I,$(subst ^,$(ROOT),$(INCLUDES)))
CXXFLAGS = $(CFLAGS)
VPATH = $(SRCDIR):$(ROOT)/shared

LINKER ?= gcc

all: $(TARGET) | $(INSTALLDIR)
	cp -p $(TARGET) $(INSTALLDIR)

publish: distclean mkversion
	$(MAKE)

# check version and force timestamp change so build
# information is updated
mkversion: $(INSTALLDIR)/getVersion
	$(INSTALLDIR)/getVersion -f $(SRCDIR) 

$(INSTALLDIR)/getVersion:
	(cd $(ROOT)/Linux/bootGetVersion; ./mkGetVersion)

_version.o: _version.h _appinfo.h appinfo.h

$(SRCDIR)/_version.h:
	(cd $(SRCDIR); perl $(ROOT)/Scripts/getVersion.pl -W)

$(TARGET): $(OBJS) _version.o $(LIBS)
	$(LINKER) -o $@ $^

clean:
	rm -f *.o

distclean: clean
	rm -f $(TARGET)

rebuild: distclean
	$(MAKE)

$(INSTALLDIR):
	mkdir $@

_version.o: _version.c showVersion.h appinfo.h 
