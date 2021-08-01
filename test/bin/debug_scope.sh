#!/bin/sh

if [ $# -lt 1 ]
then
	echo "Usage: $0 src [cflags...]" >&2
	exit 1
fi

if [ -z "$UCC_TESTDIR" ] || [ -z "$UCC" ]
then
	echo "need \$UCC_TESTDIR and \$UCC" >&2
	exit 2
fi

f="$1"
shift
e="$UCC_TESTDIR/exe"

# compile
"$UCC" -g -o "$e" "$f" "$@"
r=$?
if [ $r -ne 0 ]
then exit $r
fi

# walk source file looking for // SCOPE:
line_no=1
while read line
do
	expected=$(echo "$line" \
		| sed -n 's@.*SCOPE: *\(.*\)@\1@p' \
		| tr ' ' '\n' \
		| sort)

	if [ -n "$expected" ]
	then
		got=$(echo "i scope $f:$line_no" \
			| gdb "$e" 2>/dev/null \
			| sed -n 's/Symbol \([a-zA-Z0-9_]*\) is.*/\1/p' \
			| sort)

		if [ "$expected" != "$got" ]
		then
			echo "$f:$line_no:"
			echo "  expected = '$expected'"
			echo "  got      = '$got'"
		fi
	fi
	line_no=$(expr $line_no + 1)
done <"$f"
