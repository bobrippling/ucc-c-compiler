// RUN: %debug_check %s

typedef float fp;

typedef struct A
{
	char c;
	short s;
	char pad;
	int i;
	long l;
	char *p;
	char buf[5], buf2[6];
	fp f, fs[2];
	double d;
} A;

A x = {
	.p = "hello",
	.s = 5,
	.buf = "hello",
	.buf2 = "hello",
	.d = 3.251,
	.f = 59.7,
	.fs[1] = { 5 }
};
