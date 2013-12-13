#ifdef ONE
main()
{
	typedef struct A A;
	struct A { int j; };

	{
		struct A { int i; };
		A a = { .j = 2 };

		typedef struct A A;
		A b = { .i = 3 };
	}
}
#else

typedef struct A A;

main()
{
	struct A { int i; };

	A b;
}

#endif
