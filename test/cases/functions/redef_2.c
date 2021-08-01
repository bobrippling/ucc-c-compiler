// RUN: %check -e %s
main()
{
	extern f(void);
	{
		extern f(int); // CHECK: /error: incompatible redefinition of "f"/
		f(2);
	}
}
