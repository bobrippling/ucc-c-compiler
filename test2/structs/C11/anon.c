// RUN: %check %s -std=c99
// RUN: %check %s -std=c89
// RUN: %ucc -std=c11 %s | grep 'anonymous'; [ $? -ne 0 ]

struct A
{
	struct
	{
		int i;
	}; // CHECK: /warning: unnamed member '.*' is a C11 extension/
	int j;
};

main()
{
	struct A a;
	a.i = 3;
}
