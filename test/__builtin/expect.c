// RUN: %ocheck 3 %s

#define unlikely(x) __builtin_expect(!!(x), 0)

f(unsigned i)
{
	if(unlikely(i))
		f(i - 1);
}

g()
{
	return 3;
}

main()
{
	f(2);

	return __builtin_expect(g(), 3);
}
