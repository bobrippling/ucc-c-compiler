#!/bin/sh

require_env(){
	env=`eval echo '$'"$1"`
	if test -z "$env"
	then
		echo >&2 "$0: need \$$1"
		exit 1
	fi
}

maybe_compile(){
	require_env UCC
	require_env UCC_TESTDIR

	ccargs=
	while test $# -ne 0 && test "$1" != "--"
	do
		ccargs="$ccargs $1"
		shift
	done
	shift

	if echo "$1" | grep '\.c$' > /dev/null
	then
		in="$1"
		shift
		out="$UCC_TESTDIR/tmp.compiled.$$"

		rmfiles="$rmfiles $out"

		if test -n "$UCC_VERBOSE"
		then set -x
		fi

		# $@ are the optional compiler args
		"$UCC" $ccargs -o"$out" "$in" "$@"
		r=$?

		if test -n "$UCC_VERBOSE"
		then set +x
		fi

		if test $r -ne 0
		then exit $r
		fi

		return 0
	else
		return 1
	fi
}
