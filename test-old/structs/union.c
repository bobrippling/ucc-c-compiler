#include <assert.h>

main()
{
	union
	{
		struct
		{
			int st_first;
			int st_second;
		} st;
		int hello;
		union
		{
			int a, b, c;
		} un;
	} x;

	x.st.st_first = 5;

	assert(x.un.a == 5);
	assert(x.un.b == 5);
	assert(x.un.c == 5);

	assert(x.hello == 5);

	return *(int *)((void *)&x.st.st_second - sizeof(int)) == 5 ? 0 : 1;
}
