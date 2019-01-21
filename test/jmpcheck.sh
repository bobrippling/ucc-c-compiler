#!/bin/sh

if test $# -ne 1
then
	echo >&2 "Usage: $0 path/to/src.c"
	exit 1
fi

if test -z "$UCC"
then
	echo >&2 "$0: need \$UCC"
	exit 1
fi

tmp="$1".tmp
ucc_out="$1".S
trap "rm -f '$tmp' '$ucc_out'" EXIT

asfilter(){
	sed 's/ *#.*//; s/\.Lblk/Lblk/g; /^[ 	]*$/d; s/^_//'
}

jmpfilter(){
	asfilter | grep -i 'jmp\|[a-z0-9.]*: *$' | sed '/Lsection/d; /Lfuncend_/d'
}

asfilter <"$1".jumps >"$tmp"

$UCC -S -o "$ucc_out" "$1" 2>/dev/null

jmpfilter <$ucc_out | diff -u - "$tmp"
