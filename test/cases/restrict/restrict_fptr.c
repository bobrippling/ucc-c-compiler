// RUN: %check -e %s

main()
{
	int (*restrict f)(void); // CHECK: error: restrict qualified function pointer
}
