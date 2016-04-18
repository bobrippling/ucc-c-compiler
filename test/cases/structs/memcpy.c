// RUN: %ocheck 0 %s -fstack-protector-all

typedef unsigned long size_t;

void *memset(void *, int, size_t);
_Noreturn void abort(void);

static void check_pattern(void *mem, size_t nmem, unsigned char pattern)
{
	unsigned char *p = mem;

	for(size_t i = 0; i < nmem; i++)
		if(p[i] != pattern)
			abort();
}

int main()
{
	// size mustn't be a multiple of the word size __builtin_memcpy() uses to copy
	typedef struct A { unsigned a, b, c; } A;
	typedef struct B { unsigned a, b, c, d, e; } B;
	typedef struct C { unsigned a, b, c, d, e, f, g; } C;

	/* the original bug caused an overflow by using word-pointers for
	 * less-than-word trailing bytes, so packing these together exposes it */
	A a[3] = { 0 };
	B b[3] = { 0 };
	C c[3] = { 0 };

	const unsigned char pat_a = 0b10101010;
	const unsigned char pat_b = 0b01010101;
	const unsigned char pat_c = 0b11001100;

	// fill with bit patterns
	memset(a, pat_a, sizeof(a));
	memset(b, pat_b, sizeof(b));
	memset(c, pat_c, sizeof(c));

	check_pattern(a, sizeof(a), pat_a);
	check_pattern(b, sizeof(b), pat_b);
	check_pattern(c, sizeof(c), pat_c);

	a[0] = a[1];
	a[1] = a[2];
	a[2] = a[0];

	b[0] = b[1];
	b[1] = b[2];
	b[2] = b[0];

	c[0] = c[1];
	c[1] = c[2];
	c[2] = c[0];

	check_pattern(&a, sizeof(a), pat_a);
	check_pattern(&b, sizeof(b), pat_b);
	check_pattern(&c, sizeof(c), pat_c);
}
