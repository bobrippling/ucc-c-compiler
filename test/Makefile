DIRS = assignment/ blocks/ decl_parsing/ inline/ lib/ structs/ type_parsing/ \
			 other/ pointers/ types/ arrays/ stack_save/ lvalue/                   \
			 initialisation/array/ initialisation/scalar/ initialisation/structs/

MAKEFILES = ${DIRS:/=/Makefile}

# we assume ucc/cc1/cpp are up to date
test: ${DIRS:/=/test}
	@echo All Tests + Purposeful Failures Succeeded

makefiles: ${MAKEFILES}
clean: makefiles ${DIRS:/=/clean}

${MAKEFILES}: make_gen.pl

help:
	@echo "make dir/test for individual tests"
	@echo "     dir/clean for cleaning"
	@echo "     makefiles for updated rules"

%/test: %/Makefile
	make -C$* test

%/Makefile: %/TestRules
	perl ./make_gen.pl $* > $*/Makefile

%/clean: %/TestRules
	make -C$* clean
	rm -f $*/Makefile

%/clean:
	make -C$* clean

.PHONY: all clean test gen ucc help
