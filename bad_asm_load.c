__attribute((always_inline))
static inline f(int a, int b)
{
	return a<<b;
}

main()
{
	return f(5, 32);
}
