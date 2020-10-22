// RUN: %check -e %s

f() __asm("hi");
f() __asm("hi2"); // CHECK: error: decl "f" already has an asm() name ("hi")

main()
{
	f();
}
