#!/bin/sh

verbose=
if [ "$1" = -v ]
then
	shift
	verbose=-v
fi

error=0
if [ "$1" = -e ]
then
	shift
	error=1
fi

usage(){
	echo "Usage: $0 [-e] cc-params..." >&2
	echo "-e: expect ucc to error" >&2
	exit 1
}

ucc=../ucc
e=/tmp/check.$$

trap "rm -f $e" EXIT

$ucc -o/dev/null -c $@ 2>$e
r=$?
if [ $r -ne 0 ]
then r=1
fi

if [ $r -ne $error ]
then
	s=
	if [ $error -eq 0 ]
	then s="no "
	fi
	echo "${s}error expected"
	cat $e
	exit 1
fi >&2
./check.pl $verbose < $e "$1"
exit $?
