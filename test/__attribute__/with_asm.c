// RUN: %check %s

f() __asm("hi") __attribute((noreturn));
f() __attribute((noreturn)) __asm("hi"); // CHECK: warning: asm() after __attribute__ (GCC compat)

// this test also checks that `f() noreturn asm(...)`'s type is equivalent to `f() asm(...) noreturn`'s type

main()
{
	f();
}
