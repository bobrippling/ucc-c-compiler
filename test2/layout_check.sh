#!/bin/sh

usage(){
	echo "Usage: $0 A [B] -- [cc-args]\n" >&2
	exit 1
}

verbose=0
if [ "$1" = -v ]
then
	verbose=1
	shift
fi

if [ "$1" = '--help' ]
then usage
fi

if [ $# -ge 1 ]
then
	if echo "$1" | grep '\.c$' > /dev/null
	then
		in="$1"
		shift
		out="/tmp/chk.out.$$"

		trap "rm -f $out" EXIT

		if [ $verbose -ne 0 ]
		then echo "$0: ucc -S -o'$out' '$in' $@"
		fi

		# $@ are the optional compiler args
		"$UCC" -S -o"$out" "$in" "$@"

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

./layout_normalise.pl "$1" $cc_args > $a
./layout_normalise.pl "$2" $cc_args > $b

diff -u $b $a
