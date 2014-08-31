// RUN: %check -e %s

main()
{
	__asm("mov $5, %0" : "=i"(*(int *)3)); // CHECK: error: 'i' constraint in output
}
