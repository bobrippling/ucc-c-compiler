#!/bin/sh

outfile(){
	echo $1 | sed 's/..$//'
}

compile(){
	echo CC $f
	$CC -w -o $(outfile $f) $f
}

clean(){
	rm -f $(outfile $f)
}

usage(){
	echo Usage: $0 >&2
	exit 1
}

cmd=compile

if [ $# -eq 1 ]
then
	if [ "$1" = clean ]
	then
		cmd=clean
	else
		usage
	fi
elif [ $# -ne 0 ]
then
	usage
fi

CC=../src/cc

for f in *.c
do
	of=$(outfile $f)
	if [ ! -e $of -o $f -nt $of ]
	then $cmd $f || exit $?
	fi
done

# TODO: run
