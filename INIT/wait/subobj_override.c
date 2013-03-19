main()
{
	struct X { int a, b; } f();

	struct X xs[] = {
		[0] = f(),
		[0].b = 3 // this completely overrides [0]
	};
}
