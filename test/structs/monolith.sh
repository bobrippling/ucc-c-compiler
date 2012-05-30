#!/bin/sh

grep 'ifdef ' monolith.c | sed 's/.ifdef *//' | while read macro
do
	e=monolith_$macro
	../../src/cc -D$macro -o $e monolith.c || echo crabs with $macro
	./$e || echo run crabs with $macro
done
