// RUN: %layout_check %s
// RUN: %check --only %s

int x[]; // CHECK: warning: tenative array definition assumed to have one element

f(int x[]);

extern char (*(*y)[])();

main()
{
	char c;

	//c = (*y[0])();
	c = (*(*y)[0])();
}
