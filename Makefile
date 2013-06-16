all: configure
	make -C src
	make -C lib

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

test: all
	make -Ctest test
	cd test2; sh all.sh

ALL_SRC = $(shell find . -iname '*.[ch]')

tags: ${ALL_SRC}
	ctags -R .

.PHONY: all clean cleanall configure
