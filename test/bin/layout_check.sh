#!/bin/sh

usage(){
	echo "Usage: $0 [-v] [--sections] [--layout=layout-file] [--update] test-file [cc-args]\n" >&2
	exit 1
}

verbose=$UCC_VERBOSE
if test -z "$verbose"
then verbose=0
fi

rmfiles(){
	test -z "$rmfiles" || rm -f $rmfiles
}
rmfiles=
trap rmfiles EXIT

sec=
args=
f=
update=$UCC_UPDATE_SNAPSHOTS
for arg
do
	if test "$arg" = '--help'
	then
		usage
	elif test "$arg" = '--sections'
	then
		sec="$arg"
	elif test "$arg" = '--update'
	then
		update=1
	elif echo "$arg" | grep '^--layout=' >/dev/null
	then
		f_layout="$(echo "$arg" | sed 's/^--layout=//')"
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

if test -z "$f_layout"
then f_layout="$f.layout"
fi

a="$UCC_TESTDIR"/$$.chk.a
b="$UCC_TESTDIR"/$$.chk.b
rmfiles="$rmfiles $a $b"

set -e

if test -z "$sec"
then
	# no section relevance, sort (for convenience / non-brittle tests that don't rely on section name hash ordering)
	bin/layout_normalise.pl $sec "$out" | bin/layout_sort.pl > $a
	bin/layout_normalise.pl $sec "$f_layout" | bin/layout_sort.pl > $b
else
	# sections are relevant, so make order relevant too
	bin/layout_normalise.pl $sec "$out" > $a
	bin/layout_normalise.pl $sec "$f_layout" > $b
fi

if test $update = 1
then
	cat "$a" >"$f_layout"
	echo >&2 "updated $f_layout"
else
	exec diff -u $b $a
fi
