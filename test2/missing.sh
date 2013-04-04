#!/bin/sh

./list_tests.sh | while read f
do
	if ! grep -q '// *RUN' "$f"
	then echo "$f"
	fi
done
