// RUN: %ocheck 0 %s

f(int *v)
{
	int ok;
	__asm(
			"mov %2, %0\r\n"
			"mov $3, %1"
			: "=r" (ok)
			, "=a" (*v)
			: "0" (10));
	return ok;
}

main()
{
	int x;
	if(f(&x) != 10)
		abort();
	if(x != 3)
		abort();
	return 0;
}
