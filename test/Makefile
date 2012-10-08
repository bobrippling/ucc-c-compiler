DIRS =                        \
	arrays/                     \
	Unsorted/                   \
	anywhere_decl/              \
	around/                     \
	assignment/                 \
	blocks/                     \
	builtin/                    \
	cil/                        \
	decl_parsing/               \
	initialisation/array/       \
	initialisation/scalar/      \
	initialisation/structs/     \
	hai/                        \
	inline/                     \
	input/                      \
	lib/                        \
	lvalue/                     \
	messing                     \
	misc/                       \
	other/                      \
	parse/                      \
	pointers/                   \
	stack_save/                 \
	structs/                    \
	type_parsing/               \
	types/

MAKEFILES = ${DIRS:/=/Makefile}

# we assume ucc/cc1/cpp are up to date
test: ${DIRS:/=/test}
	@echo All Tests + Purposeful Failures Succeeded

makefiles: ${MAKEFILES}
clean: makefiles ${DIRS:/=/clean}

${MAKEFILES}: make_gen.pl

help:
	@echo "make dir/test for individual tests"
	@echo "     dir/clean for cleaning"
	@echo "     makefiles for updated rules"

%/test: %/Makefile
	make -C$* test

%/Makefile: %/TestRules
	perl ./make_gen.pl $* > $*/Makefile

%/clean: %/TestRules
	make -C$* clean
	rm -f $*/Makefile

%/clean:
	make -C$* clean

.PHONY: all clean test gen ucc help

# TODO:
# anywhere_decl/
# apply/
# asm/
# C99/
# cil/
# clang/
# hai/
# input/
# messing around/
# misc/
# packing/
# parse/
# programs/
# promotion/
# Unsorted/
