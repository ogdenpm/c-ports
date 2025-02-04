SHELL=bash
.PHONY: clean distclean rebuild
DIRS = asm80 binobj hexobj ixref l81 l82 l83 lib link locate ml80 objhex plm80 plm80c plm81 plm82 tex
all:
	@for f in $(DIRS); do $(MAKE) -C $$f || exit 1; done;

publish:
	@for f in $(DIRS); do $(MAKE) -C $$f publish || exit 1; done;

clean: 
	@for f in $(LIBS) $(DIRS); do $(MAKE) -C $$f clean || exit 1; done;

distclean: 
	@for f in $(LIBS) $(DIRS); do $(MAKE) -C $$f distclean || exit 1; done;
	rm -fr Install/*

rebuild: 
	@for f in $(DIRS); do $(MAKE) -C $$f rebuild || exit 1; done;

