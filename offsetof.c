struct A
{
	char c; // 0-4
	struct B
	{
		int i;
	} b; // 4-8
	struct C
	{
		int j, z;
	} ar[2]; // 8-24 - [0] = 8-16, [2] = 16-24
	int i; // 24-28
};

long main()
{
	//return __builtin_offsetof(struct A, i);
	//return __builtin_offsetof(struct A, b.i);
	//__builtin_offsetof(struct { int a, b; }, b);

	//unsigned long f(void);
	//unsigned long i = f();
	//return __builtin_offsetof(struct A, ar[i].z);

	//return __builtin_offsetof(struct A, ar[1].z);

	switch(0){case __builtin_offsetof(struct A, ar[1].z):;}
}
