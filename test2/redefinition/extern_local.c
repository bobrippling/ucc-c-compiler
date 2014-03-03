// RUN: %check -e %s

char good;
char bad;

f()
{
	extern char good; // CHECK: !/error/
}

g()
{
	int good = 5; // CHECK: !/error/
	{
		extern char good; // CHECK: !/error/
	}
}

main()
{
	/* section 6.1.2.2 */
	extern int bad; // CHECK: error: incompatible redefinition of "bad"

	return bad;
}
