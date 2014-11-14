// RUN: %archgen %s 'x86_64,x86:movl $4, %%eax' -finline-functions

apply(int fn(int, int), int a, int b)
{
	return fn(a, b);
}

add(int a, int b)
{
	return a + b;
}

main()
{
	return apply(add, 1, 3);
}
