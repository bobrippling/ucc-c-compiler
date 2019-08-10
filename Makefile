all: src

src:
	make -C src

lib: lib/config.mk
	make -C lib

deps:
	make -Csrc deps

clean:
	make -C src clean
	make -C lib clean

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
