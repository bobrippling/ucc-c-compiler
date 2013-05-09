// RUN: %check -e %s

main()
{
	void (^const  blockA)(void) = ^{ };
	//void (*const  ptrA)         = (void *)0;

	blockA = ^{ }; // CHECK: /error: .*const/
	//ptrA   = (void *)5;
}
