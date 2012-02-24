#!/bin/sh

log=/tmp/check_log_$$
trap "rm -f $log" EXIT

d=$(dirname "$0")

ret=0

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

for f in $d/*.c
do
	b=`echo $f | sed 's/\.c$//'`
	check $d/../src/cc -o $b $f && check $b -
done

exit $ret
