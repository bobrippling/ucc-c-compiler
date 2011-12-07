#!/bin/sh

if [ $# -ne 0 ]
then
	echo Usage: $0 >&2
	exit 1
fi

CC=../src/cc

for f in addr.c \
	cat.c \
	late_decl.c \
	old_func.c \
	printf_args.c \
	printf_simple.c \
	ptr_arith.c \
	sizeof.c \
	tcc_first.c \
	title.c \
	variadic.c \
	void_func_test.c
do
	echo CC $f
	$CC -w -o $(echo $f | sed 's/..$//') $f || exit $?
done

echo Compile Stage Pass
