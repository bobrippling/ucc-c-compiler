// ebx:edx:ecx - 12 chars

hacky()
{
	char str[13];

	__asm("cpuid"
			: "=c"(*(int *)&str[8]),
			  "=d"(*(int *)&str[4]),
			  "=b"(*(int *)&str[0])
			: "a"(0));

	str[12] = 0;

	printf("str = %s\n", str);
}

no_ub()
{
	typedef int int32_t;

	union
	{
		char str[13];
		struct
		{
			int32_t low, mid, hi;
		};
	} u;

	__asm("cpuid"
			: "=c"(u.hi),
			  "=d"(u.mid),
			  "=b"(u.low)
			: "a"(0));

	u.str[12] = 0;

	printf("str = %s\n", u.str);
}

main()
{
	hacky();
	no_ub();
}
