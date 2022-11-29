void match_constraint_priority()
{
	int n = 21;
	int discard0, discard1;

	// gcc's extended inline assembly
	__asm__(
			/* n *= 5 */
			"leal (%0,%0,4),%0\n"
			"\t# 0=%0 1=%1 2=%2 3=%3 4=%4"
			: "=r" (n)
			, "=r" (discard0)
			, "=r" (discard1)
			: "0" (n) // we should prioritise the matching constraint first,
			// so we get its register before any other inputs take
			// the (input-slot) register
			, "r" (3)
	);
}

int copy_var()
{
	int discard0, discard1;
	int to, from = 3;

	__asm__(
			"# 0=%0 1=%1 2=%2 3=%3 4=%4"
			: "=r" (to)
			, "=r" (discard0)
			, "=r" (discard1)
			: "r" (99)
			, "0" (from)
	);

	return to;
}

int copy_var2()
{
	int discard0, discard1;
	int to, from = 3;

	__asm__(
			"# 0=%0 1=%1 2=%2 3=%3 4=%4"
			: "=&r" (to)
			, "=r" (discard0)
			, "=r" (discard1)
			: "r" (99)
			, "0" (from)
	);

	return to;
}

int copy_var3()
{
	int discard0, discard1;
	int to, from = 3;

	__asm__(
			"# 0=%0 1=%1 2=%2 3=%3 4=%4"
			: "+r" (to)
			, "=r" (discard0)
			, "=r" (discard1)
			: "r" (99)
			, "0" (from) // error, can't match a r/w output (the read specifically)
	);

	return to;
}
