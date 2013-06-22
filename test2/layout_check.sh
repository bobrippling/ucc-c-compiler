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

	if echo "$1" | grep '\.c$' > /dev/null
	then
		in="$1"
		out="/tmp/chk.out.$$"

		trap "rm -f $out" EXIT

		"$UCC" -S -o"$out" "$1"

		set -- "$out" "$in.layout"
	else
		set -- "$1" "${1}.layout"
	fi
fi

if [ $# -ne 2 ]
then usage
fi

a=/tmp/$$.chk.a
b=/tmp/$$.chk.b
trap "rm -f $a $b" EXIT

set -e

./layout_normalise.pl "$1" > $a
./layout_normalise.pl "$2" > $b

diff -u $a $b
