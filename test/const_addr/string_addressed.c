// RUN: %ucc %s -o %t
// RUN: %t | grep '^cd$'

char *p = &2["abcd"];

main()
{
	puts(p);
}
