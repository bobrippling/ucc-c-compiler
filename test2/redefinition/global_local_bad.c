// RUN: %check -e %s
f(int);

main()
{
	int f(char *); // CHECK: /error: incompatible redefinition of "f"/
}
