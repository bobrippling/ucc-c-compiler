#!/bin/sh

if [ $# -ne 1 ]
then
	echo "Usage: $0 src" >&2
	exit 1
fi

ucc=../ucc
e=/tmp/check.$$

trap "rm -f $e" EXIT

$ucc -o/dev/null -c "$1" 2>$e
r=$?
if [ $r -ne 0 ]
then
	echo "make '$1':"
	cat $e
	exit $r
fi >&2
./check.pl < $e "$1"
exit $?
