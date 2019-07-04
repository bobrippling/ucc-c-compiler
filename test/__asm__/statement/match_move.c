// RUN: %ocheck 3 %s

main()
{
	int i;
	__asm(""
			: "=r"(i)
			: "0"(3)
			);
	return i;
}
