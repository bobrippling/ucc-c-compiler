main()
{
	struct
	{
		int i : 2;
	} a;

	asm("movl $5, %0" : "=rm"(a.i));

	return a.i;
}
