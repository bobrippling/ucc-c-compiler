// RUN: %ucc %s -o %t
// RUN: %t | grep '^cd$'
int puts(const char *s);

char *p = &2["abcd"];

main()
{
	puts(p);
}
