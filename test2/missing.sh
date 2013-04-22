#!/bin/sh

./list_tests.sh | while read f
do
	if ! grep -q '// *RUN' "$f"
	then echo "$f: no RUN"
	elif grep -q '// *CHECK' "$f" && ! grep -q '// *RUN: *%check' "$f"
	then
		echo "$f: no RUN::%check for CHECK:s"
	fi
done
