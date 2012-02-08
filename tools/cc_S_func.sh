#!/bin/sh

if [ $# -lt 3 ]
then
	echo >&2 "Usage: $0 start-func end-func cc-args..."
	exit 1
fi

start="$1"
shift
end="$1"
shift

./src/cc -S -o- $@ | awk "BEGIN{p=0} /^$start/{p=1} /$end/{p=0}{if(p)print}"|sed 's/;.*//; /^ *$/d'
