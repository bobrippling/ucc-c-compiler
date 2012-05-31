f(void) __attribute__((overloadable))
{
	return -1;
}

f(int i) __attribute__((overloadable))
{
	return i;
}

main()
{
	printf("%d %d\n", f(), f(5));
}
