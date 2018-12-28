// RUN: %layout_check %s

struct A
{
	union
	{
		const char *s;
		int i;
	} bits;
	unsigned bf : 1;
	enum { A, B, C } ty;
	char tail;
} x[] = {
	{ { .s = 0 }, 0, A, '1' },
	{ { .i = 0 }, 0, C, '3' },
	{ { .s = 0 }, 0, A, '5' },
	{ { .s = 0 }, 0, A, '6' },
	{ { .i = 0 }, 0, B, '8' },
	{ { .s = 0 }, 0, A, '0' },
};
