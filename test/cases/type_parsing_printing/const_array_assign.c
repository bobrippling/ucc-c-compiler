// RUN: %check -e %s

f(int [const]);

f(int p[const])
{
	p++; // CHECK: error: can't modify const expression identifier
}

main()
{
	int a[3];
	f(a);
}
