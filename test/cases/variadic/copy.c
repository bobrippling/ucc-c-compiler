// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

typedef __builtin_va_list va_list;

int maths(int a, ...)
{
	va_list l, m;
	__builtin_va_start(l, a);

	int i = 0;
	int t = 0;
	for(; a; a = __builtin_va_arg(l, int), i++){
		t += a;

		if(i == 1)
			__builtin_va_copy(m, l);
	}

	__builtin_va_end(l);

	do{
		a = __builtin_va_arg(m, int);
		t += a;
	}while(a);

	__builtin_va_end(m);

	return t;
}

int main()
{
	if(maths(1, 2, 3, 0) != 9)
		abort();
	return 0;
}
