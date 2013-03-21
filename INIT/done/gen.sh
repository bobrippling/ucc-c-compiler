#!/bin/sh

e=/tmp/gen$$
trap "rm -f $e" EXIT

for f in *.c
do
	../../ucc -fplan9-extensions -S -o $(echo $f | sed 's/\.c/.s/') $f > $e 2>&1
	if [ $? -ne 0 ]
	then
		echo "$f error:"
		sed 's;^;  ;' $e
	fi >&2
done
