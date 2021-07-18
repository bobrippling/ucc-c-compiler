// RUN: %ucc -S -o- %s | grep 'hi:'
// RUN: %check %s

int f() __asm("hi");

int f() __attribute((warn_unused_result));

f()
{
	return 3;
}

main()
{
	f(); // CHECK: /warning: unused expression/
}
