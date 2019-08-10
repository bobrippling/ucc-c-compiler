// RUN: %ucc -target x86_64-linux -S -o- %s -ffreestanding | grep -F 'movq (%%rax), %%rcx'
// RUN: %ucc -target x86_64-linux -S -o- %s -DTO_VOID -ffreestanding | grep -F 'movq (%%rax), %%rcx'
// RUN: %ucc -target x86_64-linux -S -o- %s -fno-freestanding | grep 'call.*memcpy'
// RUN: %ucc -target x86_64-linux -S -o- %s -DTO_VOID -fno-freestanding | grep 'call.*memcpy'

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
