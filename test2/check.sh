#!/bin/sh

if [ $# -ne 1 ]
then
	echo "Usage: $0 src" >&2
	exit 1
fi

ucc=../ucc
e=/tmp/check.$$

trap "rm -f $e" EXIT

set -e
$ucc -o/dev/null -c "$1" 2>$e
./check.pl < $e "$1"
