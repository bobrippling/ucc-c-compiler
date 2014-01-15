all: src
	make -C lib

src: configure
	make -C src

deps:
	make -Csrc deps

configure:
	@if ! test -e config.mk; then echo ucc needs configuring; exit 1; fi

clean:
	make -C src clean
	make -C lib clean

cleanall: clean
	./configure clean

cleantest:
	make -Ctest clean
# no need to clean test2

check: all
	cd test2; ./run_tests -q -i ignores .
	# test/ pending

ALL_SRC = $(shell find . -iname '*.[ch]')

tags: ${ALL_SRC}
	ctags -R .

.PHONY: all clean cleanall configure src
