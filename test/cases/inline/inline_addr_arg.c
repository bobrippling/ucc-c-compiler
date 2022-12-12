// RUN: %ocheck 3 %s -fno-semantic-interposition
// RUN: %ocheck 3 %s -fno-semantic-interposition -fstack-protector-all

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
#include "../ocheck-init.c"
	int added = 5;

	added = f(2);

	return added;
}
