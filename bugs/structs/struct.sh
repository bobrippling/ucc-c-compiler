#!/bin/sh

set -e

for d in BASIC \
	SIMPLE \
	NESTED \
	NAMING \
	NESTED_DIFFERENT \
	EXPR_TEST
do
	./src/cc -E -D$d -o struct_$(echo $d | tr 'A-Z' 'a-z').c struct.c
done
