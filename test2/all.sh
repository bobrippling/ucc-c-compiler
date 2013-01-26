#!/bin/sh

verbose=0
if [ "$1" = -v ]
then verbose=1
fi

ec=0

find . -iname '*.c' | while read f
do
	[ $verbose -ne 0 ] && echo "$0: $f"

	perl ./test.pl $@ "$f" > /dev/null 2>&1 < /dev/null
	r=$?
	if [ $r -ne 0 ]
	then
		echo "fail: $f"
		ec=1
	fi
done

exit $ec
