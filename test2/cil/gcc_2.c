// RUN: %ucc -o %t %s
// RUN: %t

ncalls;

f()
{
	ncalls++;
	return 3;
}

main()
{
	extern int f();
	int v = f() ? : -1; // Returns the result of f unless it is 0

	return v == 3 && ncalls == 1 ? 0 : 1;
}
