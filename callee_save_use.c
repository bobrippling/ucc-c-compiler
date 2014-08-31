main()
{
	int ebx_temp;
	__asm("mov %1, %0" : "=b"(ebx_temp)  : "m"(3));
	return ebx_temp;
}
