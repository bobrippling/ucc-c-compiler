#!/bin/sh

if [ "$1" = -v ]
then verbose=1
fi

find . -iname '*.c' | while read f
do
	[ $verbose -ne 0 ] && echo "$0: $f"

	perl ./test.pl $@ "$f" || exit $?
done
