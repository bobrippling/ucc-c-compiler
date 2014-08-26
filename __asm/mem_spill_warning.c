// RUN: %check %s

main()
{
	int f();

	__asm("mem from func: %0" :: "m"(f()));
}
