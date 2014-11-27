// RUN: %ocheck 0 %s

typedef _Bool bool;

bool add_and_check_overflow(int *a, int b)
{
	bool result;

	__asm__(
			"addl %2, %1\n\t"
			"seto %0" // works because result is of bool type
			: "=q" (result), "+g" (*a)
			: "r" (b));

	return result;
}

show_of(int i, int add, int expected, int expected_of)
{
	//printf("%d\n", i);
	int of = add_and_check_overflow(&i, add);
	//printf("+%d -> %d (of=%d)\n", add, i, of);

	if(i != expected)
		abort();
	if(of != expected_of)
		abort();
}

main()
{
	show_of(5, 3, 8, 0);
	show_of(~0u / 2, 1, -(~0u / 2) - 1, 1);

	return 0;
}
