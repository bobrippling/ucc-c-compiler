#!/bin/sh

if [ $# -eq 1 ]
then
	set -- "$1" "/tmp/qtest"
elif [ $# -ne 2 ]
then
	echo >&2 "Usage: $0 [-v] path/to/file test/dir"
	exit 1
fi

perl ./test.pl "$1" < /dev/null > "$2" 2>&1
r=$?
case $r in
	0)
		echo "pass: $1"
		;;
	*)
		{
			echo "fail: $1"
			sed 's;^;  ;' < "$2"
		} >&2
		exit 1
		;;
esac
