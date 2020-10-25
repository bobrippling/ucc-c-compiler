// RUN: %check -e %s
main()
{
	int f(int);
	{
		int f(char *); // CHECK: /error: incompatible redefinition of "f"/
	}
}
