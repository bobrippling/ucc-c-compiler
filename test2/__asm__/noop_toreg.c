// RUN: %archgen %s 'x86,x86_64:movl $3, %%ecx'

main()
{
	// mov $3, %ecx
	__asm("" :: "c"(3));
}
