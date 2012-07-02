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

run(){
	f=$(outfile $1)
	./$f
}

comp_and_run(){
	compile $1
	run $1
}

clean(){
	f=$(outfile $1)
	echo rm $f
	rm -f $f
}

usage(){
	echo "Usage: $0 [clean|run]" >&2
	exit 1
}

cmd=compile

if [ $# -eq 1 ]
then
	if [ "$1" = clean ]
	then
		cmd=clean
	elif [ "$1" = run ]
	then
		cmd=comp_and_run
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
