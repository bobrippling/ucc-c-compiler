#include <stdio.h>
#include <math.h>
#include <stddef.h>

int ec;

void assert_eq(int val, int truth, int lno, const char *variant)
{
	if(!!val == !!truth)
		return;

	printf("%s:%d: %d != %d (%s variant)\n", __FILE__, lno, val, truth, variant);
	ec = 1;
}
#define CHECK1(x, b) assert_eq((x) ? 1 : 0, b, __LINE__, "?:")
#define CHECK2(x, b) assert_eq(!!(x), b, __LINE__, "!!")
#define CHECK3(x, b) assert_eq((_Bool)(x), b, __LINE__, "_Bool")
#define CHECK(x, b) CHECK1(x, b); CHECK2(x, b); CHECK3(x, b)

int main()
{
	CHECK(NAN, 1);
	CHECK("", 1);
	CHECK(NULL, 0);
	CHECK(0, 0);

	return ec;
}
