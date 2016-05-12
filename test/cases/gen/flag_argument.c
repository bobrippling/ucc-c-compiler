// RUN: %ucc -c -o %t %s

f(){}

main()
{
	int *i;
	f((char *)0 == i);
}
