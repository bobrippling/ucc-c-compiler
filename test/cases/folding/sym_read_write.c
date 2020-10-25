// RUN: %check %s -Wsym-never-read

f(int i); // CHECK: !/warn/
g(int (*pf)(int x, int y)); // CHECK: !/warn/

f(int j) // CHECK: !/warn/
{
	return j;
}

h(void (*pv)(int hello)) // CHECK: /warning: "pv" never read/
{
}

main()
{
	int i; // CHECK: /warning: "i" never read/
	int j; // CHECK: !/warn/
	int k; // CHECK:  /warning: "k" never written to/
         // CHECK: ^/warning: "k" never read/

	i = 1;
	j = 2;

	return f(j);
}
