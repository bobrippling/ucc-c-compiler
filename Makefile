PREFIX ?= /usr/local

BINDIR = ${DESTDIR}${PREFIX}/bin
LIBDIR = ${DESTDIR}${PREFIX}/lib/ucc
INCDIR = ${DESTDIR}${PREFIX}/lib/ucc/include

all: configure
	make -C src
	make -C lib

install: all
	mkdir -p ${BINDIR}
	# TODO: setup ucc with ${LIBDIR} path
	cp ucc ${BINDIR}
	mkdir -p ${LIBDIR}
	cp src/cc1/cc1 ${LIBDIR}
	cp src/cpp2/cpp ${LIBDIR}
	mkdir -p ${INCDIR}
	cp include/* ${INCDIR}

uninstall:
	rm -f ${BINDIR}/ucc
	rm -r ${LIBDIR}

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

-include bootstrap.mak

.PHONY: all clean cleanall configure install uninstall
