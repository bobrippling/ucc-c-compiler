// RUN: %ucc %s -o %t
// RUN: %t

main()
{
	void *p = &&a;
	void **pp = &p;

	goto **pp;
a:
	;

	return 0;
}
