#!/bin/sh

good=0
bad=1
abort=128
skip=125

if [ $# -eq 0 ]
then
	echo >&2 "Usage: $0 ucc-arguments"
	exit $abort
fi

make -sCsrc || exit $skip

./ucc "$@" || exit $bad
exit $good
