enum A { X };
enum B { Y };

main()
{
	enum A a = (enum A)0;
	enum B b = (enum B)0;
	int *i = (void *)0;
	char *p = (void *)0;
	int x;

	(void)x;

	(void)(i == p);
	(void)(a == b);
}
