main()
{
	int a;
	__asm("mov $5, %0" : "=&r"(a));
	return a;
}
