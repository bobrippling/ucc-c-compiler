// RUN: %check -e %s

main()
{
	int i = 3;

	__asm("%0 <-- %1"
			: "=r"(i)
			: "b+"(5)); // CHECK: error: modifier character not at start ('+')

	return i;
}
