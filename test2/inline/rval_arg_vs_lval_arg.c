// RUN: %archgen %s 'x86_64,x86:movl $5, %%eax' '-DADDR=0'

// RUN: %archgen %s 'x86_64,x86:movl $3, -4(%%rbp)' 'x86_64,x86:movl -4(%%rbp), %%eax' 'x86_64,x86:addl $2, %%eax' '-DADDR=1'

f(int i)
{
#if ADDR
	&i;
#endif
	return 2 + i;
}

caller()
{
	// '3' may be addressed
	return f(3);
}
