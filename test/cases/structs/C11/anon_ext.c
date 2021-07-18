// RUN: %check -e %s -std=c11
// ... but accepted with:
// RUN: %ucc %s -fms-extensions
// RUN: %ucc %s -fplan9-extensions

struct A
{
	struct TAGGED
	{
		int i;
	}; // CHECK: warning: unnamed member 'struct TAGGED' ignored (untagged would be accepted in C11)
	int j;
};

main()
{
	struct A a;
	a.i = 3; // CHECK: /error: struct A has no member named "i"/
}
