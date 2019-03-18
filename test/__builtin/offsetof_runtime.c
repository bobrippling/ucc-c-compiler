// RUN: %ocheck 36 %s

struct A
{
	int x;
	struct
	{
		int a, b, z;
	} ar[3];
};

f()
{
	return 2;
}

main()
{
	volatile int i = f();
	return __builtin_offsetof(struct A, ar[i].z);
}
