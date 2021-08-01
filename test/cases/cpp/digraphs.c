# // RUN: sh %s

ucc(){
	$UCC -digraphs -P -E $@
}

err(){
	echo "$@" >&2
	exit 1
}

while read -r I O
do
	if ! echo "abc $I def" | ucc - | grep -F "$O" > /dev/null
	then
		err "digraph $I->$O failed"
	fi
done <<!
<: [
:> ]
<% {
%> }
!

cpp_test(){
	t="$UCC_TESTDIR"/$$.digrp
	cat >$t

	if ! ucc -xc -E $t | grep "$1" > /dev/null
	then err "# digraph regex failed ($1)"
	fi
}

cpp_test '5' <<!
%:define YO 5
YO
!

cpp_test 'xy' <<!
#define JOIN(a, b) a %:%: b
JOIN(x, y)
!
