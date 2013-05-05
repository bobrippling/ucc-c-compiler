#!/bin/sh

./list_tests.sh | while read f
do
	if grep -q '// *CHECK' "$f"
	then
		if ! grep -q '// *RUN:.*%check '
		then echo "$f missing RUN: %check for CHECK commands"
		fi
	fi
done
