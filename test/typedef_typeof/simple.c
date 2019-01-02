// RUN: %ucc %s
main()
{
	int a[4];
	__typeof(a) b;
	__typeof(int[4]) c;
}
