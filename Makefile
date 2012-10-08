all: configure
	make -C src
	make -C lib

configure:
	@if ! test -e lib/syscall_err.s; then echo ucc needs configuring; exit 1; fi

clean:
	make -C src clean
	make -C lib clean

cleanall: clean
	./configure clean

cleantest:
	make -Ctest clean

test: all
	make -Ctest test

ALL_SRC = $(shell find . -iname '*.[ch]')

tags: ${ALL_SRC}
	ctags -R .

.PHONY: all clean cleanall configure
