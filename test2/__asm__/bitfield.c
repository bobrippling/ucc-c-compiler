// RUN: %check %s
// RUN: %ocheck 0 %s

struct A
{
	unsigned x : 1, y : 1, z : 1;
};

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

	return 0;
}
