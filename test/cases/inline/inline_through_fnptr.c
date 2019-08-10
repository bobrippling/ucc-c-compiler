// RUN: %ucc -target x86_64-linux -S -o- %s -finline-functions -fno-semantic-interposition | grep 'movl $4, %%eax'

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
