// RUN: %ucc -S -o- -target x86_64-linux %s -finline-functions -fno-semantic-interposition | grep -F 'movl $13, %%eax'

g(int i)
{
	return i + 1;
}

f(int i)
{
	return g(i) + 2;
}

main()
{
	/* this checks the single-return optimisation, where a single-return means
	 * the value isn't forced to a register, so it can take part in further
	 * optimisation, such as constant folding here. */

	// this should be movl $13, %eax
	return f(7) + 3;
}
