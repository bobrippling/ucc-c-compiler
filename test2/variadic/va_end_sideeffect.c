// RUN: %ocheck 5 %s
static int x;

__builtin_va_list *g(void)
{
	static __builtin_va_list l;
	x = 5;
	return &l;
}

void f()
{
	__builtin_va_end(*g());
}

int main()
{
	f();
	return x;
}
