// RUN: %check %s

_Noreturn struct A *p()
	// CHECK: ^/warning:.*marked no-return has a non-void return value/
{
	for(;;);
}

main()
{
}
