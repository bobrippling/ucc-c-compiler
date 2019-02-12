#!/bin/sh

verbose=
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
			-v)
				shift
				verbose=-v
				;;
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

ucc=../ucc
e=/tmp/check.$$

trap "rm -f $e" EXIT

# ensure "$f" comes after other args, to allow for things like -x ...
f="$1"
shift

$ucc $synonly "$@" "$f" 2>$e
r=$?

# check for abort
if test $r -gt 5
then
	echo >&2 "unexpected ucc exit code '$r'"
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
	cat $e
	exit 1
fi >&2
./check.pl $only $prefix $verbose < $e "$f"
exit $?
