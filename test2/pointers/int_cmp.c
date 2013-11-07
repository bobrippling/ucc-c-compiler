// RUN: %check %s
extern *i;
main()
{
	return 2 == i; // CHECK: /warning: comparison between pointer and integer \(int vs int \*\)/
}
