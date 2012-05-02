#!/bin/sh

for f in *.c
do
	echo --- $f ---
	../src/cc $f || exit $?
done
