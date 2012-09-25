DIRS = assignment/ blocks/ decl_parsing/ inline/ lib/ structs/ type_parsing/ other/
MAKEFILES = ${DIRS:/=/Makefile}

test: ucc makefiles ${DIRS:/=/test}
	@echo All Tests + Purposeful Failures Succeeded

ucc:
	@cd ../src && make

makefiles: ${MAKEFILES}
clean: makefiles ${DIRS:/=/clean}

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

.PHONY: all clean test gen ucc
