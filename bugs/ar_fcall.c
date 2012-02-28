/*
puts();

void p(void)
{
	puts("p()");
}
*/

void (*ptr_ar[2])(void);

main()
{
	ptr_ar[1] = 0;

	(ptr_ar[1])();
}
