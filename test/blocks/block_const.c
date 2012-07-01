main()
{
	void (^const  blockA)(void) = ^{ };
	void (*const  ptrA)         = (void *)0;

	blockA = ^{ };
	ptrA   = (void *)5;
}
