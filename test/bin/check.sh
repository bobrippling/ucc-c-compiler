#!/bin/sh

. "$(dirname "$0")"/common.sh

require_env UCC
require_env UCC_TESTDIR

verbose=$UCC_VERBOSE
error=0
prefix=
only=
synonly=-fsyntax-only
in_ucc_args=0
for arg in "$@"
do
	if test $in_ucc_args -eq 0
	then
		case "$arg" in
			-e)
				shift
				error=1
				;;
			--prefix=*)
				shift
				prefix="$arg"
				;;
			--only)
				shift
				only="$arg"
				;;
			*)
				# on the filename
				in_ucc_args=1
				;;
		esac
	else
		case "$arg" in
			-E)
				# don't need a -fsyntax-only option
				synonly=
				;;
			-[cS])
				echo >&2 "Useless argument '$arg' passed to ucc via %check"
				exit 2
				;;
		esac
	fi
done

usage(){
	echo "Usage: $0 [-e] [--prefix=...] [--only] cc-params..." >&2
	echo "-e: expect ucc to error" >&2
	exit 1
}

e="$UCC_TESTDIR"/check.$$

trap "rm -f '$e'" EXIT

# ensure "$f" comes after other args, to allow for things like -x ...
f="$1"
shift

$UCC -fno-show-line -Werror=unknown-warning-option $synonly "$@" "$f" 2>$e
r=$?

# check for abort
if test $(expr $r '&' 127) -ne 0
then
	echo >&2 "$0: ucc caught signal ($r)"
	echo >&2 "invocation flags: $@ $f"
	cat >&2 <"$e"
	exit 1
fi

if [ $r -ne 0 ]
then r=1
fi

if [ $r -ne $error ]
then
	s=
	if [ $error -eq 0 ]
	then s="no "
	fi
	echo "${s}error expected"
	cat "$e"
	exit 1
fi >&2

exec ./bin/check.pl $only $prefix "$f" < $e
