// RUN: %archgen %s 'x86:shll $3 %%eax' 'x86_64:shlq $3, %%rax'

f(char x)
{
	// sizeof(long) * (x + 2) should be evaluated using size_t arith
	return sizeof(long[x + 2]);
}

main()
{
	return f(2);
}
