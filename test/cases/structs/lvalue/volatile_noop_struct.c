// RUN: %archgen %s 'x86_64:movq (%%rax), %%rcx' -ffreestanding
// RUN: %archgen %s 'x86_64:movq (%%rax), %%rcx' -DTO_VOID -ffreestanding
// RUN: %archgen %s 'x86_64:/call.*memcpy/' -fno-freestanding
// RUN: %archgen %s 'x86_64:/call.*memcpy/' -DTO_VOID -fno-freestanding

struct A
{
	long a, b, c, d;
};

void f(volatile struct A *p)
{
#ifdef TO_VOID
	(void)
#endif
	*p;
}

int main()
{
	f(0);

	return 0;
}
