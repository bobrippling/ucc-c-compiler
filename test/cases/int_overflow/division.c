// RUN: %ocheck 0 %s

// https://danlark.org/2020/06/14/128-bit-division/

/*
int printf(const char *, ...)
	__attribute__((format(printf, 1, 2)));

void show(int a, int b)
{
	printf("%d / %d = %d\n", a, b, a / b);
}

void ushow(unsigned a, unsigned b)
{
	printf("%u / %u = %u\n", a, b, a / b);
}
*/

typedef unsigned T;

T div(T a, T b, T c)
{
	T res;
	T rem;

	//__asm("div %2"
	//  : "=a"(res), "=d"(rem)
	//  : "r"(a), "a"(b), "d"(c) : "cc");
	//
	// 128-bit division:
	// (b | c<<64) / a
	//
	//return -(b | c<<64) / a;

	res = -a / c;

	return res;
}

int main()
{
	// this faults because we have edx:0, eax:-1 and attempt to `_i_div` it by 1,
	// when we should be `div` by 1
	div(1, 0, 1);

	return 0;
}
