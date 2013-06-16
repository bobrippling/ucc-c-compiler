// RUN: %ucc -std=c99 %s; [ $? -ne 0 ]
// RUN: %ucc -std=c90 %s
// RUN: %ucc -std=c89 %s

main()
{
	if((enum { a, b })0 == 1)
		;
	return b;
}
