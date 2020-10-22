// RUN: %check --only %s -Wno-int-ptr-conversion
extern *i;
main()
{
	return 2 == i; // CHECK: /warning: mismatching types, comparison between pointer and integer/
}
