#!/bin/sh

if test $# -ne 1
then
	echo >&2 "Usage: $0 path/to/src.c"
	exit 1
fi

. "$(dirname "$0")"/common.sh

require_env UCC
require_env UCC_TESTDIR

tmp="$UCC_TESTDIR/$$.tmp"
ucc_out="$UCC_TESTDIR/$$.out.S"
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
