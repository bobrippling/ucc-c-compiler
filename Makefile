all: src

src:
	${MAKE} -C src

lib: lib/config.mk
	${MAKE} -C lib

deps:
	${MAKE} -Csrc deps

clean:
	${MAKE} -C src clean
	${MAKE} -C lib clean

cleanall: clean
	./configure clean

# see also check-bootstrap
check: all lib
	cd test && ./run_tests -i ignores -j4 cases

ALL_SRC = $(shell find src -iname '*.[ch]')

tags: ${ALL_SRC}
	ctags '--exclude=_*' -R src lib

include Bootstrap.mk

.PHONY: all clean cleanall src
