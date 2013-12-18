// RUN: %check %s
extern *i;
main()
{
	return 2 == i; // CHECK: /warning: mismatching types, comparison between pointer and integer/
}
