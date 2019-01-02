// RUN: %check %s

f(char *arg)
{
	char *const kp = arg; // CHECK: !/warn/
	const char *ks = arg; // CHECK: !/warn/
	char *s = ks; // CHECK: /warning: mismatching types/

	g(kp, ks, s); // use them - no warnings
}
