// RUN: %ucc %s
// RUN: %ucc %s | %check %s

main()
{
	char s;
	int i;

	s -= i;

	int *p; // CHECK: /warning: mixed code and declarations/
	++p;

	p += s;
}
