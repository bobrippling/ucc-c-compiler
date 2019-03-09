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
# no need to clean test

check: all lib
	cd test; ./run_tests -q -i ignores .
	# test/ pending

ALL_SRC = $(shell find src -iname '*.[ch]')

tags: ${ALL_SRC}
	ctags '--exclude=_*' -R .

.PHONY: all clean cleanall src
