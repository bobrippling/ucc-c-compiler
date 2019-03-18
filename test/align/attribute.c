// RUN: %ucc -c %s
struct A
{
	char a, b, c;
} __attribute((aligned(8))) x;

__attribute__((aligned)) int max_1;
__attribute__((aligned())) int max_2;

_Static_assert(_Alignof(x) == 8, "");

void f()
{
	int i __attribute__((aligned(8))) = 3;

	_Static_assert(_Alignof(i) == 8, "");
}
