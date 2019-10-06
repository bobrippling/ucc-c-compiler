// RUN: %ocheck 0 %s

_Noreturn extern void abort(void);

void f(void) __attribute__((aligned(8)))
{
}

__attribute__((aligned(32)))
void g(void)
{
}
// _Alignas() not permitted on functions

void assert_zero(unsigned long v)
{
	if(v)
		abort();
}

_Static_assert(_Alignof(f) == 8, "");
_Static_assert(__alignof(f) == 8, "");

_Static_assert(_Alignof(g) == 32, "");
_Static_assert(__alignof(g) == 32, "");

int main(void)
{
	assert_zero((unsigned long)f & 0b111);
	assert_zero((unsigned long)g & 0b11111);
	return 0;
}
