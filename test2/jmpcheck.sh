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
	sed 's/ *#.*//'
}

asfilter <"$1".jumps >"$tmp"

$UCC -S -o "$ucc_out" "$1" 2>/dev/null

<$ucc_out asfilter | grep '^\(\.Lblk\|[a-z]\+\)\|jmp ' | diff -u - "$tmp"
