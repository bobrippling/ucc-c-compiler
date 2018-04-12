// RUN: %ocheck 0 %s

extern _Noreturn void abort(void);

void f(void) __attribute__((aligned(8)))
{
}

/*
	 not allowed
_Alignas(8)
void g(void)
{
}

_Alignas(8)
void h(void)
{
}
*/

__attribute__((aligned(8)))
void i(void)
{
}

void assert_zero(unsigned long v)
{
	if(v)
		abort();
}

int main(void) {
	assert_zero((unsigned long)f & 0b111);
	//assert_zero((unsigned long)g & 0b111);
	//assert_zero((unsigned long)h & 0b111);
	assert_zero((unsigned long)i & 0b111);
	return 0;
}
