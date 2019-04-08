// RUN: %ocheck 5 %s -fsanitize=bounds -fsanitize-error=call=san_fail '-DACCESS(a, i)=a[i]'
// RUN: %ocheck 5 %s -fsanitize=bounds -fsanitize-error=call=san_fail '-DACCESS(a, i)=i[a]'
//
// RUN: %ucc -o %t %s -fsanitize=bounds -fsanitize-error=call=san_fail '-DACCESS(a, i)=a[i]' -DVLA
// RUN:   %t 1 1 1 1 1
// RUN:   %t 1 1 1 1
// RUN:   %t 1 1 1
// RUN:   %t 1 1
// RUN: ! %t 1
// RUN: ! %t
// RUN: %ucc -o %t %s -fsanitize=bounds -fsanitize-error=call=san_fail '-DACCESS(a, i)=i[a]' -DVLA
// RUN:   %t 2 2 2 2 2
// RUN:   %t 2 2 2 2
// RUN:   %t 2 2 2
// RUN:   %t 2 2
// RUN: ! %t 2
// RUN: ! %t
// RUN: %ucc -o %t %s -fsanitize=bounds -fsanitize-error=call=san_fail '-DACCESS(a, i)=a[i]' -DVLA -DVLA_DOUBLEINDIR
// RUN:   %t 1 1 1 1 1
// RUN:   %t 1 1 1 1
// RUN:   %t 1 1 1
// RUN:   %t 1 1
// RUN: ! %t 1
// RUN: ! %t
// RUN: %ucc -o %t %s -fsanitize=bounds -fsanitize-error=call=san_fail '-DACCESS(a, i)=i[a]' -DVLA -DVLA_DOUBLEINDIR
// RUN:   %t 2 2 2 2 2
// RUN:   %t 2 2 2 2
// RUN:   %t 2 2 2
// RUN:   %t 2 2
// RUN: ! %t 2
// RUN: ! %t

void san_fail(void)
{
	extern _Noreturn void exit(int);
	exit(5);
}

#ifdef VLA
int f(int i)
{
#ifdef VLA_DOUBLEINDIR
	volatile int j = 5;
	int c[i][j];

	__auto_type p = &ACCESS(c, 3);
	return p[0];
#else
	int c[i];
	//memset(c, 0, sizeof(c));
	return ACCESS(c, 3);
#endif
}

int main(int argc, char **argv)
{
	f(argc);
	//f(5); // pass
	//f(4); // pass
	//f(3); // fail
	//f(2); // fail
}
#else
int f(int i)
{
	int a[4] = { 0 };
	return ACCESS(a, i);
}

int main()
{
	// currently array[N] is allowed since its address may be taken
	f(5);
}
#endif
