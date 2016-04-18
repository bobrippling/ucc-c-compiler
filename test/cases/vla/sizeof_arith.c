// RUN: %archgen %s 'x86:imull $3 %%eax' 'x86_64:imulq $8, %%rax' -Dunsigned=
// RUN: %archgen %s 'x86:imull $3 %%eax' 'x86_64:imulq $8, %%rax'

f(unsigned char x)
{
	// sizeof(long) * (x + 2) should be evaluated using size_t arith
	return sizeof(unsigned long[x + 2]);
}

main()
{
	return f(2);
}
