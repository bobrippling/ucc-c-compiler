#!/bin/sh

log=/tmp/check_log_$$
trap "rm -f $log" EXIT

d=$(dirname "$0")

ret=0

check(){
	$@ > $log 2>&1
	if [ $? -ne 0 ]
	then
		printf '\e[1;31mbroke: %s:\e[m\n' "$f"
		sed 's/^/	/' $log
		ret=1
	fi
}

for f in $d/*.c
do
	check $d/../src/cc $f
	check ./a.out
done

rm -f a.out

exit $ret
