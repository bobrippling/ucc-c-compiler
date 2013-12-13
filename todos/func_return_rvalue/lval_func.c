struct A { int i, j; } f();

main()
{
	f().i = 3; // not an lvalue for `operator.'
}
