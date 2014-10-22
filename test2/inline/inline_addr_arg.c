// RUN: %ocheck 3 %s

__attribute((always_inline))
inline f(int x)
{
	int i = x;
	&x;
	i++;
	return i;
}

main()
{
	int added = 5;

	added = f(2);

	return added;
}
