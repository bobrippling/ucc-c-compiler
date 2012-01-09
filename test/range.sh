#!/bin/sh

t=/tmp/range_$$
./src/cc -o range range.c 2> $t
if [ $? -ne 0 ]
then
	cat $t
	rm -f $t
	exit 1
fi
rm -f $t

range(){
	./range $2
}

range 0
for i in `seq 1 6`
do
	a=
	for j in `seq 1 $i`
	do a="$a x"
	done
	range $i "$a"
done
