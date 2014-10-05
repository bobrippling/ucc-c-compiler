// RUN: %check %s

f() __asm("hi") __attribute((noreturn));
f() __attribute((noreturn)) __asm("hi"); // CHECK: warning: asm() after __attribute__ (GCC compat)

main()
{
	f();
}
