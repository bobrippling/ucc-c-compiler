// mpreferred-stack-boundary to ensure that padding for regspill
// doesn't save us any overwrites.

// RUN: %ocheck 3 %s -mpreferred-stack-boundary=3

f(){}
g(){}
h(){}

main()
{
	long l = 3; // use a long to ensure we take up the entire stack line
	f(g(), h());

	f(g(), h());
	return l; // shouldn't be clobbered by reg-saves
}
