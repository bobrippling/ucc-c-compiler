// RUN: %layout_check %s
// RUN: %check %s

union A
{
	struct
	{
		int i, j, k;
	} a;
	int i;
	char *p;
} x = { .i = 1, 2, 3 }; // CHECK: /warning:.*excess/

y = sizeof(x);

union A ar[] = { { 1 }, 2, 3 };

union A b = {};
