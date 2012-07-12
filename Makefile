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

test: all
	make -Ctest test

.PHONY: all clean cleanall configure
