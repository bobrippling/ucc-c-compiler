#!/bin/sh

exec awk '
/pop|leave/ {
	if(--indent < 0)
		print "AWK: indent less than zero (" indent ")" #| "cat >&2"
}

{
	i = indent
	while(i --> 0)
		printf "\t"

	print $0
}

/push/ {
	indent++
}

/(add|sub) rsp/ {
	n = $3 / 8
	if($1 == "sub")
		n = -n
	indent -= n
}
'
