// RUN: %check -e %s

f(int i)
{
	extern f(void); // CHECK: /error: incompatible redefinition of "f"/
	return 5;
}
