#!/bin/sh

interrupt(){
	exit 25
}

trap interrupt INT

verbose=0
if [ "$1" = -v ]
then verbose=1
fi

ec=0

tdir=/tmp/ucc.test/
mkdir -p $tdir || exit $?

./list_tests.sh | while read f
do
	[ $verbose -ne 0 ] && echo "$0: $f"

	./qtest.sh $verbose "$f" "$tdir"/"$(basename "$f")"
	if [ $? -ne 0 ]
	then ec=1
	fi
done

exit $ec
