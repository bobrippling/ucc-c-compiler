DIRS = assignment/ blocks/ decl_parsing/ inline/ lib/ structs/ type_parsing/
MAKEFILES = ${DIRS:/=/Makefile}

test: makefiles ${DIRS:/=/test}
makefiles: ${MAKEFILES}
clean: ${DIRS:/=/clean}

${MAKEFILES}: make_gen.pl

%/test:
	make -C$* test

%/Makefile: %/TestRules
	cd $*; perl ../make_gen.pl > Makefile

%/clean: %/TestRules
	make -C$* clean
	rm -f $*/Makefile

%/clean:
	make -C$* clean

.PHONY: all clean test gen
