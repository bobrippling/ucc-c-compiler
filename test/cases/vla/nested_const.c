// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f(int n)
{
	short x[2][n][3];

	/* when calculating the sizeof x,
	 * we do 2 * {saved vla size},
	 * not   2 * {saved vla size} * 3 * sizeof(short)
	 * since the saved vla size already includes
	 * the 3 * sizeof(short) calculation
	 */

	return sizeof x;
}

main()
{
	if(f(3) != sizeof(short) * 2 * 3 * 3)
		abort();

	return 0;
}
