__attribute__((overloadable)) f(void)
{
	return -1;
}

__attribute__((overloadable)) f(int i)
{
	return i;
}

main()
{
	printf("%d %d\n", f(), f(5));
}
