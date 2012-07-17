#!/bin/sh

log=/tmp/check_log_$$
trap "rm -f $log" EXIT

d=$(dirname "$0")

ret=0
cmd=comp_and_run

check(){
	i=0
	$@ > $log 2>&1

	if [ $? -ne 0 ]
	then
		printf '\e[1;31mbroke: %s:\e[m\n' "$f"
		sed 's/^/	/' $log
		ret=1
		i=1
	fi
	return $i
}

comp_and_run(){
	check $d/../../ucc -o $b $f && check $b -
}

clean(){
	rm -f $b
}

usage(){
	echo >&2 "Usage: $0 [clean]"
	exit 1
}

if [ $# -eq 1 ]
then
	if [ "$1" = clean ]
	then cmd=clean
	else usage
	fi
elif [ $# -ne 0 ]
then usage
fi

for f in $d/*.c
do
	b=`echo $f | sed 's/\.c$//'`
	$cmd
done

exit $ret
