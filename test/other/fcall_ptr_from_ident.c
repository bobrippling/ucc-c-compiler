extern printf();
main()
{
	void (*f)(void) = printf;
	((void (*)(char *, ...))printf)("%d\n", 5);
}
