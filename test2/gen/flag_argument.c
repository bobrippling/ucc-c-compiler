// RUN: %ucc %s

f(){}

main()
{
	int *i;
	f((char *)0 == i);
}
