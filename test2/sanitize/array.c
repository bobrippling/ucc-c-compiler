// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_fail '-DACCESS(a, i)=a[i]'
// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_fail '-DACCESS(a, i)=i[a]'

void san_fail(void)
{
	extern _Noreturn void exit(int);
	exit(5);
}

f(int i)
{
	int a[4] = { 0 };
	return ACCESS(a, i);
}

main()
{
	f(4);
}
