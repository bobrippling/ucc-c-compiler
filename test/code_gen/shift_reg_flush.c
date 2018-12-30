// RUN: %ocheck 0 %s

int f(char x)
{
	int y = 1;

	/* previously an offset reg would be used in the shift,
	 * so we would get
	 * "shl -1(%cl), %eax"
	 * instead of
	 * "sub $1, %ecx"
	 * "shl %cl, %eax"
	 */
	return y << (x - 1);
}

main()
{
	if(f(3) != 4)
		abort();

	return 0;
}
