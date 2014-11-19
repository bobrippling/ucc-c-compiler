// RUN: %check -e %s

main()
{
	struct A { long a, b, c; } a;
	__asm("" : "=r"(a) : "r"(a)); // CHECK: error: struct involved in __asm__() output
}
