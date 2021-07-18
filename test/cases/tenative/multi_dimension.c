// RUN: %ucc -fsyntax-only %s

int a[][2];

int main()
{
	a[0][0] = 3;
	/* this tests a bug where the above assignment would attempt to check integer
	 * promotions (on a dereference of an array in the LHS), and attempt to find
	 * the size of `a` before it had been completed. integer promotions are now
	 * handled properly */
}
