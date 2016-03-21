// RUN: %archgen %s 'x86_64,x86:/movl -[0-9]*\(%%[er]bp\), %%eax/'

__attribute((always_inline))
inline f(volatile int i)
{
	return i + 1;
}

main()
{
	// gcc incorrectly optimises this to 'ret 4'
	return f(3);
}
