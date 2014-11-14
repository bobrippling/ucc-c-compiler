// RUN: %layout_check %s

__attribute((always_inline))
inline int f(int x);

main()
{
	int added = 5;

	added = f(2);

	return added;
}

// f's storage here is default - needs emitting
int f(int x)
{
	int i = x;
	i++;
	return i;
}
