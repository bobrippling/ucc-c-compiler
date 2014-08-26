// RUN: %check %s
// RUN: %ucc -S -o- %s | grep 'mem from func: .*(%%rbp)'

main()
{
	int f();

	__asm("mem from func: %0" :: "m"(f())); // CHECK: warning: operand will need to be spilt to memory
}
