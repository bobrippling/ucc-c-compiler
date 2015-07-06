all: src
	make -C lib

src: src/config.mk
	make -C src

deps:
	make -Csrc deps

src/config.mk:
	echo ucc needs configuring >&2; exit 1

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

.PHONY: all clean cleanall src
