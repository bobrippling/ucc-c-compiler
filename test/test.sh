#!/bin/sh

outfile(){
	echo $1 | sed 's/..$//'
}

compile(){
	f=$1
	of=$(outfile $f)

	echo CC $f
	if [ ! -e $of -o $f -nt $of ]
	then $CC -w -o $of $f
	fi
}

clean(){
	f=$(outfile $1)
	echo rm $f
	rm -f $f
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
	$cmd $f || exit $?
done

# TODO: run
