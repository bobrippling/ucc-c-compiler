# // RUN: sh %s

ucc(){
	$UCC -trigraphs -P -E $@
}

err(){
	echo "$@" >&2
	exit 1
}

while read -r I O
do
	if ! echo "abc $I def" | ucc - | grep -F "$O" > /dev/null
	then
		err "trigraph $I->$O failed"
	fi
done <<!
??( [
??) ]
??< {
??> }
??/ \\
??' ^
??! |
??- ~
!

t="$UCC_TESTDIR"/$$.trigrp
cat >$t <<!
??=define YO 5
YO
!

if ! ucc -xc -E $t | grep 5 > /dev/null
then
	err "# trigraph failed"
fi
