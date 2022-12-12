// RUN: %ocheck 0 %s
int memcmp(const void *, const void *, unsigned long);

_Noreturn void abort(void);

struct A
{
	int b;
	struct
	{
		int pad1;
		struct
		{
			int pad2;
			struct
			{
				int pad3;
				int i, j;
			};
		};
	};
	int k;
};

f(struct A *p)
{
	p->i = 1;
}

main()
{
#include "../../ocheck-init.c"
	struct A a = { .i = 3 };
	int ar[7] = { [4] = 3 };

	if(memcmp(&a, ar, 7 * sizeof(int)) != 0)
		abort();
}
