// RUN: %ucc -S -o- %s -target x86_64-linux -fno-semantic-interposition | grep 'movl -[0-9]*(%%[er]bp), %%eax'

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
