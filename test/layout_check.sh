#!/bin/sh

usage(){
	echo "Usage: $0 [-v] [--sections] A [B] -- [cc-args]\n" >&2
	exit 1
}

rmfiles(){
	test -z "$rmfiles" || rm -f $rmfiles
}
rmfiles=
trap rmfiles EXIT

verbose=0
sec=
args=
f=
for arg
do
	if test "$arg" = -v
	then
		verbose=1
	elif test "$arg" = '--help'
	then
		usage
	elif test "$arg" = '--sections'
	then
		sec="$arg"
	elif test -z "$f"
	then
		f="$arg"
	else
		cc_args="$cc_args $arg"
	fi
done

cc_layout_args=
if test -n "$sec"
then
	# section tests target x86_64-linux to ensure section name consistency (and hash ordering consistency)
	cc_layout_args='-target x86_64-linux'
fi

out="$UCC_TESTDIR/chk.out.$$"
rmfiles="$rmfiles $out"

if test $verbose -ne 0
then set -x
fi
"$UCC" $cc_layout_args -S -o"$out" "$f" -fno-common $cc_args
r=$?
set +x
if test $r -ne 0
then exit $r
fi

f_layout="$f.layout"

a="$UCC_TESTDIR"/$$.chk.a
b="$UCC_TESTDIR"/$$.chk.b
rmfiles="$rmfiles $a $b"

set -e

if test -z "$sec"
then
	# no section relevance, sort (for convenience / non-brittle tests that don't rely on section name hash ordering)
	./layout_normalise.pl $sec "$out" | ./layout_sort.pl > $a
	./layout_normalise.pl $sec "$f_layout" | ./layout_sort.pl > $b
else
	# sections are relevant, so make order relevant too
	./layout_normalise.pl $sec "$out" > $a
	./layout_normalise.pl $sec "$f_layout" > $b
fi

diff -u $b $a
