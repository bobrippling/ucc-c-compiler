#!/bin/sh

grep -l 'ent' *.c | while read f
do
	cc 2>&1 -fplan9-extensions -S -o- $f | ../test2/layout_normalise.pl > $f.gcc.s
	r_gcc=$?
	../ucc 2>&1 -fplan9-extensions -S -o- $f | ../test2/layout_normalise.pl > $f.ucc.s
	r_ucc=$?

	if [ $r_gcc -ne 0 ] || [ $r_ucc -ne 0 ]
	then
		echo $f
		echo "  return codes $r_gcc $r_ucc"
	fi

	rm -f $f.gcc.s $f.ucc.s
	continue

	diff -u $f.gcc.s $f.ucc.s

	rm -f $f.gcc.s $f.ucc.s
done
