#!/bin/sh

set -e
cd `dirname $0`

cc=../../src/cc

for f in *.c
do
	b=`echo $f | sed 's/..$//'`
	if ! [ -e $b ]
	then
		$cc -o $b $f
		printf '\e[1;34m%s\e[m\n' "$f"
	else
		echo -e $b
	fi
done
