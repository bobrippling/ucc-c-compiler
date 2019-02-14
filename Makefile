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

cleantest:
	make -Ctest clean
# no need to clean test2

check: all lib
	cd test2; ./run_tests -q -i ignores .
	# test/ pending

check-bootstrap: bootstrap
	cd test2; ./run_tests -q -i ignores --ucc=../bootstrap/stage3/src/ucc/ucc .

ALL_SRC = $(shell find . -iname '*.[ch]')

tags: ${ALL_SRC}
	ctags '--exclude=_*' -R .

include Bootstrap.mk

.PHONY: all clean cleanall src
