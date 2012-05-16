#!/bin/sh

[ $# -ne 0 ] && exit 1

cc -MM *.c ops/*.c | \
sed 's#^\(stmt\|expr\)\(_[^:]\+\)#ops/\1\2#' \
> Makefile.dep
