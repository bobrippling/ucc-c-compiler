f(int i)
{
	int o;
	__asm("incl %1\r\n;"
			"movl %0, %1"
			: "=m"(o) : ""(i));
	return o;
}

main()
{
	int i = 5;
	return f(i);
}
