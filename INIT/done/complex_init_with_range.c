//int x[] = { [3] = 5 };
//struct { int x, y, z; } q = { .y = 2, 3 };
//int k[] = { [7 ... 9] = 2 };

struct A
{
	int i;
	char buf[3];
	struct B
	{
		int q;
		int a[2];
	} sub_b[10];

} x[] = {
	[0 ... 3] = {
		.i = 2,
		.buf[0 ... 1] = 7,
		.sub_b[3 ... 5].q = 1
	},
	[8].sub_b[3].a = { 2, 3 }
};

// FIXME: see asm comment
