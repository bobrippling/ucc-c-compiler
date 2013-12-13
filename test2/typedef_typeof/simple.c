// RUN: %ucc %s
main()
{
	int a[4];
	typeof(a) b;
	typeof(int[4]) c;
}
