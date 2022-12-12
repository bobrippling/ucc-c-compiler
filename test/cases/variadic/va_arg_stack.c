// RUN: %ocheck 28 %s
// RUN: %ocheck 28 %s -fstack-protector-all

f(int i, ...)
{
	__builtin_va_list l;
	__builtin_va_start(l, i);

	int t = i;

	for(;;){
		i = __builtin_va_arg(l, int); // phi nodes all up in here
		if(i == -1)
			break;
		t += i;
	}

	return t;
}

main()
{
#include "../ocheck-init.c"
	return f(1, 2, 3, 4, 5, 6, 7, -1);
}
