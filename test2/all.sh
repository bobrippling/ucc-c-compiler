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

	b="$(basename "$f")"
	e="$tdir/$b"
	perl ./test.pl "$f" < /dev/null > $e 2>&1
	r=$?
	case $r in
		0)
			echo "pass: $f"
			;;
		*)
			{
				echo "fail: $f"
				sed 's;^;  ;' < "$e"
			} >&2
			ec=1
			;;
	esac
done

exit $ec
