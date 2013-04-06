#!/bin/sh

if [ $# -ne 1 ]
then
	echo "Usage: $0 file" >&2
	exit 1
fi

o="/tmp/$$.tmp"

trap "rm -f $o" EXIT

if [ -z "$CC" ]
then CC=gcc # we want to avoid copying clang's layout for certain things
fi

./timeout 3 $CC -fno-common -S -o- "$1" > "$o"
r=$?
if [ $r -ne 0 ]
then exit $r
fi
set -x
./layout_normalise.pl "$o" > "${1}.layout"
