#!/bin/sh

for f in *.c
do
	echo --- $f ---
	../src/cc -w $f || exit $?
done
