// RUN: %check %s

take(void fn());

main()
{
	void (*pf2)() = (void *)0; // CHECK: !/warning:.*function.*pointer/
	void (*pf3)() = 0; // CHECK: !/warning:.*function.*pointer/

	take((void *)0); // CHECK: !/warning:.*function.*pointer/
	take(0); // CHECK: !/warning:.*function.*pointer/

	(void)pf2;
	(void)pf3;
}
