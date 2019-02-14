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

sec=
if [ "$1" = '--help' ]
then usage
elif [ "$1" = '--sections' ]
then
	sec="$1"
	shift
fi

rmfiles(){
	test -z "$rmfiles" || rm -f $rmfiles
}
rmfiles=
trap rmfiles EXIT

if [ $# -ge 1 ]
then
	if echo "$1" | grep '\.c$' > /dev/null
	then
		in="$1"
		shift
		out="/tmp/chk.out.$$"

		rmfiles="$rmfiles $out"

		if [ $verbose -ne 0 ]
		then echo "$0: $UCC -S -o'$out' '$in' -fno-common $@"
		fi

		# $@ are the optional compiler args
		"$UCC" -S -o"$out" "$in" -fno-common "$@"
		r=$?
		if [ $r -ne 0 ]
		then exit $r
		fi

		set -- "$out" "$in.layout"
	else
		set -- "$1" "${1}.layout"
	fi
fi

if [ $# -ne 2 ]
then usage
fi

if ! test -e "$2"
then
	echo >&2 "$0: $2 doesn't exist"
	exit 1
fi

a=/tmp/$$.chk.a
b=/tmp/$$.chk.b
rmfiles="$rmfiles $a $b"

set -e

./layout_normalise.pl $sec "$1" | ./layout_sort.pl > $a
./layout_normalise.pl $sec "$2" | ./layout_sort.pl > $b

diff -u $b $a
