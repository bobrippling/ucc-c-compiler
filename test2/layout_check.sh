#!/bin/sh

usage(){
	echo "Usage: $0 A [B]\n" >&2
	exit 1
}

if [ $# -eq 1 ]
then
	if [ "$1" = '--help' ]
	then usage
	fi
	set -- "$1" "${1}.layout"
fi

if [ $# -ne 2 ]
then usage
fi

a=/tmp/$$.chk.a
b=/tmp/$$.chk.b
trap "rm -f $a $b" EXIT

set -e

./layout_filter.pl "$1" > $a
./layout_filter.pl "$2" > $b

diff -u $a $b
