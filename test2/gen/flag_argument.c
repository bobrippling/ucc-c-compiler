// RUN: %ucc %s
// RUN: %asmcheck %s

f(){}

main()
{
	int *i;
	f((char *)0 == i);
}
