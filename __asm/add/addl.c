f(int a, int b)
{
	__asm__(
			"addl %1, %0"
			: "+d" (a)
			: "g" (b)
			);

	return a;
}
