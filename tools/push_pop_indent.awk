#!/bin/awk -f

/pop/ {
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
