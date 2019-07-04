// RUN: %check %s
// RUN: %ocheck 0 %s

struct A
{
	unsigned x : 1, y : 1, z : 1;
};

another()
{
	struct
	{
		int i : 2;
	} a;

	__asm("movl $5, %0" : "=r"(a.i));

	if(a.i != 1)
		abort();

	__asm("movl $5, %0" : "=m"(a.i));

	if(a.i != 1)
		abort();
}

main()
{
	struct A a = { 0 };

	__asm("movl $4, %0" : "=m"(a.x)); // CHECK: warning: bitfield in asm output

	// 4 == 0b100
	// a = { 0, 0 }

	if(a.x || a.y)
		abort();
	if(a.z != 1)
		abort();

	another();

	return 0;
}
