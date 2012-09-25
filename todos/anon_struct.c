struct
{
	struct
	{
		int a;
		union
		{
			void *vp;
			int *ip;
		};
		char *p;
	};
	int b;
} a;

main()
{
	//a.a = 1;
	a.b = 3;
	//a.ip = 3;

	printf("%d %p %d %p\n", a.a, a.p, a.b, a.ip);
}
