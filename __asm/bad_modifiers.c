main()
{
	int i = 3;

	__asm("%0 <-- %1"
			: "=r"(i)
			: "b+"(5)); // can't have "+" after "a"

	return i;
}
