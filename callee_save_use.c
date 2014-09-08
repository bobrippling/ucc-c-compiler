use_ebx()
{
	int ebx_temp;
	// having "m" here causes the bug, as the asm() temp spill conflicts
	__asm("mov %1, %0" : "=b"(ebx_temp)  : "m"(3));
	return ebx_temp;
}

main()
{
	__asm("mov %0, %%ebx" : : "g"(72));

	if(use_ebx() != 3)
		abort();

	int ebx_after;
	__asm("mov %%ebx, %0" : "=g"(ebx_after));

	if(ebx_after != 72)
		abort();
}
