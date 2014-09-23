#!/bin/sh

if [ $# -ne 3 ]
then
	echo >&2 "Usage: $0 exe gdb-batch-file expected"
	exit 1
fi

tmp=$UCC_TESTDIR/gdb_run

gdb -batch -x "$2" "$1" 2>/dev/null | \
	grep -E '(^[0-9]+ |[a-zA-Z0-9_$] =)' \
	>$tmp

echo "GOT:"
cat $tmp

exec 5<$tmp
exec 4<"$3"

exit_code=0

while :
do
	read expected <&4
	r_expected=$?

	if echo "$expected" | grep '^#' >/dev/null
	then continue
	fi

	read got <&5
	r_got=$?

	if [ $r_got -ne 0 ] && [ $r_expected -ne 0 ]
	then break
	elif [ $r_got -ne 0 ]
	then die "eof on $tmp"
	elif [ $r_expected -ne 0 ]
	then die "eof on $3"
	fi

	if echo "$expected" | grep '^/' >/dev/null
	then
		regex="$(echo "$expected" | sed 's_^/__; s_/$__')"
		if echo "$got" | grep -E "$regex" >/dev/null
		then
			# fine
			:
		else
			exit_code=1
			(
				echo "regex didn't match:"
				echo "   regex: /$regex/"
				echo "   got: $got"
			) >&2
		fi
	else
		if [ "$expected" = "$got" ]
		then
			# fine
			:
		else
			exit_code=1
			(
				echo "line didn't match:"
				echo "   line: $got"
				echo "   expected: $expected"
			) >&2
		fi
	fi
done <&4

exit $exit_code
