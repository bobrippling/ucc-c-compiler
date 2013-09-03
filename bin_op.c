void takes_ptr(int *);
void takes_int(int);

f(int a0, int *a, int c)
{
	takes_int(a0 + a + c);
}

main()
{
	takes_ptr(5 + 7 * 3);
	takes_ptr((unsigned short *)32 + __builtin_types_compatible_p(int,int));
	takes_ptr(+5);
}
