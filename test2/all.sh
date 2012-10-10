#!/bin/sh

find . -iname '*.c' | while read f
do perl ./test.pl $@ "$f" || exit $?
done
