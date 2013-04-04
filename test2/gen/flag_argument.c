// RUN: %ucc %s
// RUN: %asmcheck %s

main()
{
	int *i;
	f((char *)0 == i);
}
